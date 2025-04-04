/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_STATUS_REG_H
#define PLAT_STATUS_REG_H

/*
 * List of reasons reset was performed which gets stored in RESET_CAUSE
 * This enum MUST MATCH those defined in the following repos
 *
 * U-boot: /arch/arm/mach-adrv906x/include/plat_status_reg.h
 * Linux: /drivers/soc/adi/adrv906x-err.c
 * OP-TEE os: /core/drivers/adi/adrv906x/adrv906x_status_reg.c
 */
typedef enum {
	COLD_BOOT,
	WARM_RESET,
	IMG_VERIFY_FAIL,
	WATCHDOG_RESET,
	CACHE_ECC_ERROR,
	DRAM_ECC_ERROR,
	DRAM_INIT_ERROR,
	MCS_FAIL,
	MBIAS_CAL_FAIL,
	OTHER_RESET_CAUSE,
} reset_cause_t;

/* This enum MUST MATCH the enumeration found in the following repos
 *
 * U-boot: /arch/arm/mach-adrv906x/include/plat_status_reg.h
 * OP-TEE os: /core/include/drivers/adi/adrv906x/adrv906x_status_reg.h
 */
typedef enum {
	RESET_CAUSE_NS,
	RESET_CAUSE,
	BOOT_CNT,
	STARTING_SLOT,
	LAST_SLOT
} plat_status_reg_id_t;

uint32_t plat_rd_status_reg(plat_status_reg_id_t reg);
bool plat_wr_status_reg(plat_status_reg_id_t reg, uint32_t value);
const char *plat_get_reset_cause_str(reset_cause_t cause);

#endif
