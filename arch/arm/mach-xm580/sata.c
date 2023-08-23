#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <linux/delay.h>
#include <linux/io.h>

//NOTE: this file orginally was at the drivers/ata folder (I think), but it works just as well here. Note that 3 modifications are needed for SATA to work in libata-core.c

static __init int xm_sata0_init(struct device *dev, void *addr);
static __init int xm_sata1_init(struct device *dev, void *addr);
static __init void xm_sata_release(struct device *dev)
{
}

static struct ahci_platform_data xm_sata0_platdata = 
{
    .init = xm_sata0_init
};
static struct ahci_platform_data xm_sata1_platdata = 
{
    .init = xm_sata1_init
};

static struct resource xm_sata0_resource[] = {
	[0] = {
		.start = 0x50500000,
        .end = 0x505FFFFF,
        .name = "sata_addr",
        .flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 0x5D,
        .end = 0x5D,
        .name = "sata_irq",
        .flags = IORESOURCE_IRQ,
	},
};

static struct resource xm_sata1_resource[] = {
	[0] = {
		.start = 0x50600000,
        .end = 0x506FFFFF,
        .name = "sata_addr",
        .flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = 0x5E,
        .end = 0x5E,
        .name = "sata_irq",
        .flags = IORESOURCE_IRQ,
	},
};
static struct platform_device xm_ahci0 = 
{
    .name = "ahci",
    .id = 0,
    .dev = {
        .coherent_dma_mask = ~0,
        .platform_data = &xm_sata0_platdata,
        .release = xm_sata_release,
    },
    .resource = xm_sata0_resource,
    .num_resources = ARRAY_SIZE(xm_sata0_resource)
};
static struct platform_device xm_ahci1 = 
{
    .name = "ahci",
    .id = 1,
    .dev = {
        .coherent_dma_mask = ~0,
        .platform_data = &xm_sata1_platdata,
        .release = xm_sata_release
    },
    .resource = xm_sata1_resource,
    .num_resources = ARRAY_SIZE(xm_sata1_resource),
};
int __init xm580_start_sata(void)
{
    int hr;
    printk(KERN_INFO "!!!!!!xm580_start_sata...!!!!!!\n");
    hr = platform_device_register(&xm_ahci0);
    if(hr)
    {
        printk(KERN_INFO "failed to register ahci0: %d\n", hr);
    }
    hr = platform_device_register(&xm_ahci1);
    if(hr)
    {
        printk(KERN_INFO "failed to register ahci1: %d\n", hr);
    }

    return 0;
}
module_init(xm580_start_sata);



static __init int xm_sata0_init(struct device *dev, void *mmio)
{
    printk("%s......\n","xm_sata0_init");
    writel(1, (void*)0xfe100000);
    writel(2, (void*)0xfe100150);
    writel(1, mmio + 4);
    mdelay(1);
    writel(3, (void*)0xfe100150);
    mdelay(1);
    writel(7, (void*)0xfe100150);
    writel(0, mmio + 4);
    writel(0, (void*)0xfe100000);
    writel(0x7d, (void*)0xfe0e2090);
    writel(0x7a, (void*)0xfe0e25b8);
    writel(0x6f36ff80, mmio);
    writel(1, mmio + 0xC); //enable the port
    msleep(1);
    return 0;
}
static __init int xm_sata1_init(struct device *dev, void *mmio)
{
    printk(KERN_INFO "xm_sata1_init...\n");
    writel(1, (void*)0xfe100000);
    writel(2, (void*)0xfe100154);
    writel(1, mmio + 4);
    mdelay(1);
    writel(3, (void*)0xfe100154);
    mdelay(1);
    writel(7, (void*)0xfe100154);
    writel(0, mmio + 4);
    writel(0, (void*)0xfe100000);
    writel(0x7d, (void*)0xfe0e3090);
    writel(0x7a, (void*)0xfe0e35b8);
    writel(0x6f36ff80, mmio);
    writel(1, mmio + 0xC); //enable the port
    msleep(1);
    return 0;
}