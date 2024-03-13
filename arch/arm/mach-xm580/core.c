#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cnt32_to_63.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/leds.h>
#include <asm/mach-types.h>
#include <linux/irqchip/arm-gic.h>    

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/setup.h>

#include <mach/time.h>
#include <mach/hardware.h>
#include <mach/early-debug.h>
#include <mach/irqs.h>
#include <mach/dma.h>

#include <linux/of_platform.h>
#include "mach/clock.h" 
#include "platsmp.h"
#include <asm/device.h>
#include <linux/delay.h>

#define GPIO0_MULT_USE_EN (GPIO_BASE)



static struct map_desc xm580_io_desc[] __initdata = {
	{
		.virtual        = XM580_IOCH1_VIRT,
		.pfn            = __phys_to_pfn(XM580_IOCH1_PHYS),
		.length         = XM580_IOCH1_SIZE,
		.type           = MT_DEVICE
	},
	{
		.virtual        = XM580_IOCH2_VIRT,
		.pfn            = __phys_to_pfn(XM580_IOCH2_PHYS),
		.length         = XM580_IOCH2_SIZE,
		.type           = MT_DEVICE
	}
};


void __init xm580_map_io(void)
{
	int i;

	iotable_init(xm580_io_desc, ARRAY_SIZE(xm580_io_desc));

	for (i = 0; i < ARRAY_SIZE(xm580_io_desc); i++) {
		edb_putstr(" V: ");     edb_puthex(xm580_io_desc[i].virtual);
		edb_putstr(" P: ");     edb_puthex(xm580_io_desc[i].pfn);
		edb_putstr(" S: ");     edb_puthex(xm580_io_desc[i].length);
		edb_putstr(" T: ");     edb_putul(xm580_io_desc[i].type);
		edb_putstr("\n");
	}

	edb_trace();
}


static void __init usb0_init(void)
{
	writel(1, (void*)0xfe100000);
	writel(0, (void*)0xfe100114);
	mdelay(10);
	writel(2, (void*)0xfe100114);
	mdelay(10);
	writel(6, (void*)0xfe100114);
	mdelay(10);
	writel(7, (void*)0xfe100114);
	mdelay(10);
	writel(0, (void*)0xfe100000);
}

static void __init usb1_init(void)
{
	writel(1, (void*)0xfe100000);
	writel(0, (void*)0xfe100140);
 	mdelay(10);
	writel(2, (void*)0xfe100140);
	mdelay(10);
	writel(6, (void*)0xfe100140);
	mdelay(10);
	writel(7, (void*)0xfe100140);
	mdelay(10);
	writel(0, (void*)0xfe100000);
}


static void __init xm580_init_early(void)    
{
	unsigned int tmp;
	unsigned int pllclk;
	unsigned int twdclk;
	edb_trace();
	tmp = readl(__io_address(PLL_PLLA_CTRL));
	pllclk = 24000000 / (tmp & 0x3F) * ((tmp >> 6) & 0xFFF) / (((tmp >> 19) & 0x1) + 1);

	tmp = readl(__io_address(PLL_CPUCLK_CTRL));
	twdclk = pllclk / ((tmp  & 0xFF) + 1) / (((tmp >> 20) & 0x1) == 0 ? 1 : 4);
	//early_print("PLL Clock frequency: %d\nTWD Clock frequency: %d\n", pllclk, twdclk);

	//clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	usb0_init();
	usb1_init();
}

void xm580_restart(enum reboot_mode mode, const char *cmd)
{
	writel(1, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RSTEN));
	writel(0xca110000, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RST));
}

extern void __init xm580_timer_init(void);

asmlinkage void asmprint(void)
{
	edb_trace();
}

static const char *const xm580_match[] = {
	"xmeye,8536d",
	NULL
};

DT_MACHINE_START(XM580, "xm580 (Flattened Device Tree)")
	.atag_offset  = 0x100,
	.map_io         = xm580_map_io,
	.init_early     = xm580_init_early,
	//.smp          = smp_ops(xm580_smp_ops),
	.restart      = xm580_restart,
	.dt_compat	= xm580_match,
MACHINE_END
