#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/compiler.h>

// NOTE: this file orginally was at the drivers/ata folder (I think), but it works just as well here. Note that 3 modifications are needed for SATA to work in libata-core.c


static __init int xm_sata0_init(struct device *dev, void *mmio)
{
    mmio = ioremap(0x50500000, 0xFFFFF);
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
    iounmap(mmio);
    return 0;
}
static __init int xm_sata1_init(struct device *dev, void *mmio)
{
    mmio = ioremap(0x50600000, 0xFFFFF);
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
    iounmap(mmio);
    return 0;
}

int __init xm580_start_sata(void)
{
    printk(KERN_INFO "xm580: Booting SATA controller\n");

    // Hack!
    xm_sata0_init(NULL, (void*)0x50500000);
    xm_sata1_init(NULL, (void*)0x50600000);

    return 0;
}
module_init(xm580_start_sata);
