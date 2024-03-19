/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_PERIPHERAL_CLK_RST_H
#define ADRV906X_PERIPHERAL_CLK_RST_H

/* Peripheral deinit functions for pre clk switch */
void plat_pre_clk_switch(void);

/* Peripheral init functions for post clk switch */
void plat_post_clk_switch(void);

#endif /* ADRV906X_PERIPHERAL_CLK_RST_H */
