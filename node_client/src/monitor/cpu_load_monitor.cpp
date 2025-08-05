#include "node_client/src/monitor/cpu_load_monitor.h"
#include "node_client/src/monitor_struct.h"
#include <sys/mman.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

using namespace monitor;

void CpuLoadMonitor::UpdateOnce(monitor::proto::MonitorInfo *monitor_info)
{
    int fd = open("/dev/cpu_load_monitor", O_RDONLY);
    if (fd < 0)
        return;

    size_t load_size = sizeof(struct cpu_load);
    /**
    addr        指定映射区域的起始地址 传入 nullptr 表示让系统自动选择合适的内存地址
    load_size   映射区域的大小
    PROT_READ   映射区域可读
    MAP_SHARED  指定映射的类型和可见性 MAP_SHARED 表示对映射区域的修改对其他映射同一对象的进程可见
    fd          被映射的文件的描述符
    offset      被映射文件的起始偏移量，必须是页大小的倍数。0 表示从文件开头开始映射
    */
    void *addr = mmap(nullptr, load_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return;
    }
    struct cpu_load info;
    memcpy(&info, addr, load_size);

    auto cpu_load_msg = monitor_info->mutable_cpu_load();
    cpu_load_msg->set_load_avg_1(info.load_avg_1);
    cpu_load_msg->set_load_avg_3(info.load_avg_3);
    cpu_load_msg->set_load_avg_15(info.load_avg_15);

    munmap(addr, load_size);
    close(fd);
}