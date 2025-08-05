set(PROTO_FILES
    monitor_info.proto
    cpu_load.proto
    cpu_softirq.proto
    cpu_stat.proto
    mem_info.proto
    net_info.proto
    disk_info.proto
)

add_library(monitor_proto ${PROTO_FILES})    # 生成 monitor_proto 静态库
target_link_libraries(monitor_proto
    PUBLIC
    protobuf::libprotobuf
    gRPC::grpc
    gRPC::grpc++
    Threads::Threads
)
target_include_directories(monitor_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}       
    ${CMAKE_CURRENT_SOURCE_DIR}
)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)    # 获取 gRPC 插件路径
protobuf_generate(TARGET monitor_proto LANGUAGE cpp)   # 生成 Protobuf 序列化代码
# 生成 gRPC 服务代码
protobuf_generate(TARGET monitor_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")