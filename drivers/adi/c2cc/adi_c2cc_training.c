/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/adi/adi_c2cc.h>
#include "adi_c2cc_analysis.h"
#include "adi_c2cc_util.h"

/* Low speed (20 MHz) settings */
#define ADI_C2CC_TXDIV_ROSC 25
#define ADI_C2CC_TXDIV_DEVCLK 50
#define ADI_C2CC_TXDIV_PLL 50

/**
 * run the LFSR for primary-to-secondary training and collect the results.
 * @param[in] pri_base - base address for the primary c2cc (transmitter).
 * @param[in] sec_base - base address for the secondary c2cc (receiver).
 * @param[in] trim - the rx clock trim value to test.
 * @param[out] stats - the statistics for this trim value.
 * @return true if statistics were gathered, false if error.
 */
static bool adi_c2cc_p2s_gather_statistics(uintptr_t pri_base, uintptr_t sec_base, uint8_t trim, uint32_t *stats)
{
	uint64_t to = 0;

	ADI_C2CC_WRITE_RXCLK_DELAY_TRAIN(sec_base, trim); /* set the trim-to-be-tested */

	/* disable bridged AXI transactions in both directions */
	adi_c2cc_axi_control(sec_base, false);
	if (!adi_c2cc_wait_transactions(sec_base)) {
		adi_c2cc_axi_control(sec_base, true);
		return false;
	}
	adi_c2cc_axi_control(pri_base, false);
	if (!adi_c2cc_wait_transactions(pri_base)) {
		adi_c2cc_axi_control(pri_base, true);
		adi_c2cc_axi_control(sec_base, true);
		return false;
	}

	/* run the LSFR */
	ADI_C2CC_WRITE_POWERUP_CAL_START(pri_base, 1);

	to = timeout_init_us(ADI_C2C_MAX_TRAIN_WAIT);
	while (!ADI_C2CC_READ_PWR_UP_TX_CAL_IRQ(pri_base)) {
		if (timeout_elapsed(to)) {
			adi_c2cc_axi_control(pri_base, true);
			adi_c2cc_axi_control(sec_base, true);
			return false;
		}
	}

	ADI_C2CC_WRITE_PWR_UP_TX_CAL_IRQ(pri_base, 1); /* clear primary's interrupt flag */

	/* re-enable bridged AXI transactions */
	adi_c2cc_axi_control(pri_base, true);
	adi_c2cc_axi_control(sec_base, true);

	/* collect statistics (4 lanes, 2 edges each - combined in TRAINSTAT3) */
	if (stats)
		*stats = ADI_C2CC_READ_TRAINSTAT3_RAW(sec_base);

	/* Workaround: training does not work (reads all 0s) in loopback mode unless a small delay is added here */
	udelay(1);

	ADI_C2CC_WRITE_PWR_UP_RX_CAL_IRQ(sec_base, 1); /* clear secondary's interrupt flag and stats */

	return true;
}

/**
 * run the LFSR for secondary-to-primary training and collect the results.
 * @param[in] sec_base - base address for the secondary c2cc (transmitter).
 * @param[in] pri_base - base address for the primary c2cc (receiver).
 * @param[in] trim - the rx clock trim value to test.
 * @param[out] stats - the statistics for this trim value.
 * @return true if statistics were gathered, false if error.
 */
