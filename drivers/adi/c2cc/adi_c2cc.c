/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/adi/adi_c2cc.h>
#include "adi_c2cc_analysis.h"
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
	uint8_t p2s_trim_delays[ADI_C2C_LANE_COUNT] = { 0 };
	uint8_t s2p_trim_delays[ADI_C2C_LANE_COUNT] = { 0 };
	uint8_t p2s_trim = ADI_C2C_TRIM_MAX;
	uint8_t s2p_trim = ADI_C2C_TRIM_MAX;

	if (!adi_c2cc_setup_train(params))
		return false;

	if (!adi_c2cc_run_train(p2s_stats, s2p_stats, p2s_trim_delays, s2p_trim_delays))
		return false;

	if (!adi_c2cc_analyze_train_data(p2s_stats, s2p_stats, ADI_C2C_MIN_WINDOW_SIZE, ADI_C2C_TRIM_DELAY_MAX, ADI_C2C_MAX_TRIM_CENTER, p2s_trim_delays, s2p_trim_delays, NULL, NULL))
		return false;

	if (!adi_c2cc_run_train(p2s_stats, s2p_stats, p2s_trim_delays, s2p_trim_delays))
		return false;

	if (!adi_c2cc_analyze_train_data(p2s_stats, s2p_stats, ADI_C2C_MIN_WINDOW_SIZE, ADI_C2C_TRIM_DELAY_MAX, ADI_C2C_MAX_TRIM_CENTER, NULL, NULL, &p2s_trim, &s2p_trim))
		return false;

	if (!adi_c2cc_apply_training(p2s_trim, s2p_trim, &params->tx_clk))
		return false;

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

const char *adi_c2cc_get_err_name(c2c_err_type_t type)
{
	switch (type) {
	case C2C_ERR_ECC_1B:
		return "ERR_ECC_1B";
	case C2C_ERR_ECC_1B_COUNT_THRESHOLD:
		return "ERR_ECC_1B_COUNT_THRESHOLD";
	case C2C_ERR_START_BIT_1_BIT:
		return "ERR_START_BIT_1_BIT";
	case C2C_ERR_START_BIT_1B_COUNT_THRESHOLD:
		return "ERR_START_BIT_1B_COUNT_THRESHOLD";
	case C2C_ERR_ECC_2B:
		return "ERR_ECC_2B";
	case C2C_ERR_START_BIT_2_BIT:
		return "ERR_START_BIT_2_BIT";
	case C2C_ERR_INT_TX_OL:
		return "ERR_INT_TX_OL";
	case C2C_ERR_INT_RX_OL:
		return "ERR_INT_RX_OL";
	case C2C_ERR_INVALID_HEADER:
		return "ERR_INVALID_HEADER";
	case C2C_ERR_TX_INTERRUPT_OL_COUNT_SATURATED:
		return "ERR_TX_INTERRUPT_OL_COUNT_SATURATED";
	case C2C_ERR_RX_INTERRUPT_OL_COUNT_SATURATED:
		return "ERR_RX_INTERRUPT_OL_COUNT_SATURATED";
	case C2C_ERR_BACKGROUND_CAL_SW_ERR:
		return "ERR_BACKGROUND_CAL_SW_ERR";
	case C2C_ERR_BACKGROUND_CAL_SW_WARN:
		return "ERR_BACKGROUND_CAL_SW_WARN";
	case C2C_ERR_BACKGROUND_CAL_HW_ERR:
		return "ERR_BACKGROUND_CAL_HW_ERR";
	case C2C_ERR_BACKGROUND_CAL_HW_UPD:
		return "ERR_BACKGROUND_CAL_HW_UPD";
	case C2C_ERR_PWR_UP_CAL_TX_IRQ:
		return "ERR_PWR_UP_CAL_TX_IRQ";
	case C2C_ERR_PWR_UP_CAL_RX_IRQ:
		return "ERR_PWR_UP_CAL_RX_IRQ";
	case C2C_ERR_TXN_IN_C2C_DISABLED:
		return "ERR_TXN_IN_C2C_DISABLED";
	default:
		break;
	}
	return "ERR_UNKNOWN";
}

