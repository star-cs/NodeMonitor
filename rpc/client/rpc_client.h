#pragma once

#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"
#include <grpcpp/channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/client_context.h>
#include <grpcpp/impl/codegen/status.h>

namespace monitor
{
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using monitor::proto::GrpcManager;
using monitor::proto::MonitorInfo;
using google::protobuf::Empty;

class RpcClient
{
public:
    RpcClient(const std::string &server_address = "localhost:50051");
    ~RpcClient();
    void SetMonitorInfo(const MonitorInfo &monito_info);
    void GetMonitorInfo(MonitorInfo *monito_info);

    // 指向 gRPC 服务的 Stub 对象，用于调用远程方法。
private:
    std::unique_ptr<GrpcManager::Stub> stub_ptr_;
};

} // namespace monitor