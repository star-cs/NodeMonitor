#include "net_monitor.hpp"
#include <chrono>
#include <bcc/BPF.h>

using namespace monitor;

struct NetStat {
    std::string name;
    uint64_t rcv_bytes;
    uint64_t rcv_packets;
    uint64_t snd_bytes;
    uint64_t snd_packets;
    uint64_t err_in;   // 新增: 接收错误计数，用于判断网卡/驱动是否异常
    uint64_t err_out;  // 新增: 发送错误计数，辅助判断链路质量
    uint64_t drop_in;  // 新增: 接收丢弃数，反映内核队列压力
    uint64_t drop_out; // 新增: 发送丢弃数，判断应用层处理能力
};

// eBPF C代码，统计每个网卡的收发包/字节数及错误/丢弃信息
const std::string kNetEbpfProgram = R"(
    BPF_HASH(net_stats_map, char[16], struct net_stats);
    
    struct net_stats {
        u64 rcv_bytes;
        u64 rcv_packets;
        u64 snd_bytes;
        u64 snd_packets;
        u64 err_in;           // 新增: 接收错误计数，用于判断网卡/驱动是否异常
        u64 err_out;          // 新增: 发送错误计数，辅助判断链路质量
        u64 drop_in;          // 新增: 接收丢弃数，反映内核队列压力
        u64 drop_out;         // 新增: 发送丢弃数，判断应用层处理能力
    };
    
    int count_ingress(struct __sk_buff *skb) {
        char ifname[16] = {};
        bpf_get_ifname(skb->ifindex, ifname, sizeof(ifname));
        struct net_stats *stats = net_stats_map.lookup(&ifname);
        struct net_stats zero = {};
        if (!stats) {
            net_stats_map.update(&ifname, &zero);
            stats = net_stats_map.lookup(&ifname);
        }
        if (stats) {
            __sync_fetch_and_add(&stats->rcv_bytes, skb->len);
            __sync_fetch_and_add(&stats->rcv_packets, 1);
            __sync_fetch_and_add(&stats->err_in, skb->rx_error);  // 新增: 统计接收错误
            __sync_fetch_and_add(&stats->drop_in, skb->dropped);  // 新增: 统计接收丢弃
        }
        return 0;
    }
    
    int count_egress(struct __sk_buff *skb) {
        char ifname[16] = {};
        bpf_get_ifname(skb->ifindex, ifname, sizeof(ifname));
        struct net_stats *stats = net_stats_map.lookup(&ifname);
        struct net_stats zero = {};
        if (!stats) {
            net_stats_map.update(&ifname, &zero);
            stats = net_stats_map.lookup(&ifname);
        }
        if (stats) {
            __sync_fetch_and_add(&stats->snd_bytes, skb->len);
            __sync_fetch_and_add(&stats->snd_packets, 1);
            __sync_fetch_and_add(&stats->err_out, skb->tx_error); // 新增: 统计发送错误
            __sync_fetch_and_add(&stats->drop_out, skb->dropped); // 新增: 统计发送丢弃
        }
        return 0;
    }
)";

// 单例BPF对象和加载标志
static ebpf::BPF *g_bpf = nullptr;
static std::once_flag g_bpf_init_flag;

// 加载eBPF程序并挂载到所有网卡
static void load_ebpf_program()
{
    g_bpf = new ebpf::BPF();
    g_bpf->init(kNetEbpfProgram);

    // 遍历 /sys/class/net 获取所有物理网卡名
    std::vector<std::string> ifaces;
    DIR *dir = opendir("/sys/class/net");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string ifname = entry->d_name;
            // 跳过 . .. lo 虚拟网卡和以veth、docker、br、virbr等开头的虚拟设备
            if (ifname == "." || ifname == ".." || ifname == "lo" || ifname.find("veth") == 0
                || ifname.find("docker") == 0 || ifname.find("br") == 0
                || ifname.find("virbr") == 0)
                continue;
            ifaces.push_back(ifname);
        }
        closedir(dir);
    }

    // 挂载到每个网卡的 ingress/egress
    for (const auto &iface : ifaces) {
        try {
            g_bpf->attach_tc(iface, "count_ingress", "ingress");
        } catch (...) {
            // 已挂载或不支持时忽略
        }
        try {
            g_bpf->attach_tc(iface, "count_egress", "egress");
        } catch (...) {
            // 已挂载或不支持时忽略
        }
    }
}