static bool adi_c2cc_s2p_gather_statistics(uintptr_t sec_base, uintptr_t pri_base, uint8_t trim, uint32_t *stats)
{
	uint64_t to = 0;

	ADI_C2CC_WRITE_RXCLK_DELAY_TRAIN(pri_base, trim); /* set the trim-to-be-tested */

	/* disable bridged AXI transactions from the secondary */
	adi_c2cc_axi_control(sec_base, false);
	if (!adi_c2cc_wait_transactions(sec_base) || !adi_c2cc_wait_transactions(pri_base)) {
		adi_c2cc_axi_control(sec_base, true);
		return false;
	}

	/* run the LSFR, then disable bridged AXI transactions from the primary */
	ADI_C2CC_WRITE_POWERUP_CAL_START(sec_base, 1);
	adi_c2cc_axi_control(pri_base, false);

	to = timeout_init_us(ADI_C2C_MAX_TRAIN_WAIT);
	while (!ADI_C2CC_READ_PWR_UP_RX_CAL_IRQ(pri_base)) {
		if (timeout_elapsed(to)) {
			adi_c2cc_axi_control(pri_base, true);
			adi_c2cc_axi_control(sec_base, true);
			return false;
		}
	}


	/* collect statistics (4 lanes, 2 edges each - combined in TRAINSTAT3) */
	if (stats)
		*stats = ADI_C2CC_READ_TRAINSTAT3_RAW(pri_base);

	/* re-enable bridged AXI transactions */
	adi_c2cc_axi_control(pri_base, true);
	adi_c2cc_axi_control(sec_base, true);

	/* clear interrupt flags and statistics */
	ADI_C2CC_WRITE_PWR_UP_TX_CAL_IRQ(sec_base, 1);
	ADI_C2CC_WRITE_PWR_UP_RX_CAL_IRQ(pri_base, 1);

	return true;
}

static void adi_c2cc_configure_tx_clk(uintptr_t pri_base, uintptr_t sec_base, struct adi_c2cc_training_clock_settings *tx_clk)
{
	/* Set low speed (20MHz) before training procedure
	 * (setting explicitly these values (which are default on reset), allows to
	 * go and back during CLI C2C testing) */
	ADI_C2CC_WRITE_ROSC_TX_CLK_DIV(sec_base, ADI_C2CC_TXDIV_ROSC);
	ADI_C2CC_WRITE_DEV_TX_CLK_DIV(sec_base, ADI_C2CC_TXDIV_DEVCLK);
	ADI_C2CC_WRITE_PLL_TX_CLK_DIV(sec_base, ADI_C2CC_TXDIV_PLL);
	ADI_C2CC_WRITE_ROSC_TX_CLK_DIV(sec_base, ADI_C2CC_TXDIV_ROSC);
	ADI_C2CC_WRITE_DEV_TX_CLK_DIV(sec_base, ADI_C2CC_TXDIV_DEVCLK);
	ADI_C2CC_WRITE_PLL_TX_CLK_DIV(sec_base, ADI_C2CC_TXDIV_PLL);

	/* Configure training speed (high speed) */
	ADI_C2CC_WRITE_TRAIN_ROSC_TX_CLK_DIV(pri_base, tx_clk->rosc_div);
	ADI_C2CC_WRITE_TRAIN_ROSC_TX_CLK_DIV(sec_base, tx_clk->rosc_div);
	ADI_C2CC_WRITE_TRAIN_DEV_TX_CLK_DIV(pri_base, tx_clk->devclk_div);
	ADI_C2CC_WRITE_TRAIN_DEV_TX_CLK_DIV(sec_base, tx_clk->devclk_div);
	ADI_C2CC_WRITE_TRAIN_PLL_TX_CLK_DIV(pri_base, tx_clk->pll_div);
	ADI_C2CC_WRITE_TRAIN_PLL_TX_CLK_DIV(sec_base, tx_clk->pll_div);

	/* Configure drive level */
	ADI_C2CC_WRITE_TX_DRIVE_LEVEL(pri_base, tx_clk->drive_level);
	ADI_C2CC_WRITE_TX_DRIVE_LEVEL(sec_base, tx_clk->drive_level);
}

static void adi_c2cc_configure_generator(uintptr_t pri_base, uintptr_t sec_base, struct adi_c2cc_training_generator_settings *generator)
{
	unsigned int i = 0;

	for (i = 0; i < ADI_C2C_LANE_COUNT; i++) {
		/* primary */
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_SEED(pri_base, i, 0, generator->seed);    /* positive edge */
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_SEED(pri_base, i, 1, generator->seed);    /* negative edge */
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_POLY(pri_base, i, 0, generator->pos_poly);
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_POLY(pri_base, i, 1, generator->neg_poly);
		ADI_C2CC_WRITE_CHECKPOLY_LFSR_POLY(pri_base, i, 0, generator->pos_poly);
		ADI_C2CC_WRITE_CHECKPOLY_LFSR_POLY(pri_base, i, 1, generator->neg_poly);
		/* secondary */
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_SEED(sec_base, i, 0, generator->seed);
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_SEED(sec_base, i, 1, generator->seed);
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_POLY(sec_base, i, 0, generator->pos_poly);
		ADI_C2CC_WRITE_TRAINPOLY_LFSR_POLY(sec_base, i, 1, generator->neg_poly);
		ADI_C2CC_WRITE_CHECKPOLY_LFSR_POLY(sec_base, i, 0, generator->pos_poly);
		ADI_C2CC_WRITE_CHECKPOLY_LFSR_POLY(sec_base, i, 1, generator->neg_poly);
	}
}

