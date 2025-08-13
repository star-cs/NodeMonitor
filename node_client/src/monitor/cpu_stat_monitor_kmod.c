#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/kstat.h>
#include <linux/hrtimer.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#error "This module requires Linux kernel version 5.6 or later"
#endif

#define MAX_CPU 128

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
};

static struct cpu_stat *g_cpu_stats = NULL;
static struct hrtimer cpu_stat_timer;
static ktime_t ktime;
#define UPDATE_INTERVAL_NS 1000000000L  // 1秒 = 1000000000 纳秒

static void update_cpu_stats(struct cpu_stat *stats) {
    int cpu;
    for (cpu = 0; cpu < MAX_CPU; ++cpu) {
        if (!cpu_online(cpu)) {
            stats[cpu].cpu_name[0] = '\0';
            continue;
        }
        snprintf(stats[cpu].cpu_name, sizeof(stats[cpu].cpu_name), "cpu%d", cpu);
        // 从系统启动依赖累计的时间
        u64 *stat = kcpustat_cpu(cpu).cpustat;
        stats[cpu].user = (float)stat[CPUTIME_USER]; // 用户态 CPU 时间
        stats[cpu].nice = (float)stat[CPUTIME_NICE]; // 低优先级用户态 CPU 时间
        stats[cpu].system = (float)stat[CPUTIME_SYSTEM]; // 内核态 CPU 时间
        stats[cpu].idle = (float)stat[CPUTIME_IDLE];  // CPU 空闲时间
        stats[cpu].io_wait = (float)stat[CPUTIME_IOWAIT]; // CPU 等待 I/O 操作的时间
        stats[cpu].irq = (float)stat[CPUTIME_IRQ];    // 处理硬中断的 CPU 时间
        stats[cpu].soft_irq = (float)stat[CPUTIME_SOFTIRQ];  // 处理软中断的 CPU 时间
        stats[cpu].steal = (float)stat[CPUTIME_STEAL];   // 被虚拟化技术 "偷走" 的 CPU 时间
        stats[cpu].guest = (float)stat[CPUTIME_GUEST];  // 运行虚拟机的 CPU 时间
        stats[cpu].guest_nice = (float)stat[CPUTIME_GUEST_NICE];  // 运行低优先级虚拟机的 CPU 时间
    }
}

static enum hrtimer_restart cpu_stat_timer_callback(struct hrtimer *timer)
{
    update_cpu_stats(g_cpu_stats);
    hrtimer_forward_now(timer, ktime);
    return HRTIMER_RESTART;
}

static int cpu_stat_monitor_mmap(struct file *filp, struct vm_area_struct *vma) {
    unsigned long size = sizeof(struct cpu_stat) * MAX_CPU;
    if ((vma->vm_end - vma->vm_start) < size)
        return -EINVAL;
    
    return remap_pfn_range(vma, vma->vm_start,
                           virt_to_phys((void *)g_cpu_stats) >> PAGE_SHIFT,
                           size, vma->vm_page_prot);
}

static const struct file_operations cpu_stat_monitor_fops = {
    .owner = THIS_MODULE,
    .mmap = cpu_stat_monitor_mmap,
};

static struct miscdevice cpu_stat_monitor_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "cpu_stat_monitor",
    .fops = &cpu_stat_monitor_fops,
    .mode = 0444,
};

static int __init cpu_stat_monitor_init(void) {
    g_cpu_stats = vzalloc(sizeof(struct cpu_stat) * MAX_CPU);
    if (!g_cpu_stats)
        return -ENOMEM;
    
    // 初始化并启动定时器
    ktime = ktime_set(0, UPDATE_INTERVAL_NS);
    hrtimer_init(&cpu_stat_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    cpu_stat_timer.function = &cpu_stat_timer_callback;
    hrtimer_start(&cpu_stat_timer, ktime, HRTIMER_MODE_REL);
    
    misc_register(&cpu_stat_monitor_dev);
    printk(KERN_INFO "cpu_stat_monitor device registered\n");
    return 0;
}

static void __exit cpu_stat_monitor_exit(void) {
    hrtimer_cancel(&cpu_stat_timer);
    misc_deregister(&cpu_stat_monitor_dev);
    if (g_cpu_stats)
        vfree(g_cpu_stats);
    printk(KERN_INFO "cpu_stat_monitor device unregistered\n");
}

module_init(cpu_stat_monitor_init);
module_exit(cpu_stat_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("star-cs");
MODULE_DESCRIPTION("CPU Stat Monitor Module with mmap support");