// eBPF采集实现：通过bcc读取eBPF map
static std::vector<NetStat> ebpf_get_net_stats()
{
    std::call_once(g_bpf_init_flag, load_ebpf_program);

    std::vector<NetStat> stats;
    auto table =
        g_bpf->get_hash_table<std::string, std::tuple<uint64_t, uint64_t, uint64_t, uint64_t,
                                                      uint64_t, uint64_t, uint64_t, uint64_t>>(
            "net_stats_map");
    for (const auto &it : table.get_table_offline()) {
        NetStat s;
        s.name = it.first;
        s.rcv_bytes = std::get<0>(it.second);
        s.rcv_packets = std::get<1>(it.second);
        s.snd_bytes = std::get<2>(it.second);
        s.snd_packets = std::get<3>(it.second);
        s.err_in = std::get<4>(it.second);   // 新增: 接收错误计数
        s.err_out = std::get<5>(it.second);  // 新增: 发送错误计数
        s.drop_in = std::get<6>(it.second);  // 新增: 接收丢弃数
        s.drop_out = std::get<7>(it.second); // 新增: 发送丢弃数
        stats.push_back(s);
    }
    return stats;
}

void NetMonitor::UpdateOnce(MonitorInfo *monitor_info)
{
    auto now = std::chrono::steady_clock::now();

    auto stats = ebpf_get_net_stats();

    for (const auto &stat : stats) {
        auto it = last_net_info_.find(stat.name);
        double rcv_rate = 0, rcv_packets_rate = 0, send_rate = 0, send_packets_rate = 0;
        double err_in_rate = 0, err_out_rate = 0, drop_in_rate = 0, drop_out_rate = 0;

        if (it != last_net_info_.end()) {
            const NetInfo &last = it->second;
            double dt = std::chrono::duration<double>(now - last.timepoint).count();
            if (dt > 0) {
                rcv_rate = (stat.rcv_bytes - last.rcv_bytes) / 1024.0 / dt; // KB/s
                rcv_packets_rate = (stat.rcv_packets - last.rcv_packets) / dt;
                send_rate = (stat.snd_bytes - last.snd_bytes) / 1024.0 / dt; // KB/s
                send_packets_rate = (stat.snd_packets - last.snd_packets) / dt;
                err_in_rate = (stat.err_in - last.err_in) / dt;
                err_out_rate = (stat.err_out - last.err_out) / dt;
                drop_in_rate = (stat.drop_in - last.drop_in) / dt;
                drop_out_rate = (stat.drop_out - last.drop_out) / dt;
            }
        }

        // 填充 protobuf
        auto net_info = monitor_info->add_net_info();
        net_info->set_name(stat.name);
        net_info->set_rcv_rate(rcv_rate);
        net_info->set_rcv_packets_rate(rcv_packets_rate);
        net_info->set_send_rate(send_rate);
        net_info->set_send_packets_rate(send_packets_rate);
        net_info->set_err_in_rate(err_in_rate);     // 新增: 接收错误速率
        net_info->set_err_out_rate(err_out_rate);   // 新增: 发送错误速率
        net_info->set_drop_in_rate(drop_in_rate);   // 新增: 接收丢弃速率
        net_info->set_drop_out_rate(drop_out_rate); // 新增: 发送丢弃速率

        // 更新缓存
        last_net_info_[stat.name] =
            NetInfo{stat.name,   stat.rcv_bytes, stat.rcv_packets, stat.snd_bytes, stat.snd_packets,
                    stat.err_in, stat.err_out,   stat.drop_in,     stat.drop_out,  now};
    }
}