const char *adi_c2cc_get_err_description(c2c_err_type_t type)
{
	switch (type) {
	case C2C_ERR_ECC_1B:
		return "One or more ECC 1-bit error has been found in inflow stream, since the last status reset.";
	case C2C_ERR_ECC_1B_COUNT_THRESHOLD:
		return "The counter for ECC 1-bit error has crossed the threshold value.";
	case C2C_ERR_START_BIT_1_BIT:
		return "One or more 1-bit error has been found in start-nibble since the last status reset.";
	case C2C_ERR_START_BIT_1B_COUNT_THRESHOLD:
		return "The counter for 1-bit error in start bit has crossed the threshold value.";
	case C2C_ERR_ECC_2B:
		return "One or more ECC 2-bit error has been found in inflow stream, since the last status reset.";
	case C2C_ERR_START_BIT_2_BIT:
		return "One or more 2-bit error has been found in start-nibble since the last status reset.";
	case C2C_ERR_INT_TX_OL:
		return "One or more interrupt line in Tx has been found to overload. Some events in the line has been dropped, and will not be regenerated at destination chip output.";
	case C2C_ERR_INT_RX_OL:
		return "One or more interrupt line in Rx has been found to overload. Some events in the line has been dropped, and will not be regenerated at destination chip output.";
	case C2C_ERR_INVALID_HEADER:
		return "Invalid header has been found at depacketizer, this could be due to framing error or ECC 3- bit error.";
	case C2C_ERR_TX_INTERRUPT_OL_COUNT_SATURATED:
		return "The counter for tx_interrupt overload has been saturated.";
	case C2C_ERR_RX_INTERRUPT_OL_COUNT_SATURATED:
		return "The counter for rx_interrupt overload has been saturated.";
	case C2C_ERR_BACKGROUND_CAL_SW_ERR:
		return "Background cal software error.";
	case C2C_ERR_BACKGROUND_CAL_SW_WARN:
		return "Background cal software warn.";
	case C2C_ERR_BACKGROUND_CAL_HW_ERR:
		return "Background cal hardware error.";
	case C2C_ERR_BACKGROUND_CAL_HW_UPD:
		return "Background cal hardware update.";
	case C2C_ERR_PWR_UP_CAL_TX_IRQ:
		return "Trim done IRQ.";
	case C2C_ERR_PWR_UP_CAL_RX_IRQ:
		return "Trim done IRQ.";
	case C2C_ERR_TXN_IN_C2C_DISABLED:
		return "Transaction issued while c2c was disabled.";
	default:
		break;
	}
	return "Unknown error.";
}

bool adi_c2cc_error_handler(c2c_err_handler_t type, uint32_t *errors)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;
	uint32_t value = 0;

	if (pri_base == 0 || sec_base == 0) {
		ERROR("%s: C2CC must be initialized first.\n", __func__);
		return false;
	}

	switch (type) {
	case C2C_HANDLER_NON_CRITICAL_INT:
		value = ADI_C2CC_READ_NON_CRITICAL_INT_EN(pri_base);
		break;
	case C2C_HANDLER_CRITICAL_INT:
		value = ADI_C2CC_READ_CRITICAL_INT_EN(pri_base);
		break;
	default:
		ERROR("%s: C2CC invalid handler type.\n", __func__);
		return false;
	}

	value = ADI_C2CC_READ_INT_STATUS(pri_base) & value;
	if (errors)
		*errors = value;
	/* clear the interrupt; this is a clear-on-write register */
	ADI_C2CC_WRITE_INT_STATUS(pri_base, value);
	return true;
}

bool adi_c2cc_enable_error_handling(c2c_err_handler_t *params)
{
	uintptr_t pri_base = adi_c2cc_primary_addr_base;
	uintptr_t sec_base = adi_c2cc_secondary_addr_base;
	unsigned int i = 0;
	uint32_t crit_ints = 0;
	uint32_t non_crit_ints = 0;
	uint32_t pin_ints = 0;

	if (pri_base == 0 || sec_base == 0) {
		ERROR("%s: C2CC must be initialized first.\n", __func__);
		return false;
	}

	for (i = 0; i < C2C_ERR_TYPE_MAX; i++) {
		if (params[i] & C2C_HANDLER_NON_CRITICAL_INT)
			non_crit_ints |= (1 << i);
		if (params[i] & C2C_HANDLER_CRITICAL_INT)
			crit_ints |= (1 << i);
		if (params[i] & C2C_HANDLER_PIN_INT)
			pin_ints |= (1 << i);
	}

	ADI_C2CC_WRITE_NON_CRITICAL_INT_EN(pri_base, non_crit_ints);
	ADI_C2CC_WRITE_CRITICAL_INT_EN(pri_base, crit_ints);
	ADI_C2CC_WRITE_PIN_INT_EN(pri_base, pin_ints);

	return true;
}
