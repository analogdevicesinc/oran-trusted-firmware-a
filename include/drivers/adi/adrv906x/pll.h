/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_PLL_H
#define ADRV906X_PLL_H

/* Enumeration for PLLs */
typedef enum {
	PLL_CLKGEN_PLL,         /*!< Ref for the Clock Gen PLL */
	PLL_SEC_CLKGEN_PLL,     /*!< Ref for the Secondary Clock Gen PLL */
	PLL_ETHERNET_PLL,       /*!< Ref for the Ethernet PLL */
	PLL_SEC_ETHERNET_PLL,   /*!< Ref for the Secondary Ethernet PLL */
	PLL_LAST_PLL            /*!< Ref for last COMMON PLL  */
}
PllSelName_e;

int pll_program(const uint64_t base, PllSelName_e pll);
void pll_power_ctrl(PllSelName_e pll, uint8_t state, const uint64_t base, const uint64_t dig_core_base);
int pll_clk_power_init(const uint64_t base, const uint64_t dig_core_base, const uint64_t freq, const uint32_t refclk_freq, PllSelName_e pll);
void pll_configure_vco_test_out(const uint64_t base);
void pll_configure_vtune_test_out(const uint64_t base);

#endif /* ADRV906X_PLL_H */
