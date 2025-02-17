#include <stdio.h>
#include <stdbool.h>

#include <lib/utils.h>
#include <lib/mmio.h>
#include <drivers/adi/adi_qspi.h>
#include <drivers/adi/adi_spu.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/adrv906x_gpio.h>
#include <drivers/spi_nor.h>

#include <adrv906x_def.h>
#include <adrv906x_spu_def.h>
#include <adrv906x_pinctrl.h>
#include <adrv906x_pinmux_source_def.h>
#include <plat_pinctrl.h>
#include <platform_def.h>
#include "../spi/adi_qspi_regs.h"       // SPI regs

/* The test read/write buffers' size may be limited by the amount of free
 * resources (ram) in the system. If the build complains about not enough ram
 * for the read'write buffers, you can optionally define the address
 * (BUFFER_ADDR) where those buffers will be living..
 * Note: be carefull when choosing that address
 */
#define BUF_LEN       10000
#define DEVICE_ADDR   0x00000000
//#define BUFFER_ADDR   0x00500000

#define SPI_CTL_MIOM_DUAL           (SPI_CTL_MIOM_MIO_DUAL << SPI_CTL_MIOM_OFFSET)      /* MIOM: Enable DIOM (Dual I/O Mode) */
#define SPI_CTL_MIOM_QUAD           (SPI_CTL_MIOM_MIO_QUAD << SPI_CTL_MIOM_OFFSET)      /* MIOM: Enable QUAD (Quad SPI Mode) */
#define SPI_TXCTL_TDR_NOT_FULL      (SPI_TXCTL_TDR_NF << SPI_TXCTL_TDR_OFFSET)          /* TDR: TFIFO not full */
#define SPI_RXCTL_RDR_NOT_EMPTY     (SPI_RXCTL_RDR_NE << SPI_RXCTL_RDR_OFFSET)          /* RDR: RFIFO not empty */
#define SPI_STAT_TFS                (7 << 16)
#define SPI_TXCTL_TFS_EMPTY         (4 << 16)                                           /* TFS empty */

/************************** Test INSTRUCTIONS ***************************/
/* This is a test framework to verify the functionality of QSPI driver which
 * supports read,write and erase operations. By default, this framework is not
 * included in TF-A build. Do the following before attempting to test the driver.
 * 1. Add the following in drivers/adi/test/test_framework.c
 *    -> extern int adi_qspi_test(void)
 *    -> Call adi_qspi_test() inside test_main()
 * 2. Include this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 * 3. Feel free to modify the test conditions:
 *    BUF_LEN    : Read and write buffers' size (in bytes)
 *    BUFFER_ADDR: Specify the location of read/write buffers
 *    DEVICE_ADDR: Flash memory address
 *
 * Test functionality:
 * - write BUFFER_LEN bytes to flash memory, read-back the same emory area and
 *   check that was successfully
 *
 * Note: The write operation performs write enable/disable, erase and program
 * operations.
 ************************************************************************/
static int adi_qspi_test_1()
{
	int ret = 0;
	size_t len = 0;
	int i;
	int j;
	unsigned int ersz;
	unsigned long long sz;

	/* NOR flash driver init (read id and setup spi-nor structure) */
	ret = spi_nor_init(&sz, &ersz);
	if (ret != 0) {
		printf("SPI nor flash init failed\n");
		return -1;
	}

	/* System memory address to read/write from/to */
#ifdef BUFFER_ADDR
	/* Point to specific address (product specific) */
	uint8_t *bufferWR = (uint8_t *)BUFFER_ADDR;
	uint8_t *bufferRD = (uint8_t *)(BUFFER_ADDR + BUF_LEN);
#else
	/* Point to a buffer */
	static uint8_t bufWR[BUF_LEN] = { 0 };
	static uint8_t bufRD[BUF_LEN] = { 0 };
	uint8_t *bufferWR = (uint8_t *)bufWR;
	uint8_t *bufferRD = (uint8_t *)bufRD;
#endif

	/* Device address to read/write from/to */
	size_t addr = DEVICE_ADDR;

	/* SystemC write operation is not yet supported */

	/* Init WR buffer with arbitrary data */
	for (i = 0; i < BUF_LEN; i++)
		bufferWR[i] = i % 256;

	/* Write data to spi nor flash */
	ret = spi_nor_write(addr, (uintptr_t)bufferWR, BUF_LEN);
	if (ret != 0) {
		printf("Failed to Write %d bytes to nor-flash address 0x%lx \n", BUF_LEN, addr);
		return -1;
	}

	/* Init RD buffer with arbitrary data */
	for (i = 0; i < BUF_LEN; i++)
		bufferRD[i] = 0x55;

	/* Subtest 1: Read BUF_LEN bytes at one shot */
	ret = spi_nor_read(addr, (uintptr_t)bufferRD, BUF_LEN, &len);
	if (ret != 0 && len != BUF_LEN) {
		printf("Failed to Read %d bytes from nor-flash address 0x%lx \n", BUF_LEN, addr);
		return -1;
	}

	/* Dump first read 80 bytes in the same format 'hd' tool does, and
	 * compare with the content of 'nor_flash.dat' file
	 * >> hd output/nor_flash.dat | grep 00000000 -A5
	 */
	for (i = 0; i < 5; i++) {
		printf("\n%08x  ", (unsigned int)(addr + 16 * i));
		for (j = 0; j < 16; j++)
			printf("%02x ", bufferRD[16 * i + j]);
	}

	/* Check */
	for (j = 0; j < BUF_LEN ; j++)
		if (bufferWR[j] != bufferRD[j]) {
			printf("%d bytes Data written and read back from nor-flash address 0x%lx Does NOT match\n", BUF_LEN, addr);
			return -1;
		}

	return ret;
}

