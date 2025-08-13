#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

// 使用标准库函数获取UID和用户名
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include "node_client/src/monitor_base.h"
#include "node_client/src/monitor/cpu_load_monitor.hpp"
#include "node_client/src/monitor/cpu_softirq_monitor.hpp"
#include "node_client/src/monitor/cpu_stat_monitor.hpp"
#include "node_client/src/monitor/disk_monitor.hpp"
#include "node_client/src/monitor/mem_monitor.hpp"
#include "node_client/src/monitor/net_monitor.hpp"

int main()
{
    std::vector<std::shared_ptr<monitor::MonitorBase>> runners_;
    runners_.emplace_back(std::make_shared<monitor::CpuLoadMonitor>());
    runners_.emplace_back(std::make_shared<monitor::CpuSoftIrqMonitor>());
    runners_.emplace_back(std::make_shared<monitor::CpuStatMonitor>());
    runners_.emplace_back(std::make_shared<monitor::DiskMonitor>());
    runners_.emplace_back(std::make_shared<monitor::MemMonitor>());
    runners_.emplace_back(std::make_shared<monitor::NetMonitor>());

    monitor::RpcClient rpc_client_;
    uid_t uid = getuid();  // 使用标准函数获取UID
    struct passwd *pwd = getpwuid(uid); // 使用标准函数获取用户信息
    std::string username = pwd ? pwd->pw_name : "unknown_user"; // 如果获取失败，使用默认用户名

    std::thread thread_([&]() {
        while (true) {
            monitor::proto::MonitorInfo monitor_info;
            monitor_info.set_name(username); // 使用自定义获取的用户名
            for (auto &runner : runners_) {
                runner->UpdateOnce(&monitor_info);
            }

            rpc_client_.SetMonitorInfo(monitor_info);
            std::this_thread::sleep_for(std::chrono::seconds(3)); // 修改: 使用标准库的 chrono
        }
    });
    thread_.join();
    return 0;
}