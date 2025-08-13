#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/softirq.h>
#include <linux/hrtimer.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#error "This module requires Linux kernel version 5.6 or later"
#endif

#define MAX_CPU 128

struct softirq_stat {
    char cpu_name[16];
    unsigned long hi;
    unsigned long timer;
    unsigned long net_tx;
    unsigned long net_rx;
    unsigned long block;
    unsigned long irq_poll;
    unsigned long tasklet;
    unsigned long sched;
    unsigned long hrtimer;
    unsigned long rcu;
};

static struct softirq_stat *g_softirq_stats = NULL;
static struct hrtimer softirq_timer;
static ktime_t ktime;
#define UPDATE_INTERVAL_NS 1000000000L  // 1秒 = 1000000000 纳秒

static void update_softirq_stats(struct softirq_stat *stats) {
    int cpu;
    for (cpu = 0; cpu < MAX_CPU; ++cpu) {
        if (!cpu_online(cpu)) {
            stats[cpu].cpu_name[0] = '\0';
            continue;
        }
        snprintf(stats[cpu].cpu_name, sizeof(stats[cpu].cpu_name), "cpu%d", cpu);
        stats[cpu].hi = kstat_softirqs_cpu(HI_SOFTIRQ, cpu);
        stats[cpu].timer = kstat_softirqs_cpu(TIMER_SOFTIRQ, cpu);
        stats[cpu].net_tx = kstat_softirqs_cpu(NET_TX_SOFTIRQ, cpu);
        stats[cpu].net_rx = kstat_softirqs_cpu(NET_RX_SOFTIRQ, cpu);
        stats[cpu].block = kstat_softirqs_cpu(BLOCK_SOFTIRQ, cpu);
        stats[cpu].irq_poll = kstat_softirqs_cpu(IRQ_POLL_SOFTIRQ, cpu);
        stats[cpu].tasklet = kstat_softirqs_cpu(TASKLET_SOFTIRQ, cpu);
        stats[cpu].sched = kstat_softirqs_cpu(SCHED_SOFTIRQ, cpu);
        stats[cpu].hrtimer = kstat_softirqs_cpu(HRTIMER_SOFTIRQ, cpu);
        stats[cpu].rcu = kstat_softirqs_cpu(RCU_SOFTIRQ, cpu);
    }
}   

static enum hrtimer_restart softirq_timer_callback(struct hrtimer *timer)
{
    update_softirq_stats(g_softirq_stats);
    hrtimer_forward_now(timer, ktime);
    return HRTIMER_RESTART;
}

static int cpu_softirq_monitor_mmap(struct file *filp, struct vm_area_struct *vma) {
    unsigned long size = sizeof(struct softirq_stat) * MAX_CPU;
    if ((vma->vm_end - vma->vm_start) < size)
        return -EINVAL;

    return remap_pfn_range(vma, vma->vm_start,
                           virt_to_phys((void *)g_softirq_stats) >> PAGE_SHIFT,
                           size, vma->vm_page_prot);
}

static const struct file_operations cpu_softirq_monitor_fops = {
    .owner = THIS_MODULE,
    .mmap = cpu_softirq_monitor_mmap,
};

static struct miscdevice cpu_softirq_monitor_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "cpu_softirq_monitor",
    .fops = &cpu_softirq_monitor_fops,
    .mode = 0444,
};

static int __init cpu_softirq_monitor_init(void) {
    g_softirq_stats = vzalloc(sizeof(struct softirq_stat) * MAX_CPU);
    if (!g_softirq_stats)
        return -ENOMEM;
    
    // 初始化并启动定时器
    ktime = ktime_set(0, UPDATE_INTERVAL_NS);
    hrtimer_init(&softirq_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    softirq_timer.function = &softirq_timer_callback;
    hrtimer_start(&softirq_timer, ktime, HRTIMER_MODE_REL);
    
    misc_register(&cpu_softirq_monitor_dev);
    printk(KERN_INFO "cpu_softirq_monitor device registered\n");
    return 0;
}

static void __exit cpu_softirq_monitor_exit(void) {
    hrtimer_cancel(&softirq_timer);
    misc_deregister(&cpu_softirq_monitor_dev);
    if (g_softirq_stats)
        vfree(g_softirq_stats);
    printk(KERN_INFO "cpu_softirq_monitor device unregistered\n");
}

module_init(cpu_softirq_monitor_init);
module_exit(cpu_softirq_monitor_exit);

MODULE_LICENSE("GPL")
MODULE_AUTHOR("star-cs")
MODULE_DESCRIPTION("CPU SoftIRQ Monitor Module with mmap support")