static int adi_qspi_setup(bool dma)
{
	int ret = 0;
	struct adi_qspi_ctrl qspi_params;
	extern const plat_pinctrl_settings qspi_pin_grp[];
	extern const size_t qspi_pin_grp_members;

	/* Set MSEC for DDE Tx and RX */
	adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_QUAD_SPI_DMA_0);
	adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_QUAD_SPI_DMA_1);

	/* Initialize GPIO framework */
	adrv906x_gpio_init(GPIO_MODE_SECURE_BASE, SEC_GPIO_MODE_SECURE_BASE);
	plat_secure_pinctrl_set_group(qspi_pin_grp, qspi_pin_grp_members, true);

	qspi_params.reg_base = QSPI_0_BASE;
	qspi_params.tx_dde_reg_base = QSPI_0_TX_DDE_BASE;
	qspi_params.rx_dde_reg_base = QSPI_0_RX_DDE_BASE;
	qspi_params.clock_freq = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
	qspi_params.mode = SPI_TX_QUAD | SPI_RX_QUAD;
	qspi_params.cs = QSPI_FLASH_CHIP_SELECT;
	qspi_params.spi_clk_freq = QSPI_CLK_FREQ_HZ;
	qspi_params.dma = dma;

	/* QSPI driver init */
	ret = adi_qspi_init(&qspi_params);
	if (ret != 0) {
		printf("QSPI init failed\n");
		return -1;
	}

	return 0;
}

static int adi_qspi_drop()
{
	/* QSPI driver deinit */
	adi_qspi_deinit(QSPI_0_BASE, QSPI_0_TX_DDE_BASE, QSPI_0_RX_DDE_BASE);

	return 0;
}

