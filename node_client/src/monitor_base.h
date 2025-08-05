#pragma once
#include "rpc/client/rpc_client.h"
#include "monitor_info.grpc.pb.h"

namespace monitor
{
using monitor::proto::MonitorInfo;

class MonitorBase
{
public:
    MonitorBase() {}
    virtual ~MonitorBase() {}
    virtual void UpdateOnce(MonitorInfo *monitor_info) = 0;
    virtual void Stop() = 0;
};

} // namespace monitor