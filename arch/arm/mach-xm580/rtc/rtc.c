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

static u64 rtc_get(void)
{
    u64 result = 0;
    mutex_lock(&g_rtc_lock);
    result = *((u64*)0xfe0a008c);
    *((uint*)0xfe0a0080) = 2;
    while((readl((void*)0xfe0a0094) & 2) == 0)
    {

    }
    mutex_unlock(&g_rtc_lock);
    printk("rtc_get finish\n");

    return result;
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
    
}
static int __init xmrtc_init_module(void)
{
    u64 timestamp;
    int error = misc_register(&xm_rtc_dev);
    if (error) {
        printk("xm_rtc init error!\n");
        return error;
    }
    timestamp = rtc_get();
    printk("result: %llx\n", timestamp);
    if (timestamp > 946684800) //Make sure timestamp is after the year 2000
    {
        do_settimeofday(&timestamp);
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
    if (cmd_in == RTC_RD_TIME)
    {
        printk("RTC_RD_TIME is 0x%X!\n", cmd_in);
    }
    else if (cmd_in == RTC_SET_TIME)
    {
        printk("RTC_SET_TIME is 0x%X!\n", cmd_in);
    }
    printk("rtc_ioctl is no implement!\n");
    return -1;
}



/*
 * Exit function of our module.
 */
static void __exit xmrtc_cleanup_module(void)
{
    //del_timer(&g_timer);
    misc_deregister(&xm_rtc_dev);
}

module_init(xmrtc_init_module);
module_exit(xmrtc_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 RTC device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");