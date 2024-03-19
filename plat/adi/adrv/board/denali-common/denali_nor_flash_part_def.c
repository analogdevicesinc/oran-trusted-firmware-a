/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <denali_nor_flash_part_def.h>

partition_entry_t nor_part_info_list[NOR_PART_COUNT] = {
	{
		.start = NOR_BOOT_A_START,
		.length = NOR_BOOT_SIZE,
		.name = BOOT_A,
	},
	{
		.start = NOR_BOOT_B_START,
		.length = NOR_BOOT_SIZE,
		.name = BOOT_B,
	},
	{
		.start = NOR_BOOTCTRL_START,
		.length = NOR_BOOTCTRL_SIZE,
		.name = BOOTCTRL,
	},
	{
		.start = NOR_BOOTCFG_START,
		.length = NOR_BOOTCFG_SIZE,
		.name = BOOTCFG,
	},
	{
		.start = NOR_FIP_A_START,
		.length = NOR_FIP_SIZE,
		.name = FIP_A,
	},
	{
		.start = NOR_FIP_B_START,
		.length = NOR_FIP_SIZE,
		.name = FIP_B,
	},
	{
		.start = NOR_KERNEL_A_START,
		.length = NOR_KERNEL_SIZE,
		.name = KERNEL_A,
	},
	{
		.start = NOR_KERNEL_B_START,
		.length = NOR_KERNEL_SIZE,
		.name = KERNEL_B,
	},
};
