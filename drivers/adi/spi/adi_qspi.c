/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdbool.h>
#include <drivers/delay_timer.h>
#include <drivers/spi_mem.h>
#include <drivers/adi/adi_qspi.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include "adi_qspi_regs.h"

/* Basic functional description
 *
 * The functionality exposed to the user (adi_qspi_exec_op) represents a single
 * device transaction (ie: read chipID or read 40KB from addr xxxx).
 * Every transaction is composed of several individual transfers (adi_qspi_xfer):
 *     cmd + addr (3/4 bytes) + dummy cycles + data (read/write)
 * which take care of the HW transfer length limit (64 KB) by spliting the
 * transfer (if applicable).in several blocks.
 * Finally, there are two primitives (adi_qspi_tx_xfer, adi_qspi_rx_xfer)
 * intended to send or receive data.
 *
 * Driver characteristics:
 *
 * - Support all SPI protocols (standard SPI, Extended, Dual and Quad)
 * - Chip select managed by FW
 * - Support DMA (DDE)
 * - Poll based design (blocking calls)
 * - SPI word transfer size is always 8-bit
 * - DMA transfer width as large as possible
 *
 * Not supported:
 * - 16-bit and 32-bit SPI transfer word size
 * - XIP
 * - More than one connected device
 */

#define MEM_AUTO_INC                1U                  /* Increment mem address on each byte sent/received */
#define MEM_NO_INC                  0U
#define SPI_BUSWIDTH_1_LINE         1U                  /* 1 Data Line Buswidth */
#define SPI_BUSWIDTH_2_LINE         2U                  /* 2 Data Lines Buswidth */
#define SPI_BUSWIDTH_4_LINE         4U                  /* 4 Data Lines Buswidth */
#define DEVICE_ADDR_MAX_BYTES       4U                  /* Device address is 3 or 4 bytes */
#define SPI_MIN_FREQ                100000U             /* SPI min freq: 100 KHz
	                                                 * This safe limit is based on the TE timer capability to measure the required
	                                                 * timeout interval for the worst case scenario (64KB transfer) */
#define QSPI_FIFO_TIMEOUT_US        5000U               /* Timeout for CMD, ADDR and dummy bytes (less than FIFO size) */
#define QSPI_DATA_TIMEOUT_US        15000000U           /* Timeout for data block (based on worst case SPI_MIN_FREQ and max transfer size(64KB) */

/* DDE Regs */
#define SPI_SIZE                    0U                  /* SPI word transfer size to 8-bit */
#define DDE_PSIZE                   0U                  /* Peripheral-DDE bus width to 8-bit */
#define DDE_MSIZE_MAX               5U                  /* DDE-System memory bus width range: 8-bit to 64-bit */
#define DDE_STAT_STOPPED            (DDE_STAT_RUN_STOPPED << DDE_STAT_RUN_OFFSET)

/* SPI Regs */
#define QSPI_MAX_CHIP               7U                                                  /* Max Chip Select lines */
#define BIT_SSEL_VAL(x)             ((1 << x) << (SPI_SLVSEL_SSEL_OFFSET - 1))          /* Slave Select input value bit (x = 1..QSPI_MAX_CHIP) */
#define BIT_SSEL_EN(x)              ((1 << x) << (SPI_SLVSEL_SSE_OFFSET - 1))           /* Slave Select enable bit (x = 1..QSPI_MAX_CHIP) */
#define SPI_CTL_MIOM_DUAL           (SPI_CTL_MIOM_MIO_DUAL << SPI_CTL_MIOM_OFFSET)      /* MIOM: Enable DIOM (Dual I/O Mode) */
#define SPI_CTL_MIOM_QUAD           (SPI_CTL_MIOM_MIO_QUAD << SPI_CTL_MIOM_OFFSET)      /* MIOM: Enable QUAD (Quad SPI Mode) */
#define SPI_TXCTL_TDR_NOT_FULL      (SPI_TXCTL_TDR_NF << SPI_TXCTL_TDR_OFFSET)          /* TDR: TFIFO not full */
#define SPI_RXCTL_RDR_NOT_EMPTY     (SPI_RXCTL_RDR_NE << SPI_RXCTL_RDR_OFFSET)          /* RDR: RFIFO not empty */
#define MAX_TRANSFER_WORD_COUNT     ((SPI_TWC_MASK / 32) * 32)                          /* Max SPI block transfer size valid for all DDE configurations (multiple of 32 bytes)*/
#define SPI_CTL_SIZE_MAX            (SPI_CTL_SIZE_SIZE32)                               /* Max Transfer Word Size value */


