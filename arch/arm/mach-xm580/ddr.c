#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/rtc.h>
#include <linux/io.h>
#include <linux/time.h>
#include <mach/hardware.h>
u32 ddr_byte[8];
//NOTE: This file was orginally a kernel module (xm_ddr.ko), but I added it here for simplicity.

static irqreturn_t timer7_interrupt(int irq, void *data)
{
    void* ptr;
    int i = 0;

    //check if it was from this device
    if(readl((void*)0xfe0d2034) == 0)
    {
        return IRQ_NONE;
    }
    //printk("timer7 irq!\n");

    writel(1, (void*)0xfe0d202c);
    writel(0, (void*)0xfe0b2000);
    // TODO
    ptr = (u8*)0xfe0b2010;
    while (ptr != 0xfe0b2030)
    {
        ddr_byte[i] = readl(ptr);
        i++;
        ptr+=4;
    }
    //printk("ddr_byte:\n[0]: %x\n[1]: %x\n", ddr_byte[0], ddr_byte[1]);
    //this reads 32 bytes (8 uints) from 0xfe0b2010 to 0xfe0b2030
    // End TODO
    writel(1, (void*)0xfe0b2000);
    return IRQ_HANDLED;
}
static int __init xmddr_init_module(void)
{
    int result = 0;
    printk("initilizing timer!\n");
    writel(0x62, (void*)0xfe0d2028);
    writel(12000000, (void*)0xfe0d2020);
    writel(12000000, (void*)0xfe0d2038);
    writel(readl((void*)0xfe0d2028) | 0x80, (void*)0xfe0d2028);
    printk("successfully initialized VO device\n");

    result = request_threaded_irq(0x5f, timer7_interrupt, 0, 0x20, "timer7", NULL);
    if (result < 0) {
        printk("request irq %d failed\n", 0x25);
        result = -1;
    }
    return result;
}

static void __exit xmddr_cleanup_module(void)
{
    
}

module_init(xmddr_init_module);
module_exit(xmddr_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 timer tow work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");