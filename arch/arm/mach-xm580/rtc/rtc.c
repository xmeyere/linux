#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/rtc.h>
#include <linux/io.h>
#include <linux/time.h>

//NOTE: This file was orginally a kernel module (xm_rtc.ko), but I added it here for simplicity.
static DEFINE_MUTEX(g_rtc_lock);

static long rtc_ioctl(struct file *file,uint cmd_in,unsigned long arg);
static int rtc_open(struct inode * node, struct file * file) { return 0; }
static int rtc_release(struct inode * node, struct file * file) { return 0; }

static const struct file_operations xm_rtc_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = rtc_ioctl,
    .open = rtc_open,
    .release = rtc_release
};

static struct miscdevice xm_rtc_dev = {
    .minor = RTC_MINOR,
    .name = "rtc",
    .fops = &xm_rtc_fops,
};

static struct timer_list g_timer;
static u64 last_tick;

static u64 rtc_get(void)
{
    u32 result1 = 0;
    u32 result2 = 0;
    u64 finalResult = 0;
    mutex_lock(&g_rtc_lock);
    writel(2, (void*)0xfe0a0080);
    //wait for the data to be ready
    while((readl((void*)0xfe0a0094) & 2) == 0)
    {

    }
    result1 = readl((void*)0xfe0a008c);
    result2 = readl((void*)0xfe0a0090);

    mutex_unlock(&g_rtc_lock);
    finalResult = (((long long) (result2) << 32) | (u32)(result1));
    return finalResult >> 15;
}

static void rtc_adjust(unsigned long l)
{
    g_timer.expires = jiffies + 60000;
    g_timer.function = rtc_adjust;
    add_timer(&g_timer);
    mutex_lock(&g_rtc_lock);

    writel(2, (void*)0xfe0a0080);
    while((readl((void*)0xfe0a0094) & 2) == 0)
    {

    }

    //TODO
    //if (last_tick != )

    mutex_unlock(&g_rtc_lock);
}
static int __init xmrtc_init_module(void)
{
    u64 timestamp;
    struct timespec ts;
    int error = misc_register(&xm_rtc_dev);
    if (error) {
        printk("xm_rtc init error!\n");
        return error;
    }
    timestamp = rtc_get();
    printk("rtc result: %lld\n", timestamp);

    if (timestamp > 946684800) //Make sure timestamp is after the year 2000
    {   
        ts.tv_sec = timestamp;
        ts.tv_nsec = 0;
        if(do_settimeofday(&ts))
        {
            printk("do_settimeofday() failed\n");
        }
    }
    init_timer_key(&g_timer, 0,0,0);
    g_timer.function = rtc_adjust;
    g_timer.expires = jiffies + 60000;
    g_timer.data = 0;
    add_timer(&g_timer);
    printk("xm_rtc init is ok!\n");
    return 0;
}

static long rtc_ioctl(struct file *file,uint cmd_in,unsigned long arg)
{
    u64 rtctime;
    u64 rtctimeraw;
    unsigned long time;
    struct rtc_time time_struct;
    uint* ptr;
    if (cmd_in == RTC_SET_TIME)
    {
        printk("RTC_SET_TIME is 0x%X!\n", cmd_in);
        if (copy_from_user(&time_struct, (void __user *)arg, sizeof(time_struct)))
			return -EFAULT;
        rtc_tm_to_time(&time_struct, &time);
        mutex_lock(&g_rtc_lock);
        rtctimeraw = (u64)time << 15;
        ptr = ((uint*)(void*)&rtctimeraw);
        printk("setting time to %d with raw value %d. ptr[0]: %d ptr[1]: %d\n", time, rtctimeraw, ptr[0],ptr[1]);
        writel(rtctimeraw&0xFFFFFFFF, (void*)0xfe0a0084);
        writel(((rtctimeraw >> 32)&0xFFFFFFFF), (void*)0xfe0a0088);

        writel(1, (void*)0xfe0a0080);
        //wait for the write to complete
        while((readl((void*)0xfe0a0094) & 1) == 0) {}
        mutex_unlock(&g_rtc_lock);
        return 0;
    }
    if (cmd_in == RTC_RD_TIME)
    {
        printk("RTC_RD_TIME is 0x%X!\n", cmd_in);
        rtctime = rtc_get();
        rtc_time_to_tm(rtctime, &time_struct);
        //todo: there is more stuff here
        return copy_to_user((void __user *)arg, &time_struct, sizeof(struct rtc_time));
    }
    printk("rtc_ioctl: unknown iocctl command\n");
    return -1;
}

static void __exit xmrtc_cleanup_module(void)
{
    del_timer(&g_timer);
    misc_deregister(&xm_rtc_dev);
}

module_init(xmrtc_init_module);
module_exit(xmrtc_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 RTC device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");