static struct adi_qspi_ctrl adi_qspi_params;

static uintptr_t qspi_base(void)
{
	return adi_qspi_params.reg_base;
}

static uintptr_t qspi_dde_tx_base(void)
{
	return adi_qspi_params.tx_dde_reg_base;
}

static uintptr_t qspi_dde_rx_base(void)
{
	return adi_qspi_params.rx_dde_reg_base;
}

static void qspi_cs_activate(unsigned int cs)
{
	uint32_t ssel = 0;

	(void)cs;

	if ((cs > 0) && (cs <= QSPI_MAX_CHIP)) {
		ssel |= BIT_SSEL_EN(cs);
		/* Required double write to get result on SLVSEL port */
		mmio_setbits_32(qspi_base() + SPI_SLVSEL, ssel);
		mmio_setbits_32(qspi_base() + SPI_SLVSEL, ssel);
		mmio_clrbits_32(qspi_base() + SPI_SLVSEL, BIT_SSEL_VAL(cs));
		mmio_clrbits_32(qspi_base() + SPI_SLVSEL, BIT_SSEL_VAL(cs));
	}
}

static void qspi_cs_deactivate(unsigned int cs)
{
	uint32_t ssel = 0;

	if ((cs > 0) && (cs <= QSPI_MAX_CHIP)) {
		ssel |= BIT_SSEL_VAL(cs);
		/* Required double write to get result on SLVSEL port */
		mmio_setbits_32(qspi_base() + SPI_SLVSEL, ssel);
		mmio_setbits_32(qspi_base() + SPI_SLVSEL, ssel);
		ssel &= ~BIT_SSEL_EN(cs);
		mmio_setbits_32(qspi_base() + SPI_SLVSEL, ssel);
		mmio_setbits_32(qspi_base() + SPI_SLVSEL, ssel);
	}
}

static int adi_qspi_set_miom(unsigned int mode)
{
	int ret = 0;

	mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_SOSI);

	switch (mode) {
	case SPI_BUSWIDTH_1_LINE:
		mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_MIOM_MASK);
		break;
	case SPI_BUSWIDTH_2_LINE:
		mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_MIOM_MASK);
		mmio_setbits_32(qspi_base() + SPI_CTL, SPI_CTL_MIOM_DUAL);
		break;
	case SPI_BUSWIDTH_4_LINE:
		mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_MIOM_MASK);
		mmio_setbits_32(qspi_base() + SPI_CTL, SPI_CTL_MIOM_QUAD);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

/* This is necessary to allow sharing of this driver with TinyEnclave
 * - move its memory access window between reading the FIFO and writing to the buffer.
 * - configure DMA with the actual system memory address (not the remapped one)
 */
#ifdef ADI_TE_PLAT
extern void adi_qspi_read_fifo(uint8_t *val, uintptr_t addr);
extern void adi_qspi_write_fifo(uint8_t *val, uintptr_t addr);
extern uint8_t adi_qspi_te_get_dma_addr(uint32_t remapped_addr, uint32_t *unremapped_addr, bool dir);
#else
static void adi_qspi_read_fifo(uint8_t *val, uintptr_t addr)
{
	*val = mmio_read_8(addr);
}

static void adi_qspi_write_fifo(uint8_t *val, uintptr_t addr)
{
	mmio_write_8(addr, *val);
}
#endif