static int adi_qspi_playground(bool use_dma)
{
	static uint8_t bufferRD[BUF_LEN] = { 0 };
	int ret = 0;
	int i;
	int j;
	uint8_t *buf = (uint8_t *)bufferRD;
	uint8_t cmd = 0xeb;
	uint8_t dev_addr = 0x00000000;
	uint8_t dev_addr_buf[4];
	uint8_t msize = 0;

	/*
	 * Purpose: read and dump 80 bytes from device address 0x00
	 *         Spaguetti code just to play in RAW (no driver) with SPI/DDE
	 *         Using Extended Protocol (1-4-4)
	 *         Using DMA
	 *         No SPI disable between CMD-ADDR-dummy-data transfers
	 *         No CS as GPIO
	 *         No timeouts (just playground to experiment)
	 */


	/* Common config */
	mmio_write_32(QSPI_0_BASE + SPI_SLVSEL, 0x0000fe00); // Slave select 1 enabled and de-asserted
	mmio_write_32(QSPI_0_BASE + SPI_DLY, 0x00000301);
	mmio_write_32(QSPI_0_BASE + SPI_CLK, 0x00000077);

	/* Concatenate a couple of flash read transactions */
	for (i = 0; i < 2; i++) {
		/* Init RD buffer with arbitrary data */
		for (i = 0; i < BUF_LEN; i++)
			bufferRD[i] = 0x55;

		/* -Enable SPI as master */
		mmio_write_32(QSPI_0_BASE + SPI_CTL, 0x00000000);
		mmio_write_32(QSPI_0_BASE + SPI_CTL, 0x00000003);

		/* CS Assert */
		mmio_setbits_32(QSPI_0_BASE + SPI_SLVSEL, 0x00000002);
		mmio_clrbits_32(QSPI_0_BASE + SPI_SLVSEL, 0x00000200);

		/* Send CMD */
		/* - Bus width 1 line, start on MOSI and clear STAT bits */
		mmio_setbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_SOSI);
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_MASK);
		mmio_setbits_32(QSPI_0_BASE + SPI_STAT, 0xFFFFFFFF);
		/* - Set bytes to send */
		mmio_clrbits_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TWCEN);
		mmio_write_32(QSPI_0_BASE + SPI_TWC, 1);
		mmio_write_32(QSPI_0_BASE + SPI_TWCR, 0);
		mmio_write_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TWCEN);
		/* - Send */
		if (use_dma) {
			/* - Enable Tx (and set tx initiator and TDR trigger) */
			mmio_setbits_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TTI | SPI_TXCTL_TDR_NOT_FULL | SPI_TXCTL_TEN);
			/* Invalidate cache */
			flush_dcache_range((uintptr_t)&cmd, 1);
			/* - Set DMA job */
			msize = 0;
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_ADDRSTART, (uintptr_t)&cmd);
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_XCNT, 1 >> msize);
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_XMOD, 1 << msize);
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_CFG, (msize << DDE_CFG_MSIZE_OFFSET) | DDE_CFG_SYNC | DDE_CFG_EN);
			/* - Send command byte */
			while ((mmio_read_32(QSPI_0_TX_DDE_BASE + DDE_STAT) & 0x00000700) != 0x00000000);
		} else {
			/* - Enable Tx (and set tx initiator) */
			mmio_setbits_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TTI | SPI_TXCTL_TEN);
			/* - wait for Tx FIFO not full */
			while (mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_TFF); // wait for TX FIFO not full
			/* - Send command byte */
			mmio_write_8(QSPI_0_BASE + SPI_TFIFO, cmd);
		}
		/* - wait for Tx to be completed (TF flag) */
		while ((mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_TF) != SPI_STAT_TF);
		/* - Disable Tx */
		mmio_write_32(QSPI_0_BASE + SPI_TXCTL, 0x0);

		/* Send ADDR */
		/* - Bus width 4 lines and clear STAT bits */
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_SOSI);
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_MASK);
		mmio_setbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_QUAD);
		mmio_setbits_32(QSPI_0_BASE + SPI_STAT, 0xFFFFFFFF);
		/* - Set bytes to send */
		mmio_clrbits_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TWCEN);
		mmio_write_32(QSPI_0_BASE + SPI_TWC, 3);
		mmio_write_32(QSPI_0_BASE + SPI_TWCR, 0);
		mmio_write_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TWCEN);
		/* - Send address bytes */
		dev_addr_buf[0] = dev_addr >> 16;
		dev_addr_buf[1] = dev_addr >> 8;
		dev_addr_buf[2] = dev_addr;
		dev_addr_buf[0] = 0;
		/* - Send */
		//if(0)
		if (use_dma) {
			/* - Enable Tx (and set tx initiator and TDR trigger) */
			mmio_setbits_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TTI | SPI_TXCTL_TDR_NOT_FULL | SPI_TXCTL_TEN);
			/* Invalidate cache */
			flush_dcache_range((uintptr_t)&dev_addr_buf, 3);
			/* - Set DMA job */
			msize = 0;
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_ADDRSTART, (uintptr_t)dev_addr_buf);
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_XCNT, 3 >> msize);
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_XMOD, 1 << msize);
			mmio_write_32(QSPI_0_TX_DDE_BASE + DDE_CFG, (msize << DDE_CFG_MSIZE_OFFSET) | DDE_CFG_SYNC | DDE_CFG_EN);
			/* - Send command byte */
			while ((mmio_read_32(QSPI_0_TX_DDE_BASE + DDE_STAT) & 0x00000700) != 0x00000000);
		} else {
			/* - Enable Tx (and set tx initiator) */
			mmio_setbits_32(QSPI_0_BASE + SPI_TXCTL, SPI_TXCTL_TTI | SPI_TXCTL_TEN);
			for (i = 0; i < 3; i++) {
				/* - wait for Tx FIFO not full before insering the next byte */
				while (mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_TFF); // wait for TX FIFO not full
				mmio_write_8(QSPI_0_BASE + SPI_TFIFO, dev_addr_buf[i]);
			}
		}
		/* - wait for Tx to be completed (TF flag) */
		while ((mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_TF) != SPI_STAT_TF);
		/* - Disable Tx */
		mmio_write_32(QSPI_0_BASE + SPI_TXCTL, 0x0);

		/* Dummy clocks */
		/* - Bus width 4 lines and clear STAT bits */
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_SOSI);
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_MASK);
		mmio_setbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_QUAD);
		mmio_setbits_32(QSPI_0_BASE + SPI_STAT, 0xFFFFFFFF); // Clear all STATUS flags
		/* - Set bytes to read.*/
		mmio_clrbits_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_RWCEN);
		mmio_write_32(QSPI_0_BASE + SPI_RWC, 5);
		mmio_write_32(QSPI_0_BASE + SPI_RWCR, 0);
		/* Clean Rx FIFO */
		while (!(mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_RFE))
			mmio_read_8(QSPI_0_BASE + SPI_RFIFO);
		/* - Enable Rx */
		mmio_write_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_RTI | SPI_RXCTL_RWCEN);
		/* - Read data */
		if (use_dma) {
			/* Clean and invalidate dcache */
			inv_dcache_range((uintptr_t)buf, 5);
			/* - Set DMA job */
			msize = 0;
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_ADDRSTART, (uintptr_t)buf);
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_XCNT, 5 >> msize);
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_XMOD, 0 << msize);
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_CFG, (msize << DDE_CFG_MSIZE_OFFSET) | DDE_CFG_WNR | DDE_CFG_SYNC | DDE_CFG_EN);
			/* - Enable receiver and set RDR trigger */
			mmio_setbits_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_RDR_NOT_EMPTY | SPI_RXCTL_REN);
			/* - Wait for dummy bytes receive done */
			while ((mmio_read_32(QSPI_0_RX_DDE_BASE + DDE_STAT) & 0x00000700) != 0x00000000);
		} else {
			/* - Enable receiver */
			mmio_setbits_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_REN);
			/* = Receive */
			for (i = 0; i < 5; i++) {
				/* - wait for Rx FIFO not empty before reading the next byte */
				while ((mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_RFE));
				buf[i] = mmio_read_8(QSPI_0_BASE + SPI_RFIFO);
			}
		}
		/* - Disable Rx */
		mmio_write_32(QSPI_0_BASE + SPI_RXCTL, 0x0);

		/* Transfer */
		/* - Bus width 4 lines and clear STAT bits */
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_SOSI);
		mmio_clrbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_MASK);
		mmio_setbits_32(QSPI_0_BASE + SPI_CTL, SPI_CTL_MIOM_QUAD);
		mmio_setbits_32(QSPI_0_BASE + SPI_STAT, 0xFFFFFFFF); // Clear all STATUS flags
		/* - Set bytes to read.*/
		mmio_clrbits_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_RWCEN);
		mmio_write_32(QSPI_0_BASE + SPI_RWC, 80);
		mmio_write_32(QSPI_0_BASE + SPI_RWCR, 0);
		/* Clean Rx FIFO */
		while (!(mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_RFE))
			mmio_read_8(QSPI_0_BASE + SPI_RFIFO);
		/* - Enable Rx */
		mmio_write_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_RTI | SPI_RXCTL_RWCEN);
		/* - Read data */
		if (use_dma) {
			inv_dcache_range((uintptr_t)buf, 80);
			/* - Set DMA job */
			msize = 0;
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_ADDRSTART, (uintptr_t)buf);
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_XCNT, 80 >> msize);
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_XMOD, 1 << msize);
			mmio_write_32(QSPI_0_RX_DDE_BASE + DDE_CFG, (msize << DDE_CFG_MSIZE_OFFSET) | DDE_CFG_WNR | DDE_CFG_SYNC | DDE_CFG_EN);
			/* - Enable receiver and set RDR trigger */
			mmio_setbits_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_RDR_NOT_EMPTY | SPI_RXCTL_REN);
			/* - Wait for data bytes receive done */
			while ((mmio_read_32(QSPI_0_RX_DDE_BASE + DDE_STAT) & 0x00000700) != 0x00000000);
		} else {
			/* - Enable receiver */
			mmio_setbits_32(QSPI_0_BASE + SPI_RXCTL, SPI_RXCTL_REN);
			/* = Receive */
			for (i = 0; i < 80; i++) {
				/* - wait for Rx FIFO not empty before reading the next byte */
				while ((mmio_read_32(QSPI_0_BASE + SPI_STAT) & SPI_STAT_RFE)); ;
				buf[i] = mmio_read_8(QSPI_0_BASE + SPI_RFIFO);
			}
		}
		/* - Disable Rx */
		mmio_write_32(QSPI_0_BASE + SPI_RXCTL, 0x0);

		/* CS De-assert */
		mmio_clrbits_32(QSPI_0_BASE + SPI_SLVSEL, 0x00000002);
		mmio_setbits_32(QSPI_0_BASE + SPI_SLVSEL, 0x00000200);

		/* Dump read data */
		for (i = 0; i < 5; i++) {
			printf("\n%08x  ", (unsigned int)(dev_addr + 16 * i));
			for (j = 0; j < 16; j++)
				printf("%02x ", bufferRD[16 * i + j]);
		}
	}

	return ret;
}


int adi_qspi_test()
{
	int ret = 0;
	/* Feel free to enable/disable DMA support */
	int dma = false;

	adi_qspi_setup(dma);

	/* SPI test test */
	(void)&adi_qspi_playground;
	ret = adi_qspi_test_1();
	printf("Test QSPI: %s\n", (ret == 0) ? "PASS" : "KO");

	adi_qspi_drop();

//        ret = adi_qspi_playground(true);

	return ret;
}
