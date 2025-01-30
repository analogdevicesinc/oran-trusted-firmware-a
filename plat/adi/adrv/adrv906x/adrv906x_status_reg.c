/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>

#include <common/debug.h>
#include <lib/mmio.h>

#include <adrv906x_def.h>
#include <adrv906x_status_reg.h>
#include <plat_err.h>
#include <plat_status_reg.h>

/* These values MUST MATCH the implementation in the following repos
 *
 * U-boot: /arch/arm/mach-adrv906x/adrv906x_status_reg.c
 * Linux: /drivers/soc/adi/adrv906x-err.c
 * OP-TEE os: /core/drivers/adi_adrv906x_status_reg.c
 */
#define RESET_CAUSE_NS_OFFSET              0
#define RESET_CAUSE_OFFSET              0
#define BOOT_CNT_OFFSET                 4
#define STARTING_SLOT_OFFSET            8
#define LAST_SLOT_OFFSET                12

/* Read from specified boot status register */
uint32_t plat_rd_status_reg(plat_status_reg_id_t reg)
{
	switch (reg) {
	case RESET_CAUSE_NS:
		return mmio_read_32(A55_SYS_CFG + SCRATCH_NS + RESET_CAUSE_NS_OFFSET);

	case RESET_CAUSE:
		return mmio_read_32(A55_SYS_CFG + SCRATCH + RESET_CAUSE_OFFSET);

	case BOOT_CNT:
		return mmio_read_32(A55_SYS_CFG + SCRATCH + BOOT_CNT_OFFSET);

	case STARTING_SLOT:
		return mmio_read_32(A55_SYS_CFG + SCRATCH + STARTING_SLOT_OFFSET);

	case LAST_SLOT:
		return mmio_read_32(A55_SYS_CFG + SCRATCH + LAST_SLOT_OFFSET);

	default:
		plat_warn_message("Not a valid status register");
		return 0;
	}
}

/* Write value to specified boot status register */
bool plat_wr_status_reg(plat_status_reg_id_t reg, uint32_t value)
{
	switch (reg) {
	case RESET_CAUSE_NS:
		mmio_write_32(A55_SYS_CFG + SCRATCH_NS + RESET_CAUSE_NS_OFFSET, value);
		break;

	case RESET_CAUSE:
		mmio_write_32(A55_SYS_CFG + SCRATCH + RESET_CAUSE_OFFSET, value);
		break;

	case BOOT_CNT:
		mmio_write_32(A55_SYS_CFG + SCRATCH + BOOT_CNT_OFFSET, value);
		break;

	case STARTING_SLOT:
		mmio_write_32(A55_SYS_CFG + SCRATCH + STARTING_SLOT_OFFSET, value);
		break;

	case LAST_SLOT:
		mmio_write_32(A55_SYS_CFG + SCRATCH + LAST_SLOT_OFFSET, value);
		break;

	default:
		plat_warn_message("Not a valid status register");
		return false;
	}
	return true;
}

/* Get the reset cause string */
const char *plat_get_reset_cause_str(reset_cause_t cause)
{
	switch (cause) {
	case RESET_VALUE:
		return "RESET VALUE";
	case IMG_VERIFY_FAIL:
		return "IMG VERIFY FAIL";
	case WATCHDOG_RESET:
		return "WATCHDOG RESET";
	case CACHE_ECC_ERROR:
		return "CACHE ECC ERROR";
	case DRAM_ECC_ERROR:
		return "DRAM ECC ERROR";
	case DRAM_INIT_ERROR:
		return "DRAM INIT ERROR";
	case MCS_FAIL:
		return "MCS FAIL";
	case MBIAS_CAL_FAIL:
		return "MBIAS CAL FAIL";
	case OTHER_RESET_CAUSE:
		return "OTHER";
	default:
		return "UNKNOWN";
	}
}
