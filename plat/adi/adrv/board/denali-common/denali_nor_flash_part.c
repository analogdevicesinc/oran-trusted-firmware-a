/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <string.h>

#include <plat_board.h>
#include <platform_nor_flash_part_def.h>

const partition_entry_t *plat_get_nor_part_entry(const char *name)
{
	int i;

	for (i = 0; i < NOR_PART_COUNT; i++)
		if (!strcmp(name, nor_part_info_list[i].name))
			return &nor_part_info_list[i];

	return NULL;
}
