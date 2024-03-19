/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LDO_H
#define LDO_H

extern void ldo_powerdown(const uint64_t base);
extern int ldo_powerup(const uint64_t base);

#endif /* LDO_H */
