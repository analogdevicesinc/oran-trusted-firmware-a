/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/extensions/ras_arch.h>

#include <plat_ras.h>

#define CACHE_ECC_ENABLE_ALL_ERRORS 0x10C

void plat_enable_cache_ecc(void)
{
	uint32_t err_ctrl;

	/* Enable all ECC error reporting for the L1/L2 caches*/
	write_errselr_el1(0);
	err_ctrl = read_erxctlr_el1();
	err_ctrl |= CACHE_ECC_ENABLE_ALL_ERRORS;

	write_erxctlr_el1(err_ctrl);

	/* Enable all ECC error reporting for the L3 cache*/
	write_errselr_el1(1);
	err_ctrl = read_erxctlr_el1();
	err_ctrl |= CACHE_ECC_ENABLE_ALL_ERRORS;

	write_erxctlr_el1(err_ctrl);
}
