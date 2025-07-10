/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_SRAM_H
#define ADRV906X_SRAM_H

#include <adrv906x_sram_def.h>

void l4_warning_info(uintptr_t base_addr);
void l4_error_info(uintptr_t base_addr);
void plat_scrub_l4(void);

#endif /* ADRV906X_SRAM_H */
