/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/adi/adi_c2cc.h>
#include "adi_c2cc_analysis.h"
#include "adi_c2cc_training.h"
#include "adi_c2cc_util.h"

/* Background calibration */
#define ADI_C2C_BG_CAL_MODE_HW 1
#define ADI_C2C_MIN_BG_CAL_PERIOD_CYCLES 32
#define ADI_C2C_MAX_BG_CAL_PERIOD_CYCLES 0xFFFFFFFF
#define ADI_C2C_MAX_BG_CAL_MS_DELAY     3
#define ADI_C2C_NO_PRBS_INJECTION       0
#define ADI_C2C_PRBS_INJECTION          1
#define ADI_C2C_FIXED_MMR_INJECTION     2

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
