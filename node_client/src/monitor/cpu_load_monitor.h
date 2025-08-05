#pragma once
#include "node_client/src/monitor_base.h"

namespace monitor
{
class CpuLoadMonitor : public MonitorBase
{
public:
    CpuLoadMonitor() {}
    void UpdateOnce(monitor::proto::MonitorInfo *monitor_info) override;
    void Stop() override {}

private:
    float load_avg_1_;
    float load_avg_3_;
    float load_avg_15_;
};
} // namespace monitor