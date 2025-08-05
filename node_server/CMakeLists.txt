set(SERVER_NAME node_server)

set(SOURCES
    main.cpp
    agent_manager.cpp
)

add_executable(${SERVER_NAME} ${SOURCES})

target_include_directories(${SERVER_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../proto
)

target_link_libraries(${SERVER_NAME}
    PUBLIC
    monitor_proto
    rpc_client
    Threads::Threads
)