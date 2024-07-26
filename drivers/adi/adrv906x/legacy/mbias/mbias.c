/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <platform_def.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/mbias.h>
#include <drivers/delay_timer.h>

#include "mbias.h"

extern bool plat_is_sysc(void);

/**
 *******************************************************************************
 * Function: mbias_init
 *
 * @brief       Initialize the master bias driver
 *
 * @details     Initialize the master bias driver, including verifying the
 *              hardware PTAT resistor trim calibration is complete.
 *
 * Parameters:
 * @param [in]    base   - Base address
 *
 * @return        error status - NO_ERROR on success.
 *
 *******************************************************************************
 */
extern int mbias_init(const uint64_t base)
{
	int errorCode = NO_ERROR;
	uint8_t rtrimValRbOne;
	uint8_t rtrimValRbZero;
	uint64_t timeout;
	uint8_t trimDoneFlag;

	/* Configure mbias */

	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM0_MBIAS_RTRIM_RESETB(base, 0u);
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM1_MBIAS_RTRIM_RESETB(base, 0u);

	/* Enable mbias. Note that these steps should already be done by an earlier
	 * bootloader (e.g. BootROM) but are included here for completeness.
	 */
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN0_MBIAS_IGEN_PD(base, 0u);
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN1_MBIAS_IGEN_PD(base, 0u);
	WRITE_CORE_DEVCLK_BUFFER_ENABLE(base, 1u);
	udelay(30);

	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN0_MBIAS_TRIM_COMP_PD(base, 0u);
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN1_MBIAS_TRIM_COMP_PD(base, 0u);

	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM0_MBIAS_RTRIM_RESETB(base, 1u);
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM1_MBIAS_RTRIM_RESETB(base, 1u);

	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_BG_CTAT_TRIM0_MBIAS_BG_CTAT(base, 0x9u);
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_BG_CTAT_TRIM1_MBIAS_BG_CTAT(base, 0x9u);

	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_BG_PTAT_TRIM0_MBIAS_BG_PTAT(base, 0x1eu);
	WRITE_CORE_MASTER_BIAS_CTRL_MBIAS_BG_PTAT_TRIM1_MBIAS_BG_PTAT(base, 0x1eu);

	/* Kick off the cal */
	udelay(1);
	WRITE_CORE_MBIAS_CAL_TRIGGER(base, 1);

	/* Wait up to 1ms for RB0 and RB1 cals to complete, checking every 50us */
	timeout = timeout_init_us(1000);
	while (!timeout_elapsed(timeout)) {
		trimDoneFlag = READ_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM_RB0_MBIAS_IGEN_PTATR_TRIM_DONE(base);
		if (trimDoneFlag == 1u) {
			trimDoneFlag = READ_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM_RB1_MBIAS_IGEN_PTATR_TRIM_DONE(base);
			if (trimDoneFlag == 1u)
				break;
		}
		udelay(50);
	}

	/* Verify that RB0 cal is complete */
	trimDoneFlag = READ_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM_RB0_MBIAS_IGEN_PTATR_TRIM_DONE(base);
	if (trimDoneFlag != 1u)
		errorCode = ERROR_MASTER_BIAS_RB0_PTAT_CAL_FAILED;
	if (errorCode == NO_ERROR) {
		/* Verify that RB1 cal is complete. */
		trimDoneFlag = READ_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM_RB1_MBIAS_IGEN_PTATR_TRIM_DONE(base);
		if (trimDoneFlag != 1u)
			errorCode = ERROR_MASTER_BIAS_RB1_PTAT_CAL_FAILED;
	}
	if (errorCode == NO_ERROR) {
		/* Verify RB0/RB1 match. If this fails, it is not fatal. It is just a warning. */
		rtrimValRbZero = READ_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM_RB0_MBIAS_IGEN_PTATR_CODE_RB(base);
		rtrimValRbOne = READ_CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_RTRIM_RB1_MBIAS_IGEN_PTATR_CODE_RB(base);
#ifdef DUMP_PLL_SETTINGS_BL2
		INFO("Master Bias RB0/RB1 PTAT  : 0x%X 0x%X.\n", rtrimValRbZero, rtrimValRbOne);
#endif
		if (rtrimValRbZero != rtrimValRbOne) {
#ifdef DUMP_PLL_SETTINGS_BL2
			INFO("Master Bias RB0/RB1 PTAT values do not match: 0x%X 0x%X.\n", rtrimValRbZero, rtrimValRbOne);
#endif
		}
	}
	return errorCode;
}
