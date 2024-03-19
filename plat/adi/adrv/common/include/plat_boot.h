/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_BOOT_H
#define PLAT_BOOT_H

/* Boot device IDs */
typedef enum {
	PLAT_BOOT_DEVICE_SD_0,
	PLAT_BOOT_DEVICE_EMMC_0,
	PLAT_BOOT_DEVICE_QSPI_0,
	PLAT_BOOT_DEVICE_HOST,
	PLAT_BOOT_DEVICE_NUM,
} plat_boot_device_t;

/* Get the current boot device */
plat_boot_device_t plat_get_boot_device(void);

/* Initialize the boot device */
void plat_init_boot_device(void);

/* Get the address to be used for host boot */
uintptr_t plat_get_host_boot_addr(void);

/* Get string ID for the specified boot device */
const char * plat_get_boot_device_str(plat_boot_device_t boot_dev);

#endif /* PLAT_BOOT_H */
