#include <linux/dma-mapping.h>
#include <linux/amba/bus.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>

#include <mach/time.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <linux/clkdev.h>
#include <mach/dma.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>


static u8  xm580_dma_peri[] = {
	DMACH_MAX,
	DMACH_MAX,
	DMACH_SPI0_TX,
	DMACH_SPI0_RX,
	DMACH_SPI1_TX,
	DMACH_SPI1_RX,
	DMACH_SPI2_TX,
	DMACH_SPI2_RX,
	DMACH_I2S,
	DMACH_UART0_TX,
	DMACH_UART0_RX,
	DMACH_UART1_TX,
	DMACH_UART1_RX,
	DMACH_UART2_TX,
	DMACH_UART2_RX,
	DMACH_I2S_TX,
	DMACH_I2S_RX,
	DMACH_MAX,
};
struct dma_pl330_platdata {
	/*
	 * Number of valid peripherals connected to DMAC.
	 * This may be different from the value read from
	 * CR0, as the PL330 implementation might have 'holes'
	 * in the peri list or the peri could also be reached
	 * from another DMAC which the platform prefers.
	 */
	u8 nr_valid_peri;
	/* Array of valid peripherals */
	u8 *peri_id;
	/* Operational capabilities */
	dma_cap_mask_t cap_mask;
	/* Bytes to allocate for MC buffer */
	unsigned mcbuf_sz;
};

static struct dma_pl330_platdata xm580_dma_platdata = {
	.nr_valid_peri = ARRAY_SIZE(xm580_dma_peri),
	.peri_id = xm580_dma_peri,
};


static AMBA_AHB_DEVICE(xm580_dma, "dma-pl330", 0x00041330,
	DMAC_BASE, {DMAC_IRQ}, NULL);

static int __init xm580_dmac_init(void)
{
	dma_cap_set(DMA_SLAVE, xm580_dma_platdata.cap_mask);
	dma_cap_set(DMA_CYCLIC, xm580_dma_platdata.cap_mask);
	xm580_dma_device.dev.platform_data = &xm580_dma_platdata;
	amba_device_register(&xm580_dma_device, &iomem_resource);

	return 0;
}
arch_initcall(xm580_dmac_init);
