#include "agent_manager.h"
#include <chrono>
#include <memory>

using namespace monitor;

namespace
{
const char *MYSQL_HOST = "127.0.0.1";
const char *MYSQL_USER = "root";
const char *MYSQL_PASS = "your_password";
const char *MYSQL_DB = "monitor_db";
const char *MYSQL_TABLE = "monitor_data";

struct NetSample {
    double last_in_bytes = 0;
    double last_out_bytes = 0;
    std::chrono::system_clock::time_point last_time;
};
std::map<std::string, NetSample> net_samples;

struct TrendSample {
    double cpu_percent = 0;
    double load_avg_1 = 0;
    double mem_used_percent = 0;
    double net_in_rate = 0;
    double net_out_rate = 0;
    std::chrono::system_clock::time_point last_time;
};
std::map<std::string, TrendSample> trend_samples; // key: server_name

struct PerfSample {
    // 所有主表关键指标
    float cpu_percent = 0, usr_percent = 0, system_percent = 0, nice_percent = 0, idle_percent = 0;
    float io_wait_percent = 0, irq_percent = 0, soft_irq_percent = 0;
    float steal_percent = 0, guest_percent = 0, guest_nice_percent = 0;
    float load_avg_1 = 0, load_avg_3 = 0, load_avg_15 = 0;
    float mem_used_percent = 0, mem_total = 0, mem_free = 0, mem_avail = 0;
    float mem_swap_used = 0, mem_swap_total = 0, mem_commit = 0, mem_commit_limit = 0;
    float net_in_rate = 0, net_out_rate = 0, net_in_peak = 0, net_out_peak = 0;
    float net_in_drop_rate = 0, net_out_drop_rate = 0;
    float score = 0;
};

std::map<std::string, PerfSample> last_perf_samples; // key: server_name

} // namespace

AgentManager::AgentManager(const std::vector<std::string> &agent_addrs)
    : agent_addrs_(agent_addrs), running_(true)
{
    for (const auto &addr : agent_addrs_) {
        rpc_clients_.emplace_back(std::make_unique<RpcClient>(addr));
    }
}

AgentManager::~AgentManager()
{
    running_ = false;
    if (thread_ && thread_->joinable())
        thread_->join();
}

void AgentManager::Start()
{
    thread_ = std::make_unique<std::thread>(&AgentManager::FetchAndScoreLoop, this);
}

