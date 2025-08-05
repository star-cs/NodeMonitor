# NodeMonitor
高性能系统监控解决方案，采用C++开发，基于CMake构建系统，集成Protocol Buffers和gRPC实现高效数据序列化与远程过程调用。本项目创新性地结合了eBPF和mmap技术，实现了高性能的系统监控数据采集：通过eBPF技术进行网络流量和系统事件的高效捕获与分析，通过mmap机制实现内核与用户空间的数据高效共享，大幅提升监控性能。  

项目专注于节点级系统监控，能够实时采集CPU（负载、软中断、状态）、内存、网络、磁盘等关键性能指标，并通过客户端-服务器架构提供灵活的数据访问接口。项目包含监控数据展示组件、RPC通信管理模块及测试框架，支持Docker容器化部署，适用于高性能计算环境和分布式系统监控场景。


# 快速开始
```bash
uname -a #查看linux版本

sudo apt install libelf-dev build-essential pkg-config
sudo apt install bison build-essential flex libssl-dev libelf-dev bc

wget https://github.com/microsoft/WSL2-Linux-Kernel/archive/refs/tags/linux-msft-wsl-6.6.87.2.tar.gz
tar -zvxf linux-msft-wsl-6.6.87.2.tar.gz
cd linux-msft-wsl-6.6.87.2
zcat /proc/config.gz > .config
make -j $(nproc) 
sudo make -j $(nproc) modules_install

```