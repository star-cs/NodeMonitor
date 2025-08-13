#pragma once

#include "src/monitor_base.h"
#include "node_client/src/monitor_struct.h"

#include <fcntl.h>
#include <sys/mman.h>
namespace monitor
{

class CpuStatMonitor : public MonitorBase
{
public:
    CpuStatMonitor() {}
    void UpdateOnce(monitor::proto::MonitorInfo *monitor_info) override
    {
        int fd = open("/dev/cpu_stat_monitor", O_RDONLY);
        if (fd < 0)
            return;

        size_t stat_count = 128; // 假设最多128个CPU
        size_t stat_size = sizeof(struct cpu_stat) * stat_count;
        void *addr = mmap(nullptr, stat_size, PROT_READ, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            return;
        }

        struct cpu_stat *stats = static_cast<struct cpu_stat *>(addr);
        for (size_t i = 0; i < stat_count; ++i) {
            if (stats[i].cpu_name[0] == '\0')
                break;
            auto it = cpu_stat_map_.find(stats[i].cpu_name);
            if (it != cpu_stat_map_.end()) {
                struct cpu_stat old = it->second;
                auto cpu_stat_msg = monitor_info->add_cpu_stat();
                float new_cpu_total_time = stats[i].user + stats[i].system + stats[i].idle
                                           + stats[i].nice + stats[i].io_wait + stats[i].irq
                                           + stats[i].soft_irq + stats[i].steal;
                float old_cpu_total_time = old.user + old.system + old.idle + old.nice + old.io_wait
                                           + old.irq + old.soft_irq + old.steal;
                float new_cpu_busy_time = stats[i].user + stats[i].system + stats[i].nice
                                          + stats[i].irq + stats[i].soft_irq + stats[i].steal;
                float old_cpu_busy_time =
                    old.user + old.system + old.nice + old.irq + old.soft_irq + old.steal;

                // CPU使用率 = (当前忙碌时间 - 上次忙碌时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_percent = (new_cpu_busy_time - old_cpu_busy_time)
                                    / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // 用户态使用率 = (当前user时间 - 上次user时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_user_percent =
                    (stats[i].user - old.user) / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // 系统态使用率 = (当前system时间 - 上次system时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_system_percent = (stats[i].system - old.system)
                                           / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // Nice使用率 = (当前nice时间 - 上次nice时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_nice_percent =
                    (stats[i].nice - old.nice) / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // 空闲率 = (当前idle时间 - 上次idle时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_idle_percent =
                    (stats[i].idle - old.idle) / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // I/O等待率 = (当前io_wait时间 - 上次io_wait时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_io_wait_percent = (stats[i].io_wait - old.io_wait)
                                            / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // 硬中断使用率 = (当前irq时间 - 上次irq时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_irq_percent =
                    (stats[i].irq - old.irq) / (new_cpu_total_time - old_cpu_total_time) * 100.00;

                // 软中断使用率 = (当前soft_irq时间 - 上次soft_irq时间) / (当前总时间 - 上次总时间) × 100%
                float cpu_soft_irq_percent = (stats[i].soft_irq - old.soft_irq)
                                             / (new_cpu_total_time - old_cpu_total_time) * 100.00;
                                             
                cpu_stat_msg->set_cpu_name(stats[i].cpu_name);
                cpu_stat_msg->set_cpu_percent(cpu_percent);
                cpu_stat_msg->set_usr_percent(cpu_user_percent);
                cpu_stat_msg->set_system_percent(cpu_system_percent);
                cpu_stat_msg->set_nice_percent(cpu_nice_percent);
                cpu_stat_msg->set_idle_percent(cpu_idle_percent);
                cpu_stat_msg->set_io_wait_percent(cpu_io_wait_percent);
                cpu_stat_msg->set_irq_percent(cpu_irq_percent);
                cpu_stat_msg->set_soft_irq_percent(cpu_soft_irq_percent);
            }
            cpu_stat_map_[stats[i].cpu_name] = stats[i];
        }

        munmap(addr, stat_size);
        close(fd);
        return;
    }
    void Stop() override {}

private:
    std::unordered_map<std::string, struct cpu_stat> cpu_stat_map_;
};

} // namespace monitor