/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_SDHCI_H
#define ADI_SDHCI_H

#include <stdbool.h>
#include <stdint.h>

/* Structure containing MMC parameters */
struct adi_mmc_params {
	uintptr_t reg_base;
	uintptr_t phy_reg_base;
	unsigned int clk_rate;
	unsigned int bus_width;
	unsigned int flags;
	unsigned int src_clk_hz;
	struct mmc_device_info *device_info;
	bool use_dma;
	bool phy_config_needed;
};

int adi_mmc_init(struct adi_mmc_params *params);

int adi_mmc_deinit(uintptr_t reg_base);

#endif /* ADI_SDHCI_H */
