/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LDO_H
#define LDO_H

#include <drivers/adi/adrv906x/pll.h>

void ldo_powerdown(const uint64_t base);
int ldo_powerup(const uint64_t base, PllSelName_e pll);

#endif /* LDO_H */
