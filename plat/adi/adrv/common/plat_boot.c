/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <common/debug.h>
#include <plat_boot.h>

/* String table for boot modes. MUST MATCH plat_boot_device_t enum. */
static const char *boot_mode_str_tbl[] = {
	"sd0",
	"emmc0",
	"qspi0",
	"host",
	"invalid"
};
CASSERT((sizeof(boot_mode_str_tbl) / sizeof(char *)) == (PLAT_BOOT_DEVICE_NUM + 1), plat_boot_device_t_boot_mode_str_tbl_mismatch);

const char * plat_get_boot_device_str(plat_boot_device_t boot_dev)
{
	if (boot_dev > PLAT_BOOT_DEVICE_NUM)
		boot_dev = PLAT_BOOT_DEVICE_NUM;

	return boot_mode_str_tbl[boot_dev];
}