static void adi_c2cc_configure_delay(uintptr_t tx_base, uintptr_t rx_base, struct adi_c2cc_training_delay_settings *delay)
{
	ADI_C2CC_WRITE_RESP2CMD(tx_base, delay->resp2cmd);
	ADI_C2CC_WRITE_CMD2TXCLKSW(tx_base, delay->cmd2txclksw);
	ADI_C2CC_WRITE_TXCLKSW2PTRN(tx_base, delay->txclksw2ptrn);
	ADI_C2CC_WRITE_TXPTRNDUR(tx_base, delay->txptrndur);
	ADI_C2CC_WRITE_SWBCKCLK(tx_base, delay->swbckclk);
	ADI_C2CC_WRITE_TXCALDONE(tx_base, delay->caldone);
	ADI_C2CC_WRITE_RXCMD2TRNTRM(rx_base, delay->rxcmd2trntrm);
	ADI_C2CC_WRITE_RXCMD2LD(rx_base, delay->rxcmd2ld);
	ADI_C2CC_WRITE_LDDUR(rx_base, delay->lddur);
	ADI_C2CC_WRITE_MTCHDUR(rx_base, delay->mtchdur);
	ADI_C2CC_WRITE_SWBCKTRM(rx_base, delay->swbcktrm);
	ADI_C2CC_WRITE_BLKTRF(rx_base, delay->blktrf);
}

bool adi_c2cc_setup_train(struct adi_c2cc_training_settings *params)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;

	if (pri_base == 0 || sec_base == 0) {
		ERROR("%s: C2CC must be initialized first.\n", __func__);
		return false;
	}

	if (!ADI_C2CC_READ_C2C_EN(pri_base)) {
		ERROR("%s: C2CC must be enabled at normal speed first.\n", __func__);
		return false;
	}

	if (params == NULL) {
		ERROR("%s: C2CC invalid training parameters.\n", __func__);
		return false;
	}

	adi_c2cc_configure_tx_clk(pri_base, sec_base, &params->tx_clk);                 /* clock divisors */
	adi_c2cc_configure_generator(pri_base, sec_base, &params->generator);           /* polynomial and seed */
	adi_c2cc_configure_delay(pri_base, sec_base, &params->p2s_delay);               /* primary-to-secondary */
	if (!adi_c2cc_loopback)
		adi_c2cc_configure_delay(sec_base, pri_base, &params->s2p_delay);       /* secondary-to-primary */

	return true;
}

