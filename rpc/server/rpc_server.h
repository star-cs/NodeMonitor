#pragma once

#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"
#include <google/protobuf/empty.pb.h>
#include <mutex>

using grpc::ServerContext;
using grpc::Status;

using monitor::proto::MonitorInfo;
using monitor::proto::GrpcManager;
using google::protobuf::Empty;

namespace monitor
{
class GrpcManagerImpl : public GrpcManager::Service
{
public:
    GrpcManagerImpl();
    ~GrpcManagerImpl();

    Status SetMonitorInfo(grpc::ServerContext *context, const MonitorInfo *request,
                          Empty *response) override;
    Status GetMonitorInfo(grpc::ServerContext *context, const Empty *request,
                          MonitorInfo *response) override;

private:
    std::unordered_map<std::string, MonitorInfo> monitor_infos_map_;
    std::mutex mtx_;
};
} // namespace monitor