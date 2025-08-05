#include "agent_manager.h"
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

void handle_signal(int sig)
{
    exit(1);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // 解析命令行参数，获取所有 agent 地址
    std::vector<std::string> agent_addrs;
    for (int i = 1; i < argc; ++i) {
        agent_addrs.emplace_back(argv[i]);
    }
    if (agent_addrs.empty())
        agent_addrs.emplace_back("localhost:50051");

    // 创建并启动 AgentManager
    monitor::AgentManager mgr(agent_addrs);
    mgr.Start();

    std::cout << "Manager started. Press Ctrl+C to exit." << std::endl;
    // 主线程保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    return 0;
}
