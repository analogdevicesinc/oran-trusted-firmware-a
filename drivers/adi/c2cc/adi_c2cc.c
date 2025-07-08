/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/adi/adi_c2cc.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include "adi_c2cc.h"

/* Low speed (20 MHz) settings */
#define ADI_C2CC_TXDIV_ROSC 25
#define ADI_C2CC_TXDIV_DEVCLK 50
#define ADI_C2CC_TXDIV_PLL 50

/* Background calibration */
#define ADI_C2C_BG_CAL_MODE_HW 1
#define ADI_C2C_MIN_BG_CAL_PERIOD_CYCLES 32
#define ADI_C2C_MAX_BG_CAL_PERIOD_CYCLES 0xFFFFFFFF
#define ADI_C2C_MAX_BG_CAL_MS_DELAY     3
#define ADI_C2C_NO_PRBS_INJECTION       0
#define ADI_C2C_PRBS_INJECTION          1
#define ADI_C2C_FIXED_MMR_INJECTION     2

static uintptr_t adi_c2cc_primary_addr_base = 0;
static uintptr_t adi_c2cc_secondary_addr_base = 0;
static bool adi_c2cc_loopback = false;

static uint32_t adi_c2cc_read_bf32(uintptr_t addr, uint8_t position, uint32_t mask)
{
	return (mmio_read_32(addr) & mask) >> position;
}

static void adi_c2cc_write_bf32(uintptr_t addr, uint8_t position, uint32_t mask, uint32_t val)
{
	uint32_t reg = (mmio_read_32(addr) & ~mask);

	mmio_write_32(addr, reg | ((val << position) & mask));
}

static bool adi_c2cc_wait_transactions(uintptr_t addr_base)
{
	uint64_t timeout = timeout_init_us(ADI_C2C_MAX_AXI_WAIT);

	while (ADI_C2CC_IS_AXI_OUTSTANDING(addr_base))
		if (timeout_elapsed(timeout))
			return false;
	return true;
}

static void adi_c2cc_intr_control(uintptr_t addr_base, bool enabled)
{
	uint32_t reg = 0x0;

	/* Enable/disable all individual interrupt ports */
	/* TODO: Determine if these should be enabled on a per-interrupt
	 * basis or if it is ok to do just turn everything on, as it is
	 * done here.
	 */
	for (reg = (addr_base + ADI_C2CC_REG_INTPORTEN0); reg <= (addr_base + ADI_C2CC_REG_INTPORTEN9); reg += sizeof(uint32_t))
		mmio_write_32(reg, enabled ? 0xFFFFFFFF : 0x0);

	/* Then configure the global interrupt enable */
	ADI_C2CC_WRITE_INT_EN(addr_base, enabled ? 1 : 0);
	dsb();
}

