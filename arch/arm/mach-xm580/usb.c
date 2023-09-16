#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>

static struct resource xm_usb0_resource[] = {
	[0] = {
		.start = 0x50300000,
        .end = 0x503FFFFF,
        .name = "usb_addr",
        .flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 0x4C,
        .end = 0x4C,
        .name = "usb_irq",
        .flags = IORESOURCE_IRQ,
	},
};

static struct resource xm_usb1_resource[] = {
	[0] = {
		.start = 0x50400000,
        .end = 0x504FFFFF,
        .name = "usb_addr",
        .flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 0x5B,
        .end = 0x5B,
        .name = "usb_irq",
        .flags = IORESOURCE_IRQ,
	},
};


static struct platform_device xm_usb0_dev = 
{
    .name = "xm_usb",
    .id = 0,
    .dev = {
        .coherent_dma_mask = ~0,
        //.platform_data = &xm_sata0_platdata,
        //.release = xm_sata_release,
    },
    .resource = xm_usb0_resource,
    .num_resources = ARRAY_SIZE(xm_usb0_resource)
};

static struct platform_device xm_usb1_dev = 
{
    .name = "xm_usb",
    .id = 1,
    .dev = {
        .coherent_dma_mask = ~0,
        //.platform_data = &xm_sata0_platdata,
        //.release = xm_sata_release,
    },
    .resource = xm_usb1_resource,
    .num_resources = ARRAY_SIZE(xm_usb1_resource)
};

static int __init xmusb_init_module(void)
{
    platform_device_register(&xm_usb0_dev);
    platform_device_register(&xm_usb1_dev);
    return 0;
}

static void __exit xmusb_cleanup_module(void)
{

}

module_init(xmusb_init_module);
module_exit(xmusb_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 audio device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");