/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLK_SWITCH_PHASES_H
#define CLK_SWITCH_PHASES_H

#include <stdint.h>
#include <stdbool.h>

#include <drivers/adi/adrv906x/clk.h>

void clk_set_src_pre_switch(const uintptr_t baseaddr, const clk_src_t clk_src);
void clk_set_src_switch(const uintptr_t baseaddr, const clk_src_t clk_src, bool use_device_clk_until_mcs);
void clk_set_src_post_switch(const uintptr_t baseaddr, const clk_src_t clk_src);

#endif /* CLK_SWITCH_PHASES_H */