static void adi_qspi_get_buses_size(uintptr_t addr, uint32_t len,
				    uint8_t *psize, uint8_t *msize)
{
	int nbytes;
	int i;

	*msize = 0;
	*psize = DDE_PSIZE;
	for (i = DDE_MSIZE_MAX; i >= 0; i--) {
		nbytes = (1 << i);
		if (((addr % nbytes) == 0) && ((len % nbytes) == 0)) {
			*msize = i;
			break;
		}
	}
}

static int adi_qspi_tx_xfer(bool use_dma, uint8_t *buf, uint32_t transfer_len, uint8_t mem_inc)
{
	int ret = 0;
	int i;
	uint64_t timeout;
	uint32_t ctl;
	uint8_t psize;
	uint8_t msize = 0;
	uint32_t dde_xmod = (mem_inc == MEM_AUTO_INC) ? 1 : 0;

	/*  Clean status and tx_ctl */
	mmio_write_32(qspi_base() + SPI_TXCTL, 0);
	mmio_write_32(qspi_base() + SPI_STAT, 0xFFFFFFFF);

	/* Set bytes to send.*/
	mmio_write_32(qspi_base() + SPI_TWC, transfer_len);
	mmio_write_32(qspi_base() + SPI_TWCR, 0);

	/* Enable transmitter */
	ctl = SPI_TXCTL_TEN | SPI_TXCTL_TTI | SPI_TXCTL_TWCEN;
	if (use_dma)
		ctl |= SPI_TXCTL_TDR_NOT_FULL;
	mmio_write_32(qspi_base() + SPI_TXCTL, ctl);

	/* Send */
	if (use_dma) {
		uint32_t cfg;

		/* Get PSIZE/MSIZE */
		adi_qspi_get_buses_size((uintptr_t)buf, transfer_len, &psize, &msize);

		/* Clean and invalidate ddache before using DMA */
		flush_dcache_range((uintptr_t)buf, transfer_len);

		/* Configure Tx DDE */
		mmio_write_32(qspi_dde_tx_base() + DDE_ADDRSTART, (uintptr_t)buf);
		mmio_write_32(qspi_dde_tx_base() + DDE_XCNT, transfer_len >> msize);
		mmio_write_32(qspi_dde_tx_base() + DDE_XMOD, dde_xmod << msize);
		mmio_write_32(qspi_dde_tx_base() + DDE_YCNT, 0);
		mmio_write_32(qspi_dde_tx_base() + DDE_YMOD, 0);
		cfg = (msize << DDE_CFG_MSIZE_OFFSET) |
		      (psize << DDE_CFG_PSIZE_OFFSET) |
		      DDE_CFG_SYNC | DDE_CFG_EN;
		mmio_write_32(qspi_dde_tx_base() + DDE_CFG, cfg);

		/* Wait for DMA to complete */
		timeout = timeout_init_us(QSPI_DATA_TIMEOUT_US);
		while ((mmio_read_32(qspi_dde_tx_base() + DDE_STAT) & DDE_STAT_RUN) != DDE_STAT_STOPPED) {
			if (timeout_elapsed(timeout)) {
				ERROR("%s: DDE tx timeout\n", __func__);
				ret = -ETIMEDOUT;
				goto tx_transfer_end;
			}
		}
	} else {
		timeout = timeout_init_us(QSPI_DATA_TIMEOUT_US);
		for (i = 0; i < transfer_len; i++) {
			while (mmio_read_32(qspi_base() + SPI_STAT) & SPI_STAT_TFF) {
				if (timeout_elapsed(timeout)) {
					ERROR("%s: tx fifo timeout\n", __func__);
					ret = -ETIMEDOUT;
					goto tx_transfer_end;
				}
			}

			adi_qspi_write_fifo(buf, qspi_base() + SPI_TFIFO);
			if (mem_inc == MEM_AUTO_INC)
				buf++;
		}
	}

	/* Wait for transmission to complete */
	timeout = timeout_init_us(QSPI_FIFO_TIMEOUT_US);
	while ((mmio_read_32(qspi_base() + SPI_STAT) &
		SPI_STAT_TF) != SPI_STAT_TF) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: tx finish timeout\n", __func__);
			ret = -ETIMEDOUT;
			goto tx_transfer_end;
		}
	}

