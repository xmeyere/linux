#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <linux/delay.h>
#include <linux/io.h>

static DEFINE_RAW_SPINLOCK(g_wdt_lock);

static long wdt_ioctl(struct file *file,uint cmd_in,unsigned long arg);
static int wdt_open(struct inode * node, struct file * file) { return 0; }
static int wdt_release(struct inode * node, struct file * file) { return 0; }

static const struct file_operations xm_wdt_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = wdt_ioctl,
    .open = wdt_open,
    .release = wdt_release
};

static struct miscdevice xm_wdt_dev = {
    .minor = WATCHDOG_MINOR,
    .name = "wdt",
    .fops = &xm_wdt_fops,
};

int wdt_start(void)
{
    unsigned long flags;
    raw_spin_lock_irqsave(&g_wdt_lock, flags);

    writel(0x1acce551, (void*)0xfe160c00);
    writel(0, (void*)0xfe160008);
    writel(0, (void*)0xfe16000c);
    writel(3, (void*)0xfe160008);
    writel(0, (void*)0xfe160c00);

    raw_spin_unlock_irqrestore(&g_wdt_lock, flags);

    return 0;
}

int wdt_stop(void)
{
    unsigned long flags;
    raw_spin_lock_irqsave(&g_wdt_lock, flags);

    writel(0x1acce551, (void*)0xfe160c00);
    writel(0, (void*)0xfe160008);
    writel(0, (void*)0xfe16000c);
    writel(0, (void*)0xfe160c00);

    raw_spin_unlock_irqrestore(&g_wdt_lock, flags);

    return 0;
}

int __init xm580_init_wdt(void)
{
    if(misc_register(&xm_wdt_dev))
    {
        printk("xm_wdt init error\n");
        return -1;
    }
    printk("xm_wdt init ok!\n");
    
    // It's probably better to implement the WDT propery, and figuring out how/when sofia calls WatchdogWrite(), but will this work?
    wdt_stop();
    return 0;
}

static long wdt_ioctl(struct file *file,uint cmd_in,unsigned long arg)
{
    return 0;
}
module_init(xm580_init_wdt);