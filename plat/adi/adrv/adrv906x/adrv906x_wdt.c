/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <drivers/adi/adrv906x/clk.h>
#include <drivers/arm/sp805.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <plat_wdt.h>
#include <platform_def.h>

/* SP805 asserts reset after two consecutive countdowns,
 * so it must be programmed with half the platform timeout value. */
#define SP805_CALCULATE_TIMEOUT(timeout_sec)    (timeout_sec / 2U)

void plat_secure_wdt_start(void)
{
	uint32_t wdt_clk_freq;
	uint32_t wdt_timeout_load_val;
	uint32_t sp805_timeout_val = SP805_CALCULATE_TIMEOUT(PLAT_WDT_TIMEOUT_DEFAULT_SEC);

	assert(sp805_timeout_val != 0);

	wdt_clk_freq = clk_get_freq(CLK_CTL, CLK_ID_WDT);
	wdt_timeout_load_val = wdt_clk_freq * sp805_timeout_val;

	sp805_start(WDOG_TIMER0_BASE, wdt_timeout_load_val);
}

void plat_secure_wdt_stop(void)
{
	sp805_stop(WDOG_TIMER0_BASE);
}

void plat_secure_wdt_refresh(uint32_t seconds)
{
	uint32_t wdt_clk_freq;
	uint32_t wdt_timeout_load_val;
	uint32_t sp805_timeout_val = SP805_CALCULATE_TIMEOUT(seconds);

	if (seconds != 0)
		assert(sp805_timeout_val != 0);

	wdt_clk_freq = clk_get_freq(CLK_CTL, CLK_ID_WDT);
	wdt_timeout_load_val = wdt_clk_freq * sp805_timeout_val;

	sp805_refresh(WDOG_TIMER0_BASE, wdt_timeout_load_val);
}

void plat_secure_wdt_ping(void)
{
	sp805_ping(WDOG_TIMER0_BASE);
}
