set(SERVER_NAME node_client)

file(GLOB SOURCES main.cpp src/monitor/*.hpp src/moniotr/*.cpp)

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

set(KERNEL_MODULES
    cpu_load_monitor_kmod   
    cpu_softirq_monitor_kmod
    cpu_stat_monitor_kmod
)

execute_process(
  COMMAND uname -r
  OUTPUT_VARIABLE KERNEL_RELEASE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(KERNEL_HEADERS_DIR "/usr/src/linux-headers-${KERNEL_RELEASE}")
# 检查路径是否存在
if(NOT EXISTS ${KERNEL_HEADERS_DIR})
  message(FATAL_ERROR "内核头文件未找到：${KERNEL_HEADERS_DIR}")
endif()
include_directories(
  ${KERNEL_HEADERS_DIR}/include                   # 通用头文件
  ${KERNEL_HEADERS_DIR}/arch/x86/include          # 架构相关头文件（根据实际架构调整，如 arm、x86_64）
  ${KERNEL_HEADERS_DIR}/include/uapi              # 用户空间 API 头文件
  ${KERNEL_HEADERS_DIR}/include/generated         # 生成的配置头文件
)

# 定义内核模块的构建目标
set(MODULE_TARGETS)
foreach(MODULE ${KERNEL_MODULES})
    # 编译目标（已存在）
    add_custom_target(${MODULE}_module
        COMMAND make -C /lib/modules/${KERNEL_RELEASE}/build M=${CMAKE_CURRENT_SOURCE_DIR} src/monitor/${MODULE}.ko
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building kernel module: ${MODULE}"
    )
    # 新增：清理目标
    add_custom_target(clean_${MODULE}_module
        COMMAND make -C /lib/modules/${KERNEL_RELEASE}/build M=${CMAKE_CURRENT_SOURCE_DIR} src/monitor/${MODULE}.ko clean
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Cleaning kernel module: ${MODULE}"
    )
    list(APPEND MODULE_TARGETS ${MODULE}_module)
endforeach()


# 定义所有内核模块的构建目标
add_custom_target(modules ALL DEPENDS ${MODULE_TARGETS})