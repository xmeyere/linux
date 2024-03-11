#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <linux/delay.h>
#include <linux/io.h>
#include "wdt.h"

//NOTE: This file was orginally a kernel module (xm_rtc.ko), but I added it here for simplicity.

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

    writel(WATCHDOG_REGISTER_UNLOCK_VALUE, WATCHDOG_REGISTER_UNLOCK);

    writel(WATCHDOG_STATE_OFF, WATCHDOG_REGISTER_STATE);
    writel(0, WATCHDOG_REGISTER_TIMELEFT);
    writel(WATCHDOG_STATE_ON, WATCHDOG_REGISTER_STATE);

    writel(0, WATCHDOG_REGISTER_UNLOCK);

    raw_spin_unlock_irqrestore(&g_wdt_lock, flags);

    return 0;
}

int wdt_stop(void)
{
    unsigned long flags;
    raw_spin_lock_irqsave(&g_wdt_lock, flags);

    writel(WATCHDOG_REGISTER_UNLOCK_VALUE, WATCHDOG_REGISTER_UNLOCK);

    writel(WATCHDOG_STATE_OFF, WATCHDOG_REGISTER_STATE);
    writel(0, WATCHDOG_REGISTER_TIMELEFT);

    writel(0, WATCHDOG_REGISTER_UNLOCK);

    raw_spin_unlock_irqrestore(&g_wdt_lock, flags);

    return 0;
}

int wdt_set_timeout(int seconds)
{
    unsigned long flags;
    raw_spin_lock_irqsave(&g_wdt_lock, flags);

    writel(WATCHDOG_REGISTER_UNLOCK_VALUE, WATCHDOG_REGISTER_UNLOCK);
    writel(seconds * 93750 >> 1, (void*)0xfe160000); //TODO: is the size of the write correct?
    writel(0, WATCHDOG_REGISTER_UNLOCK);

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
    
    //TODO: It's probably better to implement the WDT propery, and figuring out how/when sofia calls WatchdogWrite(), but will this work?
    //Note: U-Boot enables the watchdog, so we will disable it here.
    wdt_stop();
    return 0;
}

static long wdt_ioctl(struct file *file,uint cmd_in,unsigned long arg)
{
    if(cmd_in == WATCHDOG_IOCTL_WRITE)
    {
        unsigned long flags;
        raw_spin_lock_irqsave(&g_wdt_lock, flags);

        writel(WATCHDOG_REGISTER_UNLOCK_VALUE, WATCHDOG_REGISTER_UNLOCK);
        writel(0, WATCHDOG_REGISTER_TIMELEFT);
        writel(0, WATCHDOG_REGISTER_UNLOCK);

        raw_spin_unlock_irqrestore(&g_wdt_lock, flags);
        return 0;
    }
    else if(cmd_in == WATCHDOG_IOCTL_START)
    {
        wdt_start();
    }
    else if(cmd_in == WATCHDOG_IOCTL_STOP)
    {
        wdt_stop();
    }
    else if(cmd_in == WATCHDOG_IOCTL_START)
    {
        wdt_set_timeout(*(__user int*)arg);
    }
    printk("wdt_ioctl: unknown command %d", cmd_in);
    return 0;
}
module_init(xm580_init_wdt);