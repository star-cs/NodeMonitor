#pragma once

#ifdef __KERNEL__
#    include <linux/types.h>
#else
#    include <stdint.h>
#endif

// 保证结构体字节对齐一致
#define MONITOR_PACKED __attribute__((packed))

// 内存信息
struct mem_info {
    uint64_t total;
    uint64_t free;
    uint64_t available;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swap_cached;
    uint64_t active;
    uint64_t inactive;
    uint64_t active_anon;
    uint64_t inactive_anon;
    uint64_t active_file;
    uint64_t inactive_file;
    uint64_t dirty;
    uint64_t writeback;
    uint64_t anon_pages;
    uint64_t mapped;
    uint64_t k_reclaimable;
    uint64_t s_reclaimable;
    uint64_t s_unreclaim;
} MONITOR_PACKED;

// CPU统计
struct cpu_stat {
    char cpu_name[16];
    float user;
    float system;
    float idle;
    float nice;
    float io_wait;
    float irq;
    float soft_irq;
    float steal;
    float guest;
    float guest_nice;
} MONITOR_PACKED;

// CPU负载
struct cpu_load {
    float load_avg_1;
    float load_avg_3;
    float load_avg_15;
} MONITOR_PACKED;

// 软中断统计
struct softirq_stat {
    char cpu_name[16];
    uint64_t hi;
    uint64_t timer;
    uint64_t net_tx;
    uint64_t net_rx;
    uint64_t block;
    uint64_t irq_poll;
    uint64_t tasklet;
    uint64_t sched;
    uint64_t hrtimer;
    uint64_t rcu;
} MONITOR_PACKED;

// 网络统计
struct net_stat {
    char name[32];
    uint64_t rcv_bytes;
    uint64_t rcv_packets;
    uint64_t err_in;
    uint64_t drop_in;
    uint64_t snd_bytes;
    uint64_t snd_packets;
    uint64_t err_out;
    uint64_t drop_out;
} MONITOR_PACKED;

#undef MONITOR_PACKED
