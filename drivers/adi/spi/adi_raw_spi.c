/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * adi_raw_spi
 * - This is a basic SPI driver used for communication with SPI devices that follow:
 * 		ADDR(7bits) + R/W bit | DATA0(8bits) | ... | DATAn(8bits)
 * - This driver does not rely on our qspi driver, nor the tf-a spi_mem driver.
 * - Speed and mode can be configured by the adi_raw_spi_init function:
 *   - frequency limits are (SPI_MIN_FREQ, SYSCLK)
 *   - supported mode bits are:
 *     SPI_CPHA        clock phase
 *     SPI_CPOL        clock polarity
 *     SPI_LSB_FIRST   LSB first
 * - R/W len is limited to 64kB
 */

#include <assert.h>

#include <lib/mmio.h>
#include <drivers/spi_mem.h>    /* SPI mode flags */

#include <drivers/adi/adi_raw_spi.h>
#include <drivers/delay_timer.h>

#include "adi_qspi_regs.h"      /* SPI block registers */

/*--------------------------------------------------------
 * DEFINES
 *------------------------------------------------------*/
#define SPI_MAX_CHIP_SELECTS        7U
#define BIT_SSEL_CS_VALUE(x)        ((1 << (x + 1)) << (SPI_SLVSEL_SSEL_OFFSET - 1))    /* Slave Select x input bit (x = 0..SPI_MAX_CHIP_SELECTS-1) */
#define BIT_SSEL_CS_ENABLE(x)       ((1 << (x + 1)) << (SPI_SLVSEL_SSE_OFFSET - 1))     /* Slave Select x enable bit (x = 0..SPI_MAX_CHIP_SELECTS-1) */

#define MAX_TRANSMITTED_WORD_COUNT  SPI_TWC_MASK
#define MAX_RECEIVED_WORD_COUNT     SPI_RWC_MASK

#define SPI_MIN_FREQ                100000U     /* SPI min freq: 100 KHz */
#define FIFO_READY_TIMEOUT_US       50000U      /* Timeout for reading/writing the FIFO */
#define XFER_COMPLETED_TIMEOUT_US   50000U      /* Timeout for completing the transfer after sending all data to the FIFO */

/*--------------------------------------------------------
 * GLOBALS
 *------------------------------------------------------*/
static bool use_lsb_first = false;

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS PROTOTYPES
 *------------------------------------------------------*/
static bool cs_is_valid(unsigned int cs);
static bool cs_activate(uintptr_t spi_base, unsigned int cs);
static bool cs_deactivate(uintptr_t spi_base, unsigned int cs);
static bool set_mode(uintptr_t spi_base, unsigned int mode);
static bool set_speed(uintptr_t spi_base, unsigned int clock_freq, unsigned int hz);
static bool send_bytes(uintptr_t spi_base, const uint8_t *buf, size_t len);
static bool receive_bytes(uintptr_t spi_base, uint8_t *buf, size_t len);

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS
 *------------------------------------------------------*/
static bool cs_is_valid(unsigned int cs)
{
	if (cs < SPI_MAX_CHIP_SELECTS)
		return true;
	return false;
}

static bool cs_activate(uintptr_t spi_base, unsigned int cs)
{
	if (!cs_is_valid(cs)) {
		ERROR("%s: Invalid chip select %d\n", __func__, cs);
		return false;
	}

	/* Enable cs (Note: Required double write to get result on SLVSEL port)*/
	mmio_setbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_ENABLE(cs));
	mmio_setbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_ENABLE(cs));

	/* Assert cs  (Note: Required double write to get result on SLVSEL port)*/
	mmio_clrbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_VALUE(cs));
	mmio_clrbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_VALUE(cs));

	return true;
}

static bool cs_deactivate(uintptr_t spi_base, unsigned int cs)
{
	if (!cs_is_valid(cs)) {
		ERROR("%s: Invalid chip select %d\n", __func__, cs);
		return false;
	}

	/* Disable cs (Note: Required double write to get result on SLVSEL port)*/
	mmio_clrbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_ENABLE(cs));
	mmio_clrbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_ENABLE(cs));

	/* Deassert cs  (Note: Required double write to get result on SLVSEL port)*/
	mmio_setbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_VALUE(cs));
	mmio_setbits_32(spi_base + SPI_SLVSEL, BIT_SSEL_CS_VALUE(cs));

	return true;
}

static bool set_mode(uintptr_t spi_base, unsigned int mode)
{
	if ((mode & SPI_CS_HIGH) != 0U) {
		ERROR("%s: Chip select high not supported\n", __func__);
		return false;
	}

	if ((mode & SPI_CPHA) != 0U)
		mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_CPHA);
	else
		mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_CPHA);

	if ((mode & SPI_CPOL) != 0U)
		mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_CPOL);
	else
		mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_CPOL);

	use_lsb_first = ((mode & SPI_LSB_FIRST) != 0U);
	if (use_lsb_first)
		mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_LSBF);
	else
		mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_LSBF);


	VERBOSE("%s: mode=0x%x\n", __func__, mode);

	return true;
}