static void adi_c2cc_axi_control(uintptr_t addr_base, bool enabled)
{
	ADI_C2CC_WRITE_AXI_INIT_EN(addr_base, enabled ? 1 : 0);
	dsb();
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

static size_t adi_c2cc_find_largest_combined_window(uint32_t *stats, size_t *offset)
{
	size_t s = 0;
	size_t o = 0;
	int pos = -1;
	size_t i = 0;

	/* find largest contiguous window of trim values with no errors */
	for (i = 0; i < ADI_C2C_TRIM_MAX; i++) {
		switch (pos) {
		case -1:
			if (!stats[i])
				pos = i;
			break;
		default:
			if (stats[i]) {
				size_t size = i - pos;
				VERBOSE("%s: C2CC found candidate (offset=%d,size=%lu)\n", __func__, pos, size);
				if (size > s) {
					o = pos;
					s = size;
				}
				pos = -1;
			}
			break;
		}
	}
	if (!stats[ADI_C2C_TRIM_MAX - 1] && pos >= 0) {
		size_t size = ADI_C2C_TRIM_MAX - pos;
		VERBOSE("%s: C2CC found candidate (offset=%d,size=%lu)\n", __func__, pos, size);
		if (size > s) {
			o = pos;
			s = size;
		}
	}

	if (offset)
		*offset = o;
	return s;
}

static size_t adi_c2cc_find_lane_window_end(uint32_t *stats, size_t start, unsigned int lane)
{
	size_t i = 0;

	for (i = start; i < ADI_C2C_TRIM_MAX && ADI_C2C_GET_LANE_STAT(stats, i, lane) == 0; i++) {
	}
	return i;
}

static size_t adi_c2cc_find_next_window(uint32_t *stats, size_t start, size_t max_dist, size_t min_size, unsigned int *lane, size_t *offset)
{
	bool skip_lanes[ADI_C2C_LANE_COUNT] = { 0 };
	size_t end = 0;
	size_t i = 0;
	size_t j = 0;

	if (start != 0) {
		uint8_t lane_stats[ADI_C2C_LANE_COUNT] = ADI_C2C_GET_LANE_STATS(stats, start - 1);
		/* skip over any windows that started before 'start' */
		for (j = 0; j < ADI_C2C_LANE_COUNT; j++)
			if (!lane_stats[j])
				skip_lanes[j] = true;
	}

	for (i = start; i < start + max_dist; i++) {
		uint8_t lane_stats[ADI_C2C_LANE_COUNT] = ADI_C2C_GET_LANE_STATS(stats, i);
		for (j = 0; j < ADI_C2C_LANE_COUNT; j++) {
			if (skip_lanes[j]) {
				/* stop skipping if the window closed */
				if (lane_stats[j])
					skip_lanes[j] = false;
				continue;
			}
			if (lane_stats[j])
				continue;
			end = adi_c2cc_find_lane_window_end(stats, i, j);
			VERBOSE("%s: C2CC found candidate (offset=%lu,size=%lu)\n", __func__, i, end - i);
			if (end - i < min_size) {
				skip_lanes[j] = true;
				continue;
			}
			if (offset)
				*offset = i;
			if (lane)
				*lane = j;
			return end - i;
		}
	}

	return 0;
}

static size_t adi_c2cc_find_next_lane_window(uint32_t *stats, size_t start, unsigned int lane, size_t max_dist, size_t min_size, size_t *offset)
{
	size_t end = 0;
	size_t i = start;

	if (start != 0) {
		uint8_t stat = ADI_C2C_GET_LANE_STAT(stats, start - 1, lane);
		if (!stat) {
			end = adi_c2cc_find_lane_window_end(stats, start, lane);
			if (end - start > max_dist)
				return 0;
			max_dist -= (end - start);
			start = end;
			i = end;
		}
	}

	while (i < start + max_dist) {
		uint8_t stat = ADI_C2C_GET_LANE_STAT(stats, i, lane);
		if (stat) {
			i += 1;
			continue;
		}
		end = adi_c2cc_find_lane_window_end(stats, i, lane);
		VERBOSE("%s: C2CC found candidate (offset=%lu,size=%lu)\n", __func__, i, end - i);
		if (end - i < min_size) {
			i = end;
			continue;
		}
		if (offset)
			*offset = i;
		return end - i;
	}

	return 0;
}

static bool adi_c2cc_find_window_group(uint32_t *stats, size_t max_spread, size_t min_size, size_t *offsets, size_t *sizes, size_t *largest_offset, size_t *smallest_size)
{
	size_t start = 0;
	size_t o = 0;
	size_t s = 0;
	unsigned int l = 0;
	unsigned int i = 0;

	while (start < ADI_C2C_TRIM_DELAY_MAX) {
		bool valid_candidate = true;

		if (!offsets || !sizes)
			return false;
		s = adi_c2cc_find_next_window(stats, start, ADI_C2C_TRIM_MAX - start, min_size, &l, &o);
		if (!s)
			return false;
		VERBOSE("%s: C2CC found candidate (offset=%lu,size=%lu,lane=%u)\n", __func__, o, s, l);
		start = o;
		offsets[l] = o;
		sizes[l] = s;
		if (largest_offset)
			*largest_offset = o;
		if (smallest_size)
			*smallest_size = s;

		for (i = 0; i < ADI_C2C_LANE_COUNT; i++) {
			if (i == l)
				continue;
			s = adi_c2cc_find_next_lane_window(stats, start, i, max_spread, min_size, &o);
			if (!s) {
				valid_candidate = false;
				break;
			}
			offsets[i] = o;
			sizes[i] = s;
			if (largest_offset && o > *largest_offset)
				*largest_offset = o;
			if (smallest_size && s < *smallest_size)
				*smallest_size = s;
		}

		if (!valid_candidate) {
			start += 1;
			continue;
		}

		return true;
	}

	return false;
}

/**
 * attempt to find the optimal trim value based on reviewing the training
 * statistics. ideally, this is done by finding the largest contiguous window
 * of trim values that reported no error, and selecting the middle of that
 * range.
 * however, in some cases, a certain lane may be consistently too slow or fast
 * which causes that lane's "no-error" window to sit at slightly higher or
 * lower trim values than the other lanes. if the windows for all four lanes
 * do not overlap enough, then we need to apply a delay to some lanes in order
 * to bring everything back into alignment.
 * @param[in] stats - the array of training statistics.
 * @param[in] min_size - minimum valid window size
 * @param[out] trim_delays - the chosen trim delay values (this is an array of
 *                           size ADI_C2C_LANE_COUNT).
 * @param[out] eye_width - the window size for the chosen trim value
 * @return the chosen trim value, or ADI_C2C_TRIM_MAX if no window was found.
 */
static uint8_t adi_c2cc_find_optimal_trim(uint32_t *stats, size_t min_size, uint8_t *trim_delays, size_t *eye_width)
{
	size_t target_offset = 0;
	size_t target_size = 0;
	size_t offsets[ADI_C2C_LANE_COUNT] = { 0 };
	size_t sizes[ADI_C2C_LANE_COUNT] = { 0 };
	size_t i = 0;

	target_size = adi_c2cc_find_largest_combined_window(stats, &target_offset);
	if (target_size >= min_size) {
		if (trim_delays)
			for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
				trim_delays[i] = 0;
		if (eye_width)
			*eye_width = target_size;
		return target_offset + (target_size / 2);
	}
	WARN("%s: C2CC failed to find suitable trim window. Re-attempting with delay to account for skew.\n", __func__);

	if (!adi_c2cc_find_window_group(stats, ADI_C2C_TRIM_DELAY_MAX, min_size, offsets, sizes, &target_offset, &target_size)) {
		WARN("%s: C2CC failed to find suitable trim window (with delay).\n", __func__);
		return ADI_C2C_TRIM_MAX;
	}

	if (trim_delays)
		for (i = 0; i < ADI_C2C_LANE_COUNT; i++)
			trim_delays[i] = target_offset - offsets[i];
	if (eye_width)
		*eye_width = target_size;

	return target_offset + (target_size / 2);
}

static bool adi_c2cc_apply_training(
	uintptr_t pri_base, uintptr_t sec_base,
	uint8_t p2s_trim, uint8_t *p2s_trim_delays,
	uint8_t s2p_trim, uint8_t *s2p_trim_delays,
	struct adi_c2cc_training_clock_settings *tx_clk
	)
{
	unsigned int i = 0;

	for (i = 0; i < ADI_C2C_LANE_COUNT; i++) {
		ADI_C2CC_WRITE_TXDATA_DELAY(pri_base, i, p2s_trim_delays[i]);
		ADI_C2CC_WRITE_RXDATA_DELAY(pri_base, i, s2p_trim_delays[i]);
	}

	if (!adi_c2cc_wait_transactions(pri_base) || !adi_c2cc_wait_transactions(sec_base))
		return false;

	ADI_C2CC_WRITE_RXCLK_DELAY(sec_base, p2s_trim);

	if (!adi_c2cc_wait_transactions(pri_base))
		return false;

	udelay(10U);
	ADI_C2CC_WRITE_ROSC_TX_CLK_DIV(pri_base, tx_clk->rosc_div);
	ADI_C2CC_WRITE_DEV_TX_CLK_DIV(pri_base, tx_clk->devclk_div);
	ADI_C2CC_WRITE_PLL_TX_CLK_DIV(pri_base, tx_clk->pll_div);

	if (!adi_c2cc_wait_transactions(pri_base) || !adi_c2cc_wait_transactions(sec_base))
		return false;

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

static void adi_c2cc_phydig_enable(bool enable)
{
	ADI_C2CC_WRITE_PHYDIG_LPBK_EN(adi_c2cc_primary_addr_base, enable);
}

bool adi_c2cc_enable_high_speed(struct adi_c2cc_training_settings *params)
{
	uint32_t p2s_stats[ADI_C2C_TRIM_MAX];
	uint32_t s2p_stats[ADI_C2C_TRIM_MAX];

	if (!adi_c2cc_setup_train(params))
		return false;

	if (!adi_c2cc_run_train(p2s_stats, s2p_stats))
		return false;

	if (!adi_c2cc_process_train_data_and_apply(ADI_C2C_MIN_WINDOW_SIZE, NULL, p2s_stats, s2p_stats, &(params->tx_clk)))
		return false;

	return true;
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

bool adi_c2cc_run_train(uint32_t *p2s_stats, uint32_t *s2p_stats)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;
	uint8_t trim = 0;

	if ((p2s_stats == NULL) || (s2p_stats == NULL)) {
		ERROR("%s: C2CC invalid statistics parameters.\n", __func__);
		return false;
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

bool adi_c2cc_process_train_data_and_apply(
	size_t min_size, struct adi_c2cc_trim_settings *forced_trim,
	uint32_t *p2s_stats, uint32_t *s2p_stats,
	struct adi_c2cc_training_clock_settings *tx_clk)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;
	uint8_t p2s_trim = ADI_C2C_TRIM_MAX;
	uint8_t p2s_trim_delays[ADI_C2C_LANE_COUNT] = { 0 };
	uint8_t s2p_trim = ADI_C2C_TRIM_MAX;
	uint8_t s2p_trim_delays[ADI_C2C_LANE_COUNT] = { 0 };
	size_t eye_width;
	int lane;

	/* re-enable bridged interrupts */
	ADI_C2CC_WRITE_INT_EN(pri_base, 1);
	ADI_C2CC_WRITE_INT_EN(sec_base, 1);

	if (forced_trim) {
		p2s_trim = forced_trim->p2s_trim;
		s2p_trim = forced_trim->s2p_trim;
		for (lane = 0; lane < ADI_C2C_LANE_COUNT; lane++) {
			p2s_trim_delays[lane] = forced_trim->p2s_trim_delays[lane];
			s2p_trim_delays[lane] = forced_trim->s2p_trim_delays[lane];
		}

		INFO("%s: C2CC trim settings forced by user\n", __func__);
	} else {
		p2s_trim = adi_c2cc_find_optimal_trim(p2s_stats, min_size, p2s_trim_delays, &eye_width);
		if (p2s_trim == ADI_C2C_TRIM_MAX) {
			WARN("%s: C2CC failed to find optimal p2s trim.\n", __func__);
			return false;
		}
		INFO(
			"%s: C2CC selected primary-to-secondary trim %u eye_width %lu (delays=%u,%u,%u,%u)\n",
			__func__, p2s_trim, eye_width, p2s_trim_delays[0], p2s_trim_delays[1], p2s_trim_delays[2], p2s_trim_delays[3]
			);

		/* Loopback mode does not require s2p config */
		if (!adi_c2cc_loopback) {
			s2p_trim = adi_c2cc_find_optimal_trim(s2p_stats, min_size, s2p_trim_delays, &eye_width);
			if (s2p_trim == ADI_C2C_TRIM_MAX) {
				WARN("%s: C2CC failed to find optimal s2p trim.\n", __func__);
				return false;
			}
			INFO(
				"%s: C2CC selected secondary-to-primary trim %u eye_width %lu (delays=%u,%u,%u,%u)\n",
				__func__, s2p_trim, eye_width, s2p_trim_delays[0], s2p_trim_delays[1], s2p_trim_delays[2], s2p_trim_delays[3]
				);
		}
	}

	if (!adi_c2cc_apply_training(pri_base, sec_base, p2s_trim, p2s_trim_delays, s2p_trim, s2p_trim_delays, tx_clk)) {
		ERROR("%s: C2CC timeout while applying training.\n", __func__);
		return false;
	}

	INFO("%s: C2CC phy training complete.\n", __func__);

	return true;
}

bool adi_c2cc_enable(void)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;

	if (pri_base == 0 || sec_base == 0) {
		ERROR("%s: C2CC must be initialized first.\n", __func__);
		return false;
	}

	ADI_C2CC_WRITE_C2C_EN(pri_base, 1); /* secondary is already enabled */
	/* enable bi-directional bridged AXI transactions */
	adi_c2cc_axi_control(pri_base, true);
	adi_c2cc_axi_control(sec_base, true);
	/* enable bi-directional bridged interrupts */
	adi_c2cc_intr_control(pri_base, true);
	adi_c2cc_intr_control(sec_base, true);

	return true;
}

void adi_c2cc_init(uintptr_t pri_base, uintptr_t sec_base, c2c_mode_t mode)
{
	adi_c2cc_loopback = (mode != C2C_MODE_NORMAL);
	adi_c2cc_primary_addr_base = pri_base;
	adi_c2cc_secondary_addr_base = adi_c2cc_loopback ? pri_base : sec_base;

	adi_c2cc_phydig_enable(mode == C2C_MODE_PHYDIG_LOOPBACK);
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

bool adi_c2cc_enable_hw_bg_cal(struct adi_c2cc_calibration_settings *params, struct adi_c2cc_training_generator_settings *generator_params)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;

	uint8_t pattern_mode = generator_params != NULL ? ADI_C2C_PRBS_INJECTION : ADI_C2C_NO_PRBS_INJECTION;

	if (pri_base == 0 || sec_base == 0) {
		ERROR("%s: C2CC must be initialized first.\n", __func__);
		return false;
	}

	if (params->period < ADI_C2C_MIN_BG_CAL_PERIOD_CYCLES ||
	    params->period > ADI_C2C_MAX_BG_CAL_PERIOD_CYCLES) {
		ERROR("%s: C2CC background calibration period out of bounds.\n", __func__);
		return false;
	}

	if (params->transition_delay >= params->period) {
		ERROR("%s: C2CC background calibration transition delay must be lower than the background calibration period.\n", __func__);
		return false;
	}

	if (params->multisample_delay > ADI_C2C_MAX_BG_CAL_MS_DELAY) {
		ERROR("%s: C2CC background calibration multisample delay out of bounds.\n", __func__);
		return false;
	}

	// Disable BG calibration
	ADI_C2CC_WRITE_BG_CAL_EN(pri_base, 0);
	ADI_C2CC_WRITE_BG_CAL_EN(sec_base, 0);

	// 1. Configure PRIMARY.CALIBCTRL1.background_cal_mode, SECONDARY.CALIBCTRL1.background_cal_mode to decide Hardware vs. Software Mode (i.e. 1: Hardware mode ; 0: software mode).
	ADI_C2CC_WRITE_BG_CAL_MODE(pri_base, ADI_C2C_BG_CAL_MODE_HW);
	ADI_C2CC_WRITE_BG_CAL_MODE(sec_base, ADI_C2C_BG_CAL_MODE_HW);

	// 3. Configure Primary.CALIBCTRL3.background_cal_prd, Secondary.CALIBCTRL3.background_cal_prd for Background calibration period
	ADI_C2CC_WRITE_BG_CAL_PERIOD(pri_base, params->period);
	ADI_C2CC_WRITE_BG_CAL_PERIOD(sec_base, params->period);

	// 4. Configure CALIBCTRL1.background_cal_tran to define the time taken for threshold value to be updated in the PHY
	ADI_C2CC_WRITE_BG_CAL_TRANSITION(pri_base, params->transition_delay);
	ADI_C2CC_WRITE_BG_CAL_TRANSITION(sec_base, params->transition_delay);

	// 5. Enable background calibration pattern injection if required
	// a) PRBS/Fixed pattern
	ADI_C2CC_WRITE_BG_CAL_PATTERN_MODE(pri_base, pattern_mode);
	ADI_C2CC_WRITE_BG_CAL_PATTERN_MODE(sec_base, pattern_mode);
	// b)
	ADI_C2CC_WRITE_BG_CAL_PATTERN_PERIOD(pri_base, params->pattern_period);
	ADI_C2CC_WRITE_BG_CAL_PATTERN_PERIOD(sec_base, params->pattern_period);
	// c)
	ADI_C2CC_WRITE_BG_CAL_PATTERN_SIZE(pri_base, params->pattern_size);
	ADI_C2CC_WRITE_BG_CAL_PATTERN_SIZE(sec_base, params->pattern_size);

	// 6. Configure Primary.CALIBCTRL4.bg_lfsr_poly , Primary.CALIBCTRL4.bg_lfsr_seed, Secondary.CALIBCTRL4.bg_lfsr_poly , Secondary.CALIBCTRL4.bg_lfsr_seed  with the polynomial and seed value for PRBS generator
	if (pattern_mode == ADI_C2C_PRBS_INJECTION) {
		ADI_C2CC_WRITE_BG_CAL_LFSR_POLY(pri_base, generator_params->pos_poly);
		ADI_C2CC_WRITE_BG_CAL_LFSR_POLY(sec_base, generator_params->pos_poly);
		ADI_C2CC_WRITE_BG_CAL_LFSR_SEED(pri_base, generator_params->seed);
		ADI_C2CC_WRITE_BG_CAL_LFSR_SEED(sec_base, generator_params->seed);
	}

	// 7. The multi-sample delay line structure is programmable: max 3 stages of delta.
	// a)
	ADI_C2CC_WRITE_RXCLK_DELAY_TRAIN(pri_base, 0);
	ADI_C2CC_WRITE_RXCLK_DELAY_TRAIN(sec_base, 0);
	// b)
	ADI_C2CC_WRITE_RXCLK_MS_DELAY(pri_base, params->multisample_delay);
	ADI_C2CC_WRITE_RXCLK_MS_DELAY(sec_base, params->multisample_delay);

	// 8. Finally enable the background calibration
	ADI_C2CC_WRITE_BG_CAL_EN(pri_base, 1);
	ADI_C2CC_WRITE_BG_CAL_EN(sec_base, 1);

	return true;
}
