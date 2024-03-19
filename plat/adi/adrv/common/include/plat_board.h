/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_BOARD_H
#define PLAT_BOARD_H

#include <drivers/partition/partition.h>

/* Returns a partition_entry_t for the given NOR flash partition name.
 * Returns NULL if no partition exists
 */
const partition_entry_t *plat_get_nor_part_entry(const char *name);

/* Get NOR flash device configuration.
 * Returns 0 on success, negative value otherwise
 *
 * Declared in spi_nor.h. Included here for reference only.
 * int plat_get_nor_data(struct nor_device *device);
 */

#endif
