/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_MMAP_H
#define ADRV906X_MMAP_H

int plat_setup_secondary_mmap(bool device_region_only);
int plat_setup_ns_sram_mmap(void);

#endif /* ADRV906X_MMAP_H */