bool adi_c2cc_run_train(uint32_t *p2s_stats, uint32_t *s2p_stats, uint8_t *p2s_trim_delays, uint8_t *s2p_trim_delays)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;
	uint8_t trim = 0;
	unsigned int i = 0;

	if ((p2s_trim_delays == NULL) || (s2p_trim_delays == NULL) || (p2s_stats == NULL) || (s2p_stats == NULL)) {
		ERROR("%s: C2CC invalid statistics parameters.\n", __func__);
		return false;
	}

	for (i = 0; i < ADI_C2C_LANE_COUNT; i++) {
		ADI_C2CC_WRITE_TXDATA_DELAY(pri_base, i, p2s_trim_delays[i]);
		ADI_C2CC_WRITE_RXDATA_DELAY(pri_base, i, s2p_trim_delays[i]);
	}

	/* disable bridged interrupts */
	ADI_C2CC_WRITE_INT_EN(sec_base, 0);
	ADI_C2CC_WRITE_INT_EN(pri_base, 0);

	VERBOSE("%s: C2CC gathering primary (Tx) to secondary (Rx) statistics.\n", __func__);
	for (trim = 0; trim < ADI_C2C_TRIM_MAX; trim++) {
		if (!adi_c2cc_p2s_gather_statistics(pri_base, sec_base, trim, &p2s_stats[trim])) {
			ERROR("%s: C2CC timeout while gathering statistics (p2s,trim=%u).\n", __func__, trim);
			ADI_C2CC_WRITE_INT_EN(pri_base, 1);
			ADI_C2CC_WRITE_INT_EN(sec_base, 1);
			return false;
		}
	}

	if (!adi_c2cc_loopback) {
		VERBOSE("%s: C2CC gathering secondary (Tx) to primary (Rx) statistics.\n", __func__);
		for (trim = 0; trim < ADI_C2C_TRIM_MAX; trim++)
			if (!adi_c2cc_s2p_gather_statistics(sec_base, pri_base, trim, &s2p_stats[trim])) {
				ERROR("%s: C2CC timeout while gathering statistics (s2p,trim=%u).\n", __func__, trim);
				ADI_C2CC_WRITE_INT_EN(pri_base, 1);
				ADI_C2CC_WRITE_INT_EN(sec_base, 1);
				return false;
			}
	}

	/* re-enable bridged interrupts */
	ADI_C2CC_WRITE_INT_EN(pri_base, 1);
	ADI_C2CC_WRITE_INT_EN(sec_base, 1);

	return true;
}

static bool adi_c2cc_is_all_zero(uint8_t *arr, size_t size)
{
	size_t i;

	if (arr == NULL || size == 0)
		return false;
	for (i = 0; i < size; i++)
		if (arr[i])
			return false;
	return true;
}

bool adi_c2cc_analyze_train_data(uint32_t *p2s_stats, uint32_t *s2p_stats, size_t min_size, size_t max_spread, size_t max_center, uint8_t *p2s_trim_delays, uint8_t *s2p_trim_delays, uint8_t *p2s_trim, uint8_t *s2p_trim)
{
	size_t eye_width = 0;
	uint8_t trim = 0;
	uint8_t trim_delays[ADI_C2C_LANE_COUNT] = { 0 };

	if ((p2s_stats == NULL) || (s2p_stats == NULL)) {
		ERROR("%s: C2CC invalid statistics parameters.\n", __func__);
		return false;
	}

	trim = adi_c2cc_find_optimal_trim(p2s_stats, min_size, max_spread, max_center, trim_delays, &eye_width);
	if (trim == ADI_C2C_TRIM_MAX && (p2s_trim_delays == NULL || adi_c2cc_is_all_zero(trim_delays, sizeof(trim_delays) / sizeof(trim_delays[0])))) {
		WARN("%s: C2CC failed to find optimal p2s trim.\n", __func__);
		return false;
	}

	INFO(
		"%s: C2CC selected primary-to-secondary trim %u eye_width %lu (delays=%u,%u,%u,%u)\n",
		__func__, trim, eye_width, trim_delays[0], trim_delays[1], trim_delays[2], trim_delays[3]
		);

	if (p2s_trim)
		*p2s_trim = trim;
	if (p2s_trim_delays)
		memcpy(p2s_trim_delays, trim_delays, sizeof(trim_delays));

	/* Loopback mode does not require s2p config */
	if (adi_c2cc_loopback)
		return true;

	trim = adi_c2cc_find_optimal_trim(s2p_stats, min_size, max_spread, max_center, trim_delays, &eye_width);
	if (trim == ADI_C2C_TRIM_MAX && (s2p_trim_delays == NULL || adi_c2cc_is_all_zero(trim_delays, sizeof(trim_delays) / sizeof(trim_delays[0])))) {
		WARN("%s: C2CC failed to find optimal s2p trim.\n", __func__);
		return false;
	}

	INFO(
		"%s: C2CC selected secondary-to-primary trim %u eye_width %lu (delays=%u,%u,%u,%u)\n",
		__func__, trim, eye_width, trim_delays[0], trim_delays[1], trim_delays[2], trim_delays[3]
		);

	if (s2p_trim)
		*s2p_trim = trim;
	if (s2p_trim_delays)
		memcpy(s2p_trim_delays, trim_delays, sizeof(trim_delays));

	return true;
}

