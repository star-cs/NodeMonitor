#pragma once
#include "rpc/client/rpc_client.h"
#include <thread>

namespace monitor
{
struct AgentScore {
    MonitorInfo info;
    double score;
    std::chrono::system_clock::time_point timestamp;
};

class AgentManager
{
public:
    AgentManager(const std::vector<std::string> &agent_addrs);
    ~AgentManager();
    void Start();

private:
    void FetchAndScoreLoop();
    double CalcScore(const MonitorInfo &info);
    void WriteToMysql(const std::string &server_name, const AgentScore &agent_score,
                      double net_in_rate, double net_out_rate, float cpu_percent_rate,
                      float usr_percent_rate, float system_percent_rate, float nice_percent_rate,
                      float idle_percent_rate, float io_wait_percent_rate, float irq_percent_rate,
                      float soft_irq_percent_rate, float steal_percent_rate,
                      float guest_percent_rate, float guest_nice_percent_rate,
                      float load_avg_1_rate, float load_avg_3_rate, float load_avg_15_rate,
                      float mem_used_percent_rate, float mem_total_rate, float mem_free_rate,
                      float mem_avail_rate, float net_in_rate_rate, float net_out_rate_rate,
                      float net_in_peak_rate, float net_out_peak_rate, float net_in_drop_rate_rate,
                      float net_out_drop_rate_rate, float score_rate);

    std::vector<std::string> agent_addrs_;
    std::vector<std::unique_ptr<RpcClient>> rpc_clients_;
    std::unordered_map<std::string, AgentScore> agent_scores_;
    std::mutex mtx_;
    bool running_;
    std::unique_ptr<std::thread> thread_;
};

} // namespace monitor