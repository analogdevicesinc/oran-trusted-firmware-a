/*
 * Copyright (c) 2017-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SYSTEMC_PL180_H
#define SYSTEMC_PL180_H

#include <stdbool.h>

#include <drivers/mmc.h>

struct systemc_pl180_params {
	uintptr_t reg_base;
	unsigned int clk_rate;
	unsigned int bus_width;
	unsigned int flags;
	struct mmc_device_info *device_info;
	unsigned int pin_ckin;
	unsigned int negedge;
	unsigned int dirpol;
	unsigned int clock_id;
	unsigned int reset_id;
	unsigned int max_freq;
	bool use_dma;
};

unsigned long long systemc_pl180_mmc_get_device_size(void);
int systemc_pl180_mmc_init(struct systemc_pl180_params *params);
bool systemc_pl180_mmc_use_dma(unsigned int instance, unsigned int memory);

#endif /* SYSTEMC_PL180_H */
