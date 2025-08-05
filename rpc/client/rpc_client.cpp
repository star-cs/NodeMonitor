#include "rpc_client.h"
#include <google/protobuf/empty.pb.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>

using namespace monitor;

RpcClient::RpcClient(const std::string &server_address)
{
    //创建 gRPC 通道并初始化 Stub 对象
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    stub_ptr_ = GrpcManager::NewStub(channel);
}

RpcClient::~RpcClient()
{
}

void RpcClient::SetMonitorInfo(const MonitorInfo &monito_info)
{
    ClientContext context; //ClientContex用于管理一个 gRPC 调用的生命周期和状态。
    Empty response;

    Status status = stub_ptr_->SetMonitorInfo(&context, monito_info, &response);
    if (!status.ok()) {
        // 输出错误信息
        std::cout << status.error_details() << std::endl;
        std::cout << "status.error_message: " << status.error_message() << std::endl;
        std::cout << "falied to connect !!!" << std::endl;
    }
}

void RpcClient::GetMonitorInfo(MonitorInfo *monito_info)
{
    ClientContext context;
    Empty request;

    Status status = stub_ptr_->GetMonitorInfo(&context, request, monito_info);

    if (!status.ok()) {
        // 输出错误信息
        std::cout << status.error_details() << std::endl;
        std::cout << "status.error_message: " << status.error_message() << std::endl;
        std::cout << "falied to connect !!!" << std::endl;
    }
}