void AgentManager::FetchAndScoreLoop()
{
    while (running_) {
        for (size_t i = 0; i < rpc_clients_.size(); ++i) {
            MonitorInfo info;
            rpc_clients_[i]->GetMonitorInfo(&info);
            std::string server_name = info.name();
            double score = CalcScore(info);
            auto now = std::chrono::system_clock::now();

            // 网络速率计算
            double net_in_rate = 0, net_out_rate = 0;
            if (info.net_info_size() > 0) {
                // double in_bytes = info.net_info(0).rcv_bytes();
                // double out_bytes = info.net_info(0).snd_bytes();
                // todo
                double in_bytes = 0, out_bytes = 0;
                auto it = net_samples.find(server_name);
                if (it != net_samples.end()) {
                    double seconds =
                        std::chrono::duration<double>(now - it->second.last_time).count();
                    if (seconds > 0) {
                        net_in_rate =
                            (in_bytes - it->second.last_in_bytes) / (1024.0 * 1024.0) / seconds;
                        net_out_rate =
                            (out_bytes - it->second.last_out_bytes) / (1024.0 * 1024.0) / seconds;
                        if (net_in_rate < 0)
                            net_in_rate = 0;
                        if (net_out_rate < 0)
                            net_out_rate = 0;
                    }
                }
                net_samples[server_name] = {in_bytes, out_bytes, now};
            }

            // 当前采样
            PerfSample curr;
            if (info.cpu_stat_size() > 0) {
                const auto &cpu = info.cpu_stat(0);
                curr.cpu_percent = cpu.cpu_percent();
                curr.usr_percent = cpu.usr_percent();
                curr.system_percent = cpu.system_percent();
                curr.nice_percent = cpu.nice_percent();
                curr.idle_percent = cpu.idle_percent();
                curr.io_wait_percent = cpu.io_wait_percent();
                curr.irq_percent = cpu.irq_percent();
                curr.soft_irq_percent = cpu.soft_irq_percent();
                curr.steal_percent = cpu.steal_percent();
                curr.guest_percent = cpu.guest_percent();
                curr.guest_nice_percent = cpu.guest_nice_percent();
            }
            if (info.has_cpu_load()) {
                curr.load_avg_1 = info.cpu_load().load_avg_1();
                curr.load_avg_3 = info.cpu_load().load_avg_3();
                curr.load_avg_15 = info.cpu_load().load_avg_15();
            }
            if (info.has_mem_info()) {
                const auto &mem = info.mem_info();
                curr.mem_used_percent = mem.used_percent();
                curr.mem_total = mem.total();
                curr.mem_free = mem.free();
                curr.mem_avail = mem.avail();
                curr.mem_swap_used = mem.swap_used();
                curr.mem_swap_total = mem.swap_total();
                curr.mem_commit = mem.commit();
                curr.mem_commit_limit = mem.commit_limit();
            }
            curr.net_in_rate = net_in_rate;
            curr.net_out_rate = net_out_rate;
            curr.net_in_peak = 0; // 可根据历史最大值实现
            curr.net_out_peak = 0;
            curr.net_in_drop_rate = 0;
            curr.net_out_drop_rate = 0;
            curr.score = score;

            // 变化率计算（仅主表字段）
            PerfSample last = last_perf_samples[server_name];
            auto rate = [](float now, float last) -> float {
                if (last == 0)
                    return 0;
                return (now - last) / last;
            };

            // 只为主表字段计算变化率
            float cpu_percent_rate = rate(curr.cpu_percent, last.cpu_percent);
            float usr_percent_rate = rate(curr.usr_percent, last.usr_percent);
            float system_percent_rate = rate(curr.system_percent, last.system_percent);
            float nice_percent_rate = rate(curr.nice_percent, last.nice_percent);
            float idle_percent_rate = rate(curr.idle_percent, last.idle_percent);
            float io_wait_percent_rate = rate(curr.io_wait_percent, last.io_wait_percent);
            float irq_percent_rate = rate(curr.irq_percent, last.irq_percent);
            float soft_irq_percent_rate = rate(curr.soft_irq_percent, last.soft_irq_percent);
            float steal_percent_rate = rate(curr.steal_percent, last.steal_percent);
            float guest_percent_rate = rate(curr.guest_percent, last.guest_percent);
            float guest_nice_percent_rate = rate(curr.guest_nice_percent, last.guest_nice_percent);
            float load_avg_1_rate = rate(curr.load_avg_1, last.load_avg_1);
            float load_avg_3_rate = rate(curr.load_avg_3, last.load_avg_3);
            float load_avg_15_rate = rate(curr.load_avg_15, last.load_avg_15);
            float mem_used_percent_rate = rate(curr.mem_used_percent, last.mem_used_percent);
            float mem_total_rate = rate(curr.mem_total, last.mem_total);
            float mem_free_rate = rate(curr.mem_free, last.mem_free);
            float mem_avail_rate = rate(curr.mem_avail, last.mem_avail);
            float mem_swap_used_rate = rate(curr.mem_swap_used, last.mem_swap_used);
            float mem_swap_total_rate = rate(curr.mem_swap_total, last.mem_swap_total);
            float mem_commit_rate = rate(curr.mem_commit, last.mem_commit);
            float mem_commit_limit_rate = rate(curr.mem_commit_limit, last.mem_commit_limit);
            float net_in_rate_rate = rate(curr.net_in_rate, last.net_in_rate);
            float net_out_rate_rate = rate(curr.net_out_rate, last.net_out_rate);
            float net_in_peak_rate = rate(curr.net_in_peak, last.net_in_peak);
            float net_out_peak_rate = rate(curr.net_out_peak, last.net_out_peak);
            float net_in_drop_rate_rate = rate(curr.net_in_drop_rate, last.net_in_drop_rate);
            float net_out_drop_rate_rate = rate(curr.net_out_drop_rate, last.net_out_drop_rate);
            float score_rate = rate(curr.score, last.score);

            last_perf_samples[server_name] = curr;

            std::lock_guard<std::mutex> lock(mtx_);
            agent_scores_[server_name] = AgentScore{info, score, now};
            WriteToMysql(
                server_name, agent_scores_[server_name], net_in_rate, net_out_rate,
                cpu_percent_rate, usr_percent_rate, system_percent_rate, nice_percent_rate,
                idle_percent_rate, io_wait_percent_rate, irq_percent_rate, soft_irq_percent_rate,
                steal_percent_rate, guest_percent_rate, guest_nice_percent_rate, load_avg_1_rate,
                load_avg_3_rate, load_avg_15_rate, mem_used_percent_rate, mem_total_rate,
                mem_free_rate, mem_avail_rate,
                /* 移除 mem_swap_used_rate, mem_swap_total_rate, mem_commit_rate, mem_commit_limit_rate, */
                net_in_rate_rate, net_out_rate_rate, net_in_peak_rate, net_out_peak_rate,
                net_in_drop_rate_rate, net_out_drop_rate_rate, score_rate);
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

/**
重要性：cpu使用率 cpu负载 内存使用率 网络接收 网络发送
*/
double AgentManager::CalcScore(const MonitorInfo &info)
{
    // 权重
    const double cpu_weight = 0.4;
    const double load_weight = 0.3;
    const double mem_weight = 0.2;
    const double net_recv_weight = 0.05;
    const double net_send_weight = 0.05;

    // 指标
    double cpu_percent = 0;
    double load_avg_1 = 0;
    double mem_percent = 0;
    double net_recv_rate = 0;
    double net_send_rate = 0;
    int cpu_cores = 1; // 默认1核，避免除0

    // CPU整体使用率
    if (info.cpu_stat_size() > 0) {
        cpu_percent = info.cpu_stat(0).cpu_percent();
        cpu_cores = info.cpu_stat_size();
    }
    // 1分钟负载
    if (info.has_cpu_load()) {
        load_avg_1 = info.cpu_load().load_avg_1();
    }
    // 内存使用率
    if (info.has_mem_info()) {
        mem_percent = info.mem_info().used_percent();
    }

    // 网络速率（假设net_info第一个为主网卡，实际可遍历所有网卡求和/取最大）
    if (info.net_info_size() > 0) {
        net_recv_rate = info.net_info(0).send_rate();
        net_send_rate = info.net_info(0).rcv_rate();
    }

    // 归一化（反向归一化，越小越好，分数越高）
    double cpu_score = 1.0 - cpu_percent / 100.0;
    if (cpu_score < 0)
        cpu_score = 0;
    double load_score = 1.0 - (load_avg_1 / cpu_cores);
    if (load_score < 0)
        load_score = 0;
    double mem_score = 1.0 - mem_percent / 100.0;
    if (mem_score < 0)
        mem_score = 0;
    // 假设最大带宽为1Gbps=125000000B/s
    const double max_bandwidth = 125000000.0;
    double net_recv_score = 1.0 - net_recv_rate / max_bandwidth;
    if (net_recv_score < 0)
        net_recv_score = 0;
    double net_send_score = 1.0 - net_send_rate / max_bandwidth;
    if (net_send_score < 0)
        net_send_score = 0;

    // 加权求和
    double score = cpu_score * cpu_weight + load_score * load_weight + mem_score * mem_weight
                   + net_recv_score * net_recv_weight + net_send_score * net_send_weight;

    // 转为百分制
    score *= 100.0;
    if (score < 0)
        score = 0;
    if (score > 100)
        score = 100;
    return score;
}

void AgentManager::WriteToMysql(
    const std::string &server_name, const AgentScore &agent_score, double net_in_rate,
    double net_out_rate, float cpu_percent_rate, float usr_percent_rate, float system_percent_rate,
    float nice_percent_rate, float idle_percent_rate, float io_wait_percent_rate,
    float irq_percent_rate, float soft_irq_percent_rate, float steal_percent_rate,
    float guest_percent_rate, float guest_nice_percent_rate, float load_avg_1_rate,
    float load_avg_3_rate, float load_avg_15_rate, float mem_used_percent_rate,
    float mem_total_rate, float mem_free_rate, float mem_avail_rate, float net_in_rate_rate,
    float net_out_rate_rate, float net_in_peak_rate, float net_out_peak_rate,
    float net_in_drop_rate_rate, float net_out_drop_rate_rate, float score_rate)
{
    // todo
}
