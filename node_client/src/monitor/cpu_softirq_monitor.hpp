#pragma once

#include "src/monitor_base.h"
#include "node_client/src/monitor_struct.h"

#include <fcntl.h>
#include <sys/mman.h>
namespace monitor
{
class CpuSoftIrqMonitor : public MonitorBase
{
    struct SoftIrq {
        std::string cpu_name;
        int64_t hi;
        int64_t timer;
        int64_t net_tx;
        int64_t net_rx;
        int64_t block;
        int64_t irq_poll;
        int64_t tasklet;
        int64_t sched;
        int64_t hrtimer;
        int64_t rcu;
        std::chrono::steady_clock::time_point timepoint; // 修改: 使用标准库时间戳
    };

public:
    CpuSoftIrqMonitor() {}
    void UpdateOnce(monitor::proto::MonitorInfo *monitor_info) override
    {
        int fd = open("/dev/cpu_softirq_monitor", O_RDONLY);
        if (fd < 0)
            return;

        size_t stat_count = 128; // 设定最多128个CPU
        size_t stat_size = sizeof(struct softirq_stat) * stat_count;
        void *addr = mmap(nullptr, stat_size, PROT_READ, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            return;
        }

        struct softirq_stat *stats = static_cast<struct softirq_stat *>(addr);
        for (size_t i = 0; i < stat_count; ++i) {
            if (stats[i].cpu_name[0] == '\0')
                break;
            auto one_softirq_msg = monitor_info->add_soft_irq();
            one_softirq_msg->set_cpu(stats[i].cpu_name);
            one_softirq_msg->set_hi(stats[i].hi);
            one_softirq_msg->set_timer(stats[i].timer);
            one_softirq_msg->set_net_tx(stats[i].net_tx);
            one_softirq_msg->set_net_rx(stats[i].net_rx);
            one_softirq_msg->set_block(stats[i].block);
            one_softirq_msg->set_irq_poll(stats[i].irq_poll);
            one_softirq_msg->set_tasklet(stats[i].tasklet);
            one_softirq_msg->set_sched(stats[i].sched);
            one_softirq_msg->set_hrtimer(stats[i].hrtimer);
            one_softirq_msg->set_rcu(stats[i].rcu);
        }
        munmap(addr, stat_size);
        close(fd);
    }
    void Stop() override {}

private:
    std::unordered_map<std::string, struct SoftIrq> cpu_softirqs_;
};
} // namespace monitor