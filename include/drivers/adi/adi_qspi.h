/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_QSPI_H
#define ADI_QSPI_H

#include <stdbool.h>
#include <stdint.h>

struct adi_qspi_ctrl {
	uintptr_t reg_base;
	uintptr_t tx_dde_reg_base;
	uintptr_t rx_dde_reg_base;
	unsigned int clock_freq;
	unsigned int cs;
	unsigned int spi_clk_freq;
	int mode;
	bool dma;
};

int adi_qspi_init(struct adi_qspi_ctrl *params);
void adi_qspi_deinit(uintptr_t qspi_base, uintptr_t tx_dde_base, uintptr_t rx_dde_base);

#endif /* ADI_QSPI_H */