tx_transfer_end:
	/* Clear SPI TXCTL and DDE CFG registers */
	mmio_write_32(qspi_dde_tx_base() + DDE_CFG, 0);
	mmio_write_32(qspi_base() + SPI_TXCTL, 0);

	return ret;
}

static int adi_qspi_rx_xfer(bool use_dma, uint8_t *buf, uint32_t transfer_len, uint8_t mem_inc)
{
	int ret = 0;
	uint64_t timeout;
	uint32_t dde_addr = (uintptr_t)buf;
	uint32_t dde_xmod = (mem_inc == MEM_AUTO_INC) ? 1 : 0;
	uint32_t ctl;
	int len;
	uint8_t psize;
	uint8_t msize = 0;
	uint8_t dummy_buf;

	/*  Clean status and rx_ctl */
	mmio_write_32(qspi_base() + SPI_RXCTL, 0);
	mmio_write_32(qspi_base() + SPI_STAT, 0xFFFFFFFF);

	/* Clean RFIFO (although it should be empty) */
	while (!(mmio_read_32(qspi_base() + SPI_STAT) & SPI_STAT_RFE))
		adi_qspi_read_fifo(&dummy_buf, qspi_base() + SPI_RFIFO);

	/* Set bytes to receive */
	mmio_write_32(qspi_base() + SPI_RWC, transfer_len);
	mmio_write_32(qspi_base() + SPI_RWCR, 0);

	ctl = SPI_RXCTL_REN | SPI_RXCTL_RTI | SPI_RXCTL_RWCEN;

	/* Receive */
	if (use_dma) {
		uint32_t cfg;

		/* Get PSIZE/MSIZE */
		adi_qspi_get_buses_size(dde_addr, transfer_len, &psize, &msize);

		/* Invalidate cache before using DMA */
		inv_dcache_range(dde_addr, transfer_len);

		/* Configure Rx DDE */
		mmio_write_32(qspi_dde_rx_base() + DDE_ADDRSTART, dde_addr);
		mmio_write_32(qspi_dde_rx_base() + DDE_XCNT, transfer_len >> msize);
		mmio_write_32(qspi_dde_rx_base() + DDE_XMOD, dde_xmod << msize);
		mmio_write_32(qspi_dde_rx_base() + DDE_YCNT, 0);
		mmio_write_32(qspi_dde_rx_base() + DDE_YMOD, 0);

		cfg = (msize << DDE_CFG_MSIZE_OFFSET) |
		      (psize << DDE_CFG_PSIZE_OFFSET) |
		      DDE_CFG_WNR | DDE_CFG_SYNC | DDE_CFG_EN;
		mmio_write_32(qspi_dde_rx_base() + DDE_CFG, cfg);

		/* Set SPI->DDE trigger */
		ctl |= SPI_RXCTL_RDR_NOT_EMPTY;

		/* Enable receiver */
		mmio_write_32(qspi_base() + SPI_RXCTL, ctl);

		/* Wait for DMA to complete */
		timeout = timeout_init_us(QSPI_DATA_TIMEOUT_US);
		while ((mmio_read_32(qspi_dde_rx_base() + DDE_STAT) & DDE_STAT_RUN) != DDE_STAT_STOPPED) {
			if (timeout_elapsed(timeout)) {
				ERROR("%s: DDE rx timeout\n", __func__);
				ret = -ETIMEDOUT;
				goto rx_transfer_end;
			}
		}
	} else {
		/* Enable receiver */
		mmio_write_32(qspi_base() + SPI_RXCTL, ctl);

		timeout = timeout_init_us(QSPI_DATA_TIMEOUT_US);
		for (len = transfer_len; len != 0U; len--) {
			while (mmio_read_32(qspi_base() + SPI_STAT) &
			       SPI_STAT_RFE) {
				if (timeout_elapsed(timeout)) {
					ERROR("%s: rx fifo timeout\n", __func__);
					ret = -ETIMEDOUT;
					goto rx_transfer_end;
				}
			}

			adi_qspi_read_fifo(buf, qspi_base() + SPI_RFIFO);
			if (mem_inc == MEM_AUTO_INC)
				buf++;
		}
	}

rx_transfer_end:
	/* Clear SPI RXCTL and DDE CFG registers */
	mmio_write_32(qspi_dde_rx_base() + DDE_CFG, 0);
	mmio_write_32(qspi_base() + SPI_RXCTL, 0);

	return ret;
}

