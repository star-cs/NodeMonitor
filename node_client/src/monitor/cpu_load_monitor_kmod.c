#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/mm.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#    error "This module requires Linux kernel version 5.6 or later"
#endif

struct cpu_load {
    float load_avg_1;
    float load_avg_3;
    float load_avg_15;
};
static struct cpu_load *g_cpu_load = NULL;

static void update_cpu_load(struct cpu_load *info)
{
    info->load_avg_1 = (float)avenrun[0] / FIXED_1;
    info->load_avg_3 = (float)avenrun[1] / FIXED_1;
    info->load_avg_15 = (float)avenrun[2] / FIXED_1;
}

static int cpu_load_monitor_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = sizeof(struct cpu_load);
    if ((vma->vm_end - vma->vm_start) < size)
        return -EINVAL;

    // 移除 update_cpu_load 调用,因为定时器会定期更新
    return remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)g_cpu_load) >> PAGE_SHIFT, size,
                           vma->vm_page_prot);
}