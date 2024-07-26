/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLK_H
#define CLK_H

#include <stdint.h>
#include <stdbool.h>

/* Exact clock PLL frequencies for 7GHz and 11GHz */
#define CLK_CLKPLL_FREQ_7GHZ    (7864320000LL)
#define CLK_CLKPLL_FREQ_11GHZ   (11796480000LL)

/* IDs for clock outputs from this block */
typedef enum {
	CLK_ID_CORE,    /* Core clock. Clock for A55 clusters and cores */
	CLK_ID_SYSCLK,  /* System clock. Clock for most A55 peripherals. */
	CLK_ID_TIMER,   /* Clock that feeds A55 timer */
	CLK_ID_WDT,     /* Watchdog timer clock */
	CLK_ID_DDR,     /* DDR clock*/
	CLK_ID_EMMC,    /* eMMC Card Clock */
	CLK_ID_NUM
} clk_id_t;

/* IDs for clock inputs to this block */
typedef enum {
	CLK_SRC_CLKPLL, /* Clock PLL */
	CLK_SRC_DEVCLK, /* Device clock */
	CLK_SRC_ROSC,   /* Ring oscillator */
	CLK_SRC_NUM
} clk_src_t;

/* Prototype for clock switch callback */
typedef void (*clk_switch_hook_t)(void);

void clk_init(const uintptr_t baseaddr, const uintptr_t clkpll_addr, const uintptr_t dig_core_addr, clk_switch_hook_t pre_switch_hook, clk_switch_hook_t post_switch_hook);
clk_src_t clk_get_src(const uintptr_t baseaddr);
uint64_t clk_get_freq(const uintptr_t baseaddr, const clk_id_t clk_id);
uint64_t clk_get_freq_by_src(const uintptr_t baseaddr, const clk_id_t clk_id, const clk_src_t clk_src);
void clk_set_freq_by_src(const uintptr_t baseaddr, const clk_id_t clk_id, const uint64_t freq, const clk_src_t clk_src);
void clk_set_freq(const uintptr_t baseaddr, const clk_id_t clk_id, const uint64_t freq);
void clk_set_src(const uintptr_t baseaddr, const clk_src_t clk_src);
void clk_notify_src_freq_change(const uintptr_t baseaddr, clk_src_t clk_src, const uint64_t freq);
void clk_print_info(const uintptr_t baseaddr);
void clk_enable_clock(const uintptr_t baseaddr, const clk_id_t clk_id);
void clk_disable_clock(const uintptr_t baseaddr, const clk_id_t clk_id);
bool clk_do_mcs(bool dual_tile, uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting, bool mcs_bypass);
bool clk_verify_config(uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting);
void clk_init_devclk(const uintptr_t baseaddr, const uintptr_t dig_core_addr);
int clk_initialize_pll_programming(bool secondary, bool eth_pll, uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting);
#endif /* CLK_H */
