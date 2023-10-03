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
#include <linux/clkdev.h>
//#include <asm/sched_clock.h>

//#include <asm/system.h>
#include <asm/irq.h>
#include <linux/leds.h>
#include <asm/hardware/arm_timer.h>
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
#include <linux/bootmem.h>
#include <linux/amba/serial.h> 
#include <linux/amba/pl330.h> 
#include "mach/clock.h" 
#include "platsmp.h"
#include <asm/device.h>

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


void __iomem *xm580_gic_cpu_base_addr;     
void __init xm580_gic_init_irq(void)
{
	edb_trace();
	xm580_gic_cpu_base_addr = (void*)0xfe300100;//__io_address(CFG_GIC_CPU_BASE);
#ifdef CONFIG_LOCAL_TIMERS
	gic_init(0, IRQ_LOCALTIMER, (void*)0xfe301000,
			(void*)0xfe300100);
#else
	gic_init(0, XM580_GIC_IRQ_START, __io_address(CFG_GIC_DIST_BASE),
			__io_address(CFG_GIC_CPU_BASE));
#endif
}


//static struct amba_pl011_data uart1_plat_data = {
	//.dma_filter = pl330_filter,
	//.dma_rx_param = (void *) DMACH_UART1_RX,
	//.dma_tx_param = (void *) DMACH_UART1_TX,
//};

#define XM_AMBADEV_NAME(name) xm_ambadevice_##name

#define XM_AMBA_DEVICE(name, busid, base, platdata)			\
	static struct amba_device XM_AMBADEV_NAME(name) =		\
	{\
		.dev            = {                                     \
			.coherent_dma_mask = ~0,                        \
			.init_name = busid,                             \
			.platform_data = platdata,                      \
		},                                                      \
		.res            = {                                     \
			.start  = base##_BASE,				\
			.end    = base##_BASE + 0x1000 - 1,		\
			.flags  = IORESOURCE_IO,                        \
		},                                                      \
		.irq            = { base##_IRQ, base##_IRQ, }		\
	}

XM_AMBA_DEVICE(uart0, "uart:0",  UART0,    NULL);
XM_AMBA_DEVICE(uart1, "uart:1",  UART1,    NULL);
XM_AMBA_DEVICE(uart2, "uart:2",  UART2,    NULL);
//XM_AMBA_DEVICE(uart1, "uart:1",  UART1,    &uart1_plat_data);

static struct amba_device *amba_devs[] __initdata = {
	&XM_AMBADEV_NAME(uart0),
	&XM_AMBADEV_NAME(uart1),
	&XM_AMBADEV_NAME(uart2),
};

/*
 * These are fixed clocks.
 */
static struct clk uart_clk = {
	.rate   = 24000000,
};
static struct clk sp804_clk = { 
	.rate = 24000000,
};
static struct clk dma_clk = { 
	.rate = 24000000,
};

//正式芯片为CPU时钟的1/4 或与CPU时钟相等
//The official chip is 1/4 of the CPU clock or equal to the CPU clock
static struct clk twd_clk = { 
	.rate = 60000000,
};

static struct clk_lookup lookups[] = {
	{       /* UART0 */
		.dev_id         = "uart:0",
		.clk            = &uart_clk,
	},
	{       /* UART1 */
		.dev_id         = "uart:1",
		.clk            = &uart_clk,
	},
	{       /* UART2 */
		.dev_id         = "uart:2",
		.clk            = &uart_clk,
	},
	{ /* SP804 timers */
		.dev_id     = "sp804",
		.clk        = &sp804_clk,
	},
	{ 
		.dev_id     = "dma-pl330",
		.clk        = &dma_clk,
	},
	{ 
		.dev_id     = "smp_twd",
		.clk        = &twd_clk,
	},
};

static void __init xm580_init_early(void)    
{
	unsigned int tmp;
	unsigned int pllclk;
	edb_trace();
	tmp = readl(__io_address(PLL_PLLA_CTRL));
	pllclk = 12000000 / (tmp & 0x3F) * ((tmp >> 6) & 0xFFF) / (((tmp >> 19) & 0x1) + 1);

	tmp = readl(__io_address(PLL_CPUCLK_CTRL));
	twd_clk.rate = pllclk / ((tmp  & 0xFF) + 1) / (((tmp >> 20) & 0x1) == 0 ? 1 : 4);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));
}

void __init xm580_init(void)
{
	unsigned long i;

	edb_trace();
	writel(0x84, (void*)0xfe020050);
	writel(0x84, (void*)0xfe020054);

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		amba_device_register(amba_devs[i], &iomem_resource);
	}
}
static void __init xm580_reserve(void)
{
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

MACHINE_START(XM580, "xm580")
	.atag_offset  = 0x100,
	.map_io         = xm580_map_io,
	.init_early     = xm580_init_early,
	.init_irq       = xm580_gic_init_irq,
	.init_time    	= xm580_timer_init,
	.init_machine   = xm580_init,
	.smp          = smp_ops(xm580_smp_ops),
	.reserve      = xm580_reserve,
	.restart      = xm580_restart,
MACHINE_END
