/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_DEVICE_PROFILE_H
#define PLAT_DEVICE_PROFILE_H
#include <stddef.h>

/* Declared in platform.h, but mentioned here for consistency.
 * unsigned int plat_get_syscnt_freq2(void);
 */

const char *plat_get_boot_slot(void);
void plat_set_boot_slot(const char *slot);

size_t plat_get_dram_size(void);

#endif /* PLAT_DEVICE_PROFILE_H */
