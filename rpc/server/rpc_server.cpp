#include "rpc_server.h"
#include <mutex>

using namespace monitor;

GrpcManagerImpl::GrpcManagerImpl()
{
}

GrpcManagerImpl::~GrpcManagerImpl()
{
}

Status GrpcManagerImpl::SetMonitorInfo(ServerContext *context, const MonitorInfo *request,
                                       Empty *response)
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::string username = request->name();
    monitor_infos_map_[username] = *request;
    return Status::OK;
}

Status GrpcManagerImpl::GetMonitorInfo(ServerContext *context, const Empty *request,
                                       MonitorInfo *response)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (!monitor_infos_map_.empty()) {
        *response = monitor_infos_map_.begin()->second;
    }
    return Status::OK;
}
