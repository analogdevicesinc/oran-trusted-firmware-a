/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADRV906X_DUAL_H__
#define __ADRV906X_DUAL_H__

#include <stdbool.h>

bool adrv906x_c2c_enable(void);
bool adrv906x_c2c_enable_high_speed(void);
int adrv906x_enable_secondary_tile(void);
void adrv906x_release_secondary_reset(void);
void adrv906x_activate_secondary_reset(void);
int adrv906x_load_secondary_image(void);

#endif /* __ADRV906X_DUAL_H__ */
