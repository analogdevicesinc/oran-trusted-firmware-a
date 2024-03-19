/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLL_H
#define PLL_H
/* Enumeration for PLLs */
typedef enum {
	PLL_CLKGEN_PLL,         /*!< Ref for the Clock Gen PLL */
	PLL_SEC_CLKGEN_PLL,     /*!< Ref for the Secondary Clock Gen PLL */
	PLL_LAST_PLL            /*!< Ref for last COMMON PLL  */
}
PllSelName_e;

extern int pll_program_clock_pll_clock(const uint64_t base, PllSelName_e pll);
extern int pll_clk_power_init(const uint64_t base, const uint64_t dig_core_base, const uint64_t freq, const uint32_t refclk_freq, PllSelName_e pll);
extern void pll_configure_vco_test_out(const uint64_t base);
extern void pll_configure_vtune_test_out(const uint64_t base);

#endif /* PLL_H */
