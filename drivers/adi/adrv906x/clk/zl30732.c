/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * Note:
 * - The ZL30732 allows self-configuration at power-up from internal flash memory.
 * - In Adrv906x, the ZL30732 flash will be programmed with the default configuration,
 *   so this driver just has to control the two outputs used as MCS sysref signals.
 */

#include <stdbool.h>

#include <common/debug.h>

#include <drivers/adi/adi_raw_spi.h>
#include <drivers/adi/adrv906x/zl30732.h>

#define ZL30732_PAGE_ADDR               0x7F
#define ZL30732_PAGE_SIZE               0x80
#define ZL30732_OUTPUTS_PAGE            9
#define ZL30732_OUTPUT_CTRL_2_ADDR      0x04AA
#define ZL30732_OUTPUT_CTRL_3_ADDR      0x04AB
#define ZL30732_OUTPUT_STOP_BIT_MASK    0x02

#define ZL30732_OUTPUT_MCS_1_OFFSET     (ZL30732_OUTPUT_CTRL_2_ADDR - (ZL30732_PAGE_SIZE * ZL30732_OUTPUTS_PAGE))
#define ZL30732_OUTPUT_MCS_2_OFFSET     (ZL30732_OUTPUT_CTRL_3_ADDR - (ZL30732_PAGE_SIZE * ZL30732_OUTPUTS_PAGE))

#define ZL30732_SPI_SPEED_HZ            (10U * 1000U * 1000U)

/**
 * adi_zl30732_init - Inits the SPI master peripheral
 */
bool adi_zl30732_init(uintptr_t spi_base, unsigned int clock_freq)
{
	unsigned int mode = 0;
	unsigned int speed = ZL30732_SPI_SPEED_HZ;

	return adi_raw_spi_init(spi_base, mode, clock_freq, speed);
}

/**
 * adi_zl30732_sysref_enable - Enables the two MCS outputs of the zl30732 chip
 */
bool adi_zl30732_sysref_enable(uintptr_t spi_base, uint8_t cs, bool dual_tile)
{
	uint8_t page = ZL30732_OUTPUTS_PAGE;
	uint8_t ctrl_mcs_1;
	uint8_t ctrl_mcs_2;

	/* Select outputs control page */
	if (!adi_raw_spi_write(spi_base, cs, ZL30732_PAGE_ADDR, &page, 1)) {
		ERROR("Cannot enable ZL30732 sysref. Error selecting outputs control page.\n");
		return false;
	}

	/* Start MCS 1 output */
	/* - read output ctrl registers */
	if (!adi_raw_spi_read(spi_base, cs, ZL30732_OUTPUT_MCS_1_OFFSET, &ctrl_mcs_1, 1)) {
		ERROR("Cannot enable ZL30732 sysref. Error reading MCS 1 output control register.\n");
		return false;
	}
	/* - disable stop bit */
	ctrl_mcs_1 &= ~ZL30732_OUTPUT_STOP_BIT_MASK;
	/* - write output ctrl register */
	if (!adi_raw_spi_write(spi_base, cs, ZL30732_OUTPUT_MCS_1_OFFSET, &ctrl_mcs_1, 1)) {
		ERROR("Cannot enable ZL30732 sysref. Error writting MCS 1 output control register.\n");
		return false;
	}

	if (dual_tile) {
		/* Start MCS 2 output */
		/* - read output ctrl register */
		if (!adi_raw_spi_read(spi_base, cs, ZL30732_OUTPUT_MCS_2_OFFSET, &ctrl_mcs_2, 1)) {
			ERROR("Cannot enable ZL30732 sysref. Error reading MCS 2 output control register.\n");
			return false;
		}
		/* - disable stop bit */
		ctrl_mcs_2 &= ~ZL30732_OUTPUT_STOP_BIT_MASK;
		/* - write output ctrl register */
		if (!adi_raw_spi_write(spi_base, cs, ZL30732_OUTPUT_MCS_2_OFFSET, &ctrl_mcs_2, 1)) {
			ERROR("Cannot enable ZL30732 sysref. Error writting MCS 2 output control register.\n");
			return false;
		}
	}

	return true;
}

/**
 * adi_zl30732_sysref_disable - Disables the two MCS outputs of the zl30732 chip
 */
bool adi_zl30732_sysref_disable(uintptr_t spi_base, uint8_t cs, bool dual_tile)
{
	uint8_t page = ZL30732_OUTPUTS_PAGE;
	uint8_t ctrl_mcs_1;
	uint8_t ctrl_mcs_2;

	/* Select outputs control page */
	if (!adi_raw_spi_write(spi_base, cs, ZL30732_PAGE_ADDR, &page, 1)) {
		ERROR("Cannot disable ZL30732 sysref. Error selecting outputs control page.\n");
		return false;
	}

	/* Stop MCS 1 output */
	/* - read output ctrl register */
	if (!adi_raw_spi_read(spi_base, cs, ZL30732_OUTPUT_MCS_1_OFFSET, &ctrl_mcs_1, 1)) {
		ERROR("Cannot disable ZL30732 sysref. Error reading MCS 1 output control register.\n");
		return false;
	}
	/* - enable stop bit */
	ctrl_mcs_1 |= ZL30732_OUTPUT_STOP_BIT_MASK;
	/* - write output ctrl register */
	if (!adi_raw_spi_write(spi_base, cs, ZL30732_OUTPUT_MCS_1_OFFSET, &ctrl_mcs_1, 1)) {
		ERROR("Cannot disable ZL30732 sysref. Error writting MCS 1 output control register.\n");
		return false;
	}

	if (dual_tile) {
		/* Stop MCS 2 output */
		/* - read output ctrl register */
		if (!adi_raw_spi_read(spi_base, cs, ZL30732_OUTPUT_MCS_2_OFFSET, &ctrl_mcs_2, 1)) {
			ERROR("Cannot disable ZL30732 sysref. Error reading MCS 2 output control register.\n");
			return false;
		}
		/* - enable stop bit */
		ctrl_mcs_2 |= ZL30732_OUTPUT_STOP_BIT_MASK;
		/* - write output ctrl register */
		if (!adi_raw_spi_write(spi_base, cs, ZL30732_OUTPUT_MCS_2_OFFSET, &ctrl_mcs_2, 1)) {
			ERROR("Cannot disable ZL30732 sysref. Error writting MCS 2 output control register.\n");
			return false;
		}
	}

	return true;
}
