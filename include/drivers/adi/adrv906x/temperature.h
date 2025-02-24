/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_TEMPERATURE_H
#define ADRV906X_TEMPERATURE_H

void tempr_init(void);
extern int tempr_read(float *clkpll_temp, float *ethpll_temp);
extern int tempr_read_secondary(float *sec_clkpll_temp, float *sec_ethpll_temp);

#endif /* ADRV906X_TEMPERATURE_H */
