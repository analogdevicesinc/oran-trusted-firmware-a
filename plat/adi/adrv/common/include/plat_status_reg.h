/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
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
 * OP-TEE os: /core/drivers/adi_adrv906x_status_reg.c
 */
typedef enum {
	RESET_VALUE,
	IMG_VERIFY_FAIL,
	WATCHDOG_RESET,
	OTHER_RESET_CAUSE,
} reset_cause_t;

/* This enum MUST MATCH the enumeration found in the following repos
 *
 * U-boot: /arch/arm/mach-adrv906x/include/plat_status_reg.h
 * OP-TEE os: /core/include/drivers/adi_adrv906x_status_reg.h
 */
typedef enum {
	RESET_CAUSE_NS,
	RESET_CAUSE,
	BOOT_CNT,
	STARTING_SLOT,
	LAST_SLOT,
	RECOVERY_BOOT_ACTIVE,
} plat_status_reg_id_t;

uint32_t plat_rd_status_reg(plat_status_reg_id_t reg);

bool plat_wr_status_reg(plat_status_reg_id_t reg, uint32_t value);

#endif
