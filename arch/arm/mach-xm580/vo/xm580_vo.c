#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/rtc.h>
#include <linux/io.h>
#include <linux/time.h>

//NOTE: This file was orginally a kernel module (xm_vo.ko), but I added it here for simplicity.
void* reg_vo_base_va = NULL;

static int __init xmvo_init_module(void)
{
    printk("initilizing video output!\n");
    reg_vo_base_va = __arm_ioremap(0x31000000,0x90000,0);
    if(!reg_vo_base_va)
    {
        printk("ioremap video in base failed!\n");
        return -12;
    }
    //TODO
    //printk("writing registers\n");
    //init VO (ioctl 0x4f00)
    writel(1, (void*)(reg_vo_base_va + 0x80020));
    writel(4, (void*)(reg_vo_base_va + 0x80038));
    writel(4, (void*)(reg_vo_base_va + 0x80044));
    printk("successfully initialized VO device\n");
    return 0;
}

static void __exit xmvo_cleanup_module(void)
{
    iounmap(reg_vo_base_va);
}

module_init(xmvo_init_module);
module_exit(xmvo_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 RTC device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");