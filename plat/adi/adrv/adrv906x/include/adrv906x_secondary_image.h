/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_SECONDARY_IMAGE_H
#define ADRV906X_SECONDARY_IMAGE_H

#include <stdint.h>

#include <adrv906x_def.h>

extern unsigned char plat_secondary_image[] __attribute__((weak));
extern unsigned char plat_secondary_image_end[] __attribute__((weak));

/* Location of secondary image embedded in primary image */
#define PLAT_SEC_IMAGE_SRC_ADDR  (&plat_secondary_image)

/* Load address of secondary image on secondary */
#define PLAT_SEC_IMAGE_DST_ADDR SEC_SRAM_BASE

#define PLAT_SEC_IMAGE_SIZE (plat_secondary_image_end - plat_secondary_image)

/* Boot config data is placed at the 5th (last) MB of secondary SRAM */
#define PLAT_SEC_BOOT_CFG_ADDR ((SEC_SRAM_BASE)+0x00500000)

#define PLAT_SEC_BOOT_MAGIC 0xAD12B007

typedef struct __packed {
	uint64_t magic;
	uint64_t kernel_cfg_addr;
	uint64_t syscnt_freq;
	uint64_t uart_clk_freq;
} plat_sec_boot_cfg_t;

#endif /* ADRV906X_SECONDARY_IMAGE_H */