bool adi_c2cc_apply_training(uint8_t p2s_trim, uint8_t s2p_trim, struct adi_c2cc_training_clock_settings *tx_clk)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;

	if (!adi_c2cc_wait_transactions(pri_base) || !adi_c2cc_wait_transactions(sec_base)) {
		ERROR("%s: C2CC timeout while applying training.\n", __func__);
		return false;
	}

	ADI_C2CC_WRITE_RXCLK_DELAY(sec_base, p2s_trim);

	if (!adi_c2cc_wait_transactions(pri_base))
		return false;

	udelay(10U);
	ADI_C2CC_WRITE_ROSC_TX_CLK_DIV(pri_base, tx_clk->rosc_div);
	ADI_C2CC_WRITE_DEV_TX_CLK_DIV(pri_base, tx_clk->devclk_div);
	ADI_C2CC_WRITE_PLL_TX_CLK_DIV(pri_base, tx_clk->pll_div);

	if (!adi_c2cc_wait_transactions(pri_base) || !adi_c2cc_wait_transactions(sec_base)) {
		ERROR("%s: C2CC timeout while applying training.\n", __func__);
		return false;
	}

	if (adi_c2cc_loopback)
		/* No need to apply s2p related settings in loopback mode */
		return true;

	ADI_C2CC_WRITE_RXCLK_DELAY(pri_base, s2p_trim);
	udelay(10U);
	ADI_C2CC_WRITE_ROSC_TX_CLK_DIV(sec_base, tx_clk->rosc_div);
	ADI_C2CC_WRITE_DEV_TX_CLK_DIV(sec_base, tx_clk->devclk_div);
	ADI_C2CC_WRITE_PLL_TX_CLK_DIV(sec_base, tx_clk->pll_div);

	return true;
}

bool adi_c2cc_run_loopback_test()
{
	unsigned int lane;
	unsigned int edge;
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	bool ret = true;
	uint16_t cnt;

	/* TODO: pending HW design confirmation. By now, keeping power up
	 *       calibration configuration values
	 * - txptrndur
	 * - lpdur
	 */

	/* Start LFSR */
	ADI_C2CC_WRITE_TSTCONFIG_START_LFSR(pri_base, true);

	udelay(100);

	/* Stop LFSR */
	ADI_C2CC_WRITE_TSTCONFIG_STOP_LFSR(pri_base, true);

	/* Check errors */
	for (lane = 0; lane < ADI_C2C_LANE_COUNT; lane++) {
		for (edge = 0; edge < 2; edge++) {
			cnt = ADI_C2CC_READ_TRAINSTAT1_FAIL_CNT(pri_base, lane, edge);
			if (cnt != 0) {
				ERROR("%s: FAIL_CNT: lane %d, edge %d: %d\n", __func__, lane, edge, cnt);
				ret = false;
			}
			cnt = ADI_C2CC_READ_TRAINSTAT2_BAD_CNT(pri_base, lane, edge);
			if (cnt != 0) {
				ERROR("%s: BAD_CNT: lane %d, edge %d: %d\n", __func__, lane, edge, cnt);
				ret = false;
			}
			cnt = ADI_C2CC_READ_TRAINSTAT1_LATE_CNT(pri_base, lane, edge);
			if (cnt != 0) {
				ERROR("%s: LATE_CNT: lane %d, edge %d: %d\n", __func__, lane, edge, cnt);
				ret = false;
			}
			cnt = ADI_C2CC_READ_TRAINSTAT2_EARLY_CNT(pri_base, lane, edge);
			if (cnt != 0) {
				ERROR("%s: EARLY_CNT: lane %d, edge %d: %d\n", __func__, lane, edge, cnt);
				ret = false;
			}
		}
	}

	/* Clean everything */
	ADI_C2CC_WRITE_TSTCONFIG_LFSR_STS_CLR(pri_base, true);
	ADI_C2CC_WRITE_TSTCONFIG_START_LFSR(pri_base, false);
	ADI_C2CC_WRITE_TSTCONFIG_STOP_LFSR(pri_base, false);
	ADI_C2CC_WRITE_TSTCONFIG_LFSR_STS_CLR(pri_base, false);

	return ret;
}
