#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/sched/loadavg.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/io.h>  

// 内核版本检查（要求 5.6+）
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#    error "This module requires Linux kernel version 5.6 or later"
#endif

struct cpu_load {
    float load_avg_1;
    float load_avg_3;
    float load_avg_15;
};

// 全局负载数据指针，记录此时的指标
static struct cpu_load *g_cpu_load = NULL;

// 更新 CPU 负载数据函数
static void update_cpu_load(struct cpu_load *info)
{   
    /* 
     * avenrun[] 是内核维护的固定点负载值数组：
     * [0] = 1分钟, [1] = 3分钟, [2] = 15分钟
     * FIXED_1 是内核定义的缩放因子 (1 << 11)
     * 转换为浮点数便于用户空间读取
     */
    info->load_avg_1 = (float)avenrun[0] / FIXED_1;
    info->load_avg_3 = (float)avenrun[1] / FIXED_1;
    info->load_avg_15 = (float)avenrun[2] / FIXED_1;
}

/** 
struct vm_area_struct (定义在 <linux/mm.h>) 描述 进程的虚拟内存区域

vm_start：区域起始地址
vm_end：区域结束地址
vm_page_prot：页面保护标志
*/ 
static int cpu_load_monitor_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = sizeof(struct cpu_load);

    // 检查请求映射大小是否足够
    if ((vma->vm_end - vma->vm_start) < size)
        return -EINVAL;


    /*
     * 将用户空间映射关联到内核分配的 g_cpu_load 内存
     * virt_to_phys: 虚拟地址转物理地址
     * remap_pfn_range: 建立物理页到用户空间的映射
     * 注意：此处未调用 update_cpu_load，由定时器定期更新
     */
    return remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)g_cpu_load) >> PAGE_SHIFT, size,
                           vma->vm_page_prot);
}

// 文件操作结构体
/**
struct file_operations (定义在 <linux/fs.h>)  定义文件操作函数指针集合

    .mmap：指向内存映射处理函数
    .owner：指向拥有该结构的模块
*/
static const struct file_operations cpu_load_monitor_fops = {
    .owner = THIS_MODULE,   // 所属模块
    .mmap = cpu_load_monitor_mmap,  // 注册mmap处理函数
};

// 杂项设备定义
/**
struct miscdevice (定义在 <linux/miscdevice.h>)

.minor：次设备号（MISC_DYNAMIC_MINOR 表示动态分配）
.name：设备名称（出现在 /dev/ 下）
.fops：关联的文件操作集
*/
static struct miscdevice cpu_load_monitor_dev = {
    .minor = MISC_DYNAMIC_MINOR,  // 动态分配次设备号
    .name = "cpu_load_monitor",    // 设备名称（/dev/cpu_load_monitor）
    .fops = &cpu_load_monitor_fops, // 关联文件操作
    .mode = 0444,                  // 只读权限
};


// 定时器相关变量
static struct hrtimer cpu_load_timer;  // 高精度定时器
static ktime_t ktime;                  // 时间间隔
#define UPDATE_INTERVAL_NS 1000000000L // 1秒更新间隔（纳秒）

// 定时器回调函数
/**
struct hrtimer (定义在 <linux/hrtimer.h>)

.function：定时器到期时的回调函数
*/
static enum hrtimer_restart cpu_load_timer_callback(struct hrtimer *timer)
{
    // 定期更新负载数据
    update_cpu_load(g_cpu_load);
    
    // 重置定时器（相对当前时间前进1秒）
    hrtimer_forward_now(timer, ktime);
    
    return HRTIMER_RESTART;  // 继续定时器运行
}

// 模块初始化函数
static int __init cpu_load_monitor_init(void) {
    // 分配非连续物理内存（适合mmap）
    g_cpu_load = vzalloc(sizeof(struct cpu_load));
    if (!g_cpu_load)
        return -ENOMEM;  // 内存分配失败
    
    // 初始化定时器
    ktime = ktime_set(0, UPDATE_INTERVAL_NS);  // 设置1秒间隔
    hrtimer_init(&cpu_load_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL); // 单调时钟模式
    cpu_load_timer.function = &cpu_load_timer_callback;  // 绑定回调函数
    hrtimer_start(&cpu_load_timer, ktime, HRTIMER_MODE_REL); // 启动定时器
    
    // 注册杂项设备
    misc_register(&cpu_load_monitor_dev);
    printk(KERN_INFO "cpu_load_monitor device registered\n");
    return 0;
}


// 模块退出函数
static void __exit cpu_load_monitor_exit(void) {
    // 停止定时器
    hrtimer_cancel(&cpu_load_timer);
    
    // 注销设备
    misc_deregister(&cpu_load_monitor_dev);
    
    // 释放内存
    if (g_cpu_load)
        vfree(g_cpu_load);
    
    printk(KERN_INFO "cpu_load_monitor device unregistered\n");
}


// 注册模块入口/出口
module_init(cpu_load_monitor_init)
module_exit(cpu_load_monitor_exit)

// 模块元信息
MODULE_LICENSE("GPL")                      // GPL许可
MODULE_AUTHOR("star-cs")                  // 作者
MODULE_DESCRIPTION("CPU Load Monitor Module with mmap support") // 描述