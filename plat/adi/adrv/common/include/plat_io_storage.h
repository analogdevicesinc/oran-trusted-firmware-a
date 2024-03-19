/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_IO_STORAGE_H
#define PLAT_IO_STORAGE_H

#include <drivers/io/io_storage.h>

void plat_io_setup(bool use_bootctrl, uint32_t reset_cause, uint32_t reset_cause_ns);
uintptr_t plat_get_boot_handle(void);
int plat_get_partition_spec(const char *partition_id, io_block_spec_t *spec);

#endif /* PLAT_IO_STORAGE_H */