static int adi_qspi_xfer(bool dir, uint8_t buswidth, uint8_t *buf, uint32_t size, uint8_t mem_inc)
{
	int ret = 0;
	uint32_t rem_len;
	uint16_t transfer_len;
	bool use_dma = adi_qspi_params.dma;

	if (size == 0)
		/* Nothing to do */
		return 0;

#ifdef ADI_TE_PLAT
	uint32_t dde_addr = (uintptr_t)buf;

	if (use_dma) {
		if (0 == adi_qspi_te_get_dma_addr(dde_addr, &dde_addr, dir))
			/* Use regular host address for DMA */
			buf = (uint8_t *)dde_addr;
		else
			/* Use legacy SPI, no DMA support for TE SRAM */
			use_dma = false;
	}
#endif

	/* Set bus width */
	ret = adi_qspi_set_miom(buswidth);
	if (ret != 0)
		return ret;

	/* Do transfer */
	rem_len = size;
	while (rem_len > 0U) {
		/* Transfer length limited to MAX_TRANSFER_WORD_COUNT bytes (< 64KB) */
		if (rem_len > MAX_TRANSFER_WORD_COUNT)
			transfer_len = MAX_TRANSFER_WORD_COUNT;
		else
			transfer_len = rem_len;

		if (dir == SPI_MEM_DATA_IN)
			ret = adi_qspi_rx_xfer(use_dma, buf, transfer_len, mem_inc);
		else
			ret = adi_qspi_tx_xfer(use_dma, buf, transfer_len, mem_inc);

		if (ret != 0)
			break;

		/* Update remaining length */
		rem_len -= transfer_len;
		if (mem_inc == MEM_AUTO_INC)
			buf += transfer_len;
	}

	return ret;
}

static int adi_qspi_exec_op(const struct spi_mem_op *op)
{
	int i;
	uint8_t addr_buf[DEVICE_ADDR_MAX_BYTES];
	uint8_t dummy_buf;
	int ret;

	VERBOSE("%s: cmd:%x mode:%d.%d.%d.%d addr:%lx len:%x\n",
		__func__, op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);

	if (op->cmd.opcode) {
		/* Enable SPI (not strictly needed, but disabling first ensures
		 * that internal HW machinery starts from a known state) */
		mmio_clrbits_32(qspi_base() + SPI_CTL, 1);
		mmio_setbits_32(qspi_base() + SPI_CTL, 1);

		/* Command */
		ret = adi_qspi_xfer(SPI_MEM_DATA_OUT, op->cmd.buswidth, (uint8_t *)&op->cmd.opcode, 1, MEM_AUTO_INC);
		if (ret != 0)
			return ret;

		/* Address (shall be sent in Big Endian format */
		if (op->addr.nbytes > DEVICE_ADDR_MAX_BYTES)
			return -EINVAL;

		for (i = 0; i < op->addr.nbytes; i++)
			addr_buf[i] = (uint8_t)(op->addr.val >> (8 * (op->addr.nbytes - 1 - i)));

		ret = adi_qspi_xfer(SPI_MEM_DATA_OUT, op->addr.buswidth, addr_buf, op->addr.nbytes, MEM_AUTO_INC);
		if (ret != 0)
			return ret;

		/* Dummy clocks (0 or more) */
		ret = adi_qspi_xfer(SPI_MEM_DATA_IN, op->dummy.buswidth, &dummy_buf, op->dummy.nbytes, MEM_NO_INC);
		if (ret != 0)
			return ret;

		/* Transfer data */
		ret = adi_qspi_xfer(op->data.dir, op->data.buswidth, op->data.buf, op->data.nbytes, MEM_AUTO_INC);
		if (ret != 0)
			return ret;
	}

	return 0;
}

static int adi_qspi_claim_bus(unsigned int cs)
{
	(void)cs;

	/* Set master enable.*/
	mmio_setbits_32(qspi_base() + SPI_CTL, SPI_CTL_MSTR);

	/* Use FW controlled chip select.*/
	mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_ASSEL);

	/* Assert chip select */
	qspi_cs_activate(adi_qspi_params.cs);

	return 0;
}


static void adi_qspi_release_bus(void)
{
	/* De-assert chip select */
	qspi_cs_deactivate(adi_qspi_params.cs);
}

static int adi_qspi_set_speed(unsigned int hz)
{
	unsigned int sclk = adi_qspi_params.clock_freq;
	uint32_t baud;

	if ((hz < SPI_MIN_FREQ) || (hz > sclk))
		return -EINVAL;

	/* Baud rate formula from spec */
	baud = (sclk / hz) - 1;

	/* Ensure that configured speed is equal or lower than requested */
	if ((sclk % hz) != 0)
		baud++;

	mmio_write_32(qspi_base() + SPI_CLK, baud);

	VERBOSE("%s: baud=%u\n", __func__, baud);

	return 0;
}

static int adi_qspi_set_mode(unsigned int mode)
{
	if ((mode & SPI_CS_HIGH) != 0U)
		return -ENODEV;

	if ((mode & SPI_CPHA) != 0U)
		mmio_setbits_32(qspi_base() + SPI_CTL, SPI_CTL_CPHA);
	else
		mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_CPHA);

	if ((mode & SPI_CPOL) != 0U)
		mmio_setbits_32(qspi_base() + SPI_CTL, SPI_CTL_CPOL);
	else
		mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_CPOL);

	if ((mode & SPI_LSB_FIRST) != 0U)
		mmio_setbits_32(qspi_base() + SPI_CTL, SPI_CTL_LSBF);
	else
		mmio_clrbits_32(qspi_base() + SPI_CTL, SPI_CTL_LSBF);

	VERBOSE("%s: mode=0x%x\n", __func__, mode);

	return 0;
}


static const struct spi_bus_ops adi_qspi_bus_ops = {
	.claim_bus	= adi_qspi_claim_bus,
	.release_bus	= adi_qspi_release_bus,
	.set_speed	= adi_qspi_set_speed,
	.set_mode	= adi_qspi_set_mode,
	.exec_op	= adi_qspi_exec_op,
};

void adi_qspi_deinit(uintptr_t qspi_base, uintptr_t tx_dde_base, uintptr_t rx_dde_base)
{
	/* Disable Tx and Rx DMA channels */
	mmio_write_32(tx_dde_base + DDE_CFG, 0);
	mmio_write_32(rx_dde_base + DDE_CFG, 0);

	/* Disable QSPI.*/
	mmio_write_32(qspi_base + SPI_SLVSEL, 0xFE00);
	mmio_write_32(qspi_base + SPI_SLVSEL, 0xFE00);
	mmio_write_32(qspi_base + SPI_TXCTL, 0);
	mmio_write_32(qspi_base + SPI_RXCTL, 0);
	mmio_write_32(qspi_base + SPI_CTL, 0);
}

int adi_qspi_init(struct adi_qspi_ctrl *params)
{
	assert(params != NULL);

	memcpy(&adi_qspi_params, params, sizeof(struct adi_qspi_ctrl));

	adi_qspi_deinit(qspi_base(), qspi_dde_tx_base(), qspi_dde_rx_base());

	return spi_mem_init_slave_nofdt(adi_qspi_params.mode, adi_qspi_params.cs, adi_qspi_params.spi_clk_freq, &adi_qspi_bus_ops);
};
