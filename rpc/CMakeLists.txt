set(SOURCES
    client/rpc_client.cpp
)

add_library(rpc_client  ${SOURCES})

target_link_libraries(rpc_client
    PUBLIC
    monitor_proto
)

set(SOURCES2
    server/rpc_server.cpp
)

add_library(rpc_server  ${SOURCES2})

target_link_libraries(rpc_server
    PUBLIC
    monitor_proto
)