static bool set_speed(uintptr_t spi_base, unsigned int clock_freq, unsigned int hz)
{
	unsigned int sclk = clock_freq;
	uint32_t baud;

	if ((hz < SPI_MIN_FREQ) || (hz > sclk)) {
		ERROR("%s: Speed %d hz out of range (%d, %d)\n", __func__, hz, SPI_MIN_FREQ, sclk);
		return false;
	}

	/* Baud rate formula from spec */
	baud = (sclk / hz) - 1;

	/* Ensure that configured speed is equal or lower than requested */
	if ((sclk % hz) != 0)
		baud++;

	mmio_write_32(spi_base + SPI_CLK, baud);

	VERBOSE("%s: baud=%u\n", __func__, baud);

	return true;
}

static bool send_bytes(uintptr_t spi_base, const uint8_t *buf, size_t len)
{
	bool ret = true;
	size_t i;
	uint64_t timeout;
	uint32_t value = 0;

	/* Check len */
	if (len > MAX_TRANSMITTED_WORD_COUNT) {
		ERROR("%s: len > max transmitted word counter (%u)\n", __func__, MAX_TRANSMITTED_WORD_COUNT);
		return false;
	}

	/* - Clear TXCTL */
	mmio_write_32(spi_base + SPI_TXCTL, 0);
	/* - Clear STAT bits */
	mmio_setbits_32(spi_base + SPI_STAT, 0xFFFFFFFF);
	/* - Set bytes to send */
	mmio_clrbits_32(spi_base + SPI_TXCTL, SPI_TXCTL_TWCEN); /* Disable Transmit Word Counter */
	mmio_write_32(spi_base + SPI_TWC, len);                 /* Transmitted Word Count Register set to len */
	mmio_write_32(spi_base + SPI_TWCR, 0);                  /* Transmitted Word Count Reload Register set to 0 */
	mmio_write_32(spi_base + SPI_TXCTL, SPI_TXCTL_TWCEN);   /* Enable Transmit Word Counter */
	/* - Enable Tx (and set tx initiator) */
	mmio_setbits_32(spi_base + SPI_TXCTL, SPI_TXCTL_TTI | SPI_TXCTL_TEN);
	/* - Send bytes */
	for (i = 0; i < len; i++) {
		/* - Wait for Tx FIFO not full before inserting the next byte */
		timeout = timeout_init_us(FIFO_READY_TIMEOUT_US);
		while (mmio_read_32(spi_base + SPI_STAT) & SPI_STAT_TFF) {
			if (timeout_elapsed(timeout)) {
				ERROR("%s: send bytes fifo timeout\n", __func__);
				ret = false;
				goto send_bytes_end;
			}
		}
		/* - Send data byte */
		value = (uint32_t)buf[i];
		mmio_write_32(spi_base + SPI_TFIFO, value);
		VERBOSE("%s: %x\n", __func__, buf[i]);
	}
	/* - Wait for transmission to complete */
	timeout = timeout_init_us(XFER_COMPLETED_TIMEOUT_US);
	while ((mmio_read_32(spi_base + SPI_STAT) & SPI_STAT_TF) != SPI_STAT_TF) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: send bytes finish timeout\n", __func__);
			ret = false;
			goto send_bytes_end;
		}
	}

send_bytes_end:
	/* - Disable Tx */
	mmio_write_32(spi_base + SPI_TXCTL, 0x0);

	return ret;
}

static bool receive_bytes(uintptr_t spi_base, uint8_t *buf, size_t len)
{
	bool ret = true;
	size_t i;
	uint64_t timeout;
	uint32_t value = 0x0;

	/* Check len */
	if (len > MAX_RECEIVED_WORD_COUNT) {
		ERROR("%s: len > max received word counter (%u)\n", __func__, MAX_RECEIVED_WORD_COUNT);
		return false;
	}

	/* - Clear RXCTL */
	mmio_write_32(spi_base + SPI_RXCTL, 0);
	/* - Clear STAT bits */
	mmio_setbits_32(spi_base + SPI_STAT, 0xFFFFFFFF);
	/* - Set bytes to read.*/
	mmio_clrbits_32(spi_base + SPI_RXCTL, SPI_RXCTL_RWCEN);         /* Disable Received Word Counter */
	mmio_write_32(spi_base + SPI_RWC, len);                         /* Received Word Count Register set to len */
	mmio_write_32(spi_base + SPI_RWCR, 0);                          /* Received Word Count Reload Register set to 0 */
	/* Clean Rx FIFO */
	timeout = timeout_init_us(FIFO_READY_TIMEOUT_US);
	while (!(mmio_read_32(spi_base + SPI_STAT) & SPI_STAT_RFE)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: receive bytes fifo timeout\n", __func__);
			ret = false;
			goto receive_bytes_end;
		}
		/* Drop byte */
		value = mmio_read_32(spi_base + SPI_RFIFO);
	}

	/* - Enable Rx  (and set rx initiator) */
	mmio_write_32(spi_base + SPI_RXCTL, SPI_RXCTL_RTI | SPI_RXCTL_RWCEN);
	/* - Enable receiver */
	mmio_setbits_32(spi_base + SPI_RXCTL, SPI_RXCTL_REN);
	/* = Receive */
	for (i = 0; i < len; i++) {
		/* - wait for Rx FIFO not empty before reading the next byte */
		timeout = timeout_init_us(FIFO_READY_TIMEOUT_US);
		while ((mmio_read_32(spi_base + SPI_STAT) & SPI_STAT_RFE)) {
			if (timeout_elapsed(timeout)) {
				ERROR("%s: receive bytes fifo timeout\n", __func__);
				ret = false;
				goto receive_bytes_end;
			}
		}
		value = mmio_read_32(spi_base + SPI_RFIFO);
		buf[i] = (uint8_t)(value & 0xff);
		VERBOSE("%s: %x\n", __func__, buf[i]);
	}

receive_bytes_end:
	/* - Disable Rx */
	mmio_write_32(spi_base + SPI_RXCTL, 0x0);

	return ret;
}

/*--------------------------------------------------------
 * EXPORTED FUNCTIONS
 *------------------------------------------------------*/
bool adi_raw_spi_init(uintptr_t spi_base, unsigned int mode, unsigned int clock_freq, unsigned int hz)
{
	/* Clear control register */
	mmio_write_32(spi_base + SPI_CTL, 0);

	/* Set master enable.*/
	mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_EN);        /* Disable SPI */
	mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_MSTR);      /* Enable SPI Master */
	mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_EN);        /* Enable SPI */

	/* Set Mode and Speed */
	if (!set_mode(spi_base, mode)) return false;
	if (!set_speed(spi_base, clock_freq, hz)) return false;

	/* Use FW controlled chip select.*/
	mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_ASSEL);

	/* Bus width 1 line, start on MOSI and clear STAT bits */
	mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_SOSI);
	mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_MIOM_MASK);
	mmio_setbits_32(spi_base + SPI_STAT, 0xFFFFFFFF);

	return true;
}

bool adi_raw_spi_write(uintptr_t spi_base, uint8_t cs, uint8_t address, const uint8_t *buf, size_t len)
{
	uint8_t cmd_address;

	/* Enable SPI */
	mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_EN);

	if (!cs_activate(spi_base, cs)) return false;

	/* Build command/address */
	if (use_lsb_first)
		cmd_address = (address << 1) & ~0x01;   /* Indicate Write command, LSB */
	else
		cmd_address = address & ~0x80;          /* Indicate Write command, MSB */

	/* Send command/address */

	if (!send_bytes(spi_base, &cmd_address, 1)) return false;

	/* Send Data */
	if (!send_bytes(spi_base, buf, len)) return false;

	/* Dectivate chip select */
	if (!cs_deactivate(spi_base, cs)) return false;

	/* Disable SPI */
	mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_EN);

	return true;
}

bool adi_raw_spi_read(uintptr_t spi_base, uint8_t cs, uint8_t address, uint8_t *buf, size_t len)
{
	uint8_t cmd_address;

	/* Enable SPI */
	mmio_setbits_32(spi_base + SPI_CTL, SPI_CTL_EN);

	if (!cs_activate(spi_base, cs)) return false;

	/* Build command/address */
	if (use_lsb_first)
		cmd_address = (address << 1) | 0x01;    /* Indicate Read command, LSB */
	else
		cmd_address = address | 0x80;           /* Indicate Read command, MSB */

	/* Send command/address */
	if (!send_bytes(spi_base, &cmd_address, 1)) return false;

	/* Receive Data */
	if (!receive_bytes(spi_base, buf, len)) return false;

	/* Dectivate chip select */
	if (!cs_deactivate(spi_base, cs)) return false;

	/* Disable SPI */
	mmio_clrbits_32(spi_base + SPI_CTL, SPI_CTL_EN);

	return true;
}

void adi_raw_spi_deinit(uintptr_t spi_base)
{
	/* Disable and Deassert all chip selects */
	mmio_write_32(spi_base + SPI_SLVSEL, 0xFE00);
	mmio_write_32(spi_base + SPI_SLVSEL, 0xFE00);

	/* Disable transmitter, receiver */
	mmio_write_32(spi_base + SPI_TXCTL, 0);
	mmio_write_32(spi_base + SPI_RXCTL, 0);

	/* Clear control register */
	mmio_write_32(spi_base + SPI_CTL, 0);
}
