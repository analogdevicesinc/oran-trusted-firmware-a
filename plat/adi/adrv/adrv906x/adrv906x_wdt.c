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
#include <adrv906x_gpint.h>
#include <plat_wdt.h>
#include <platform_def.h>

/* SP805 asserts reset after two consecutive countdowns,
 * so it must be programmed with half the platform timeout value. */
#define SP805_CALCULATE_TIMEOUT(timeout_sec)    (timeout_sec / 2U)
#define CONVERT_S_TO_MS         1000
#define CONVERT_FREQ_S_TO_MS(freq)              (freq / CONVERT_S_TO_MS)
#define WDT0_OFFSET_MS          500             /* 0.5 second differential between WDT1 and WDT0 */

/* Dual WDT strategy
 * WDT1 is utilized as interrupt only to raise the GPINT interrupt (unmasked for GPINT1)
 * to signal that the system will reset due to the WDT.
 * WDT0 is utilized to reset the system 0.5 seconds after the WDT1 interrupt is raised,
 * to provide time for components of the system to safely shutdown before the WDT
 * reset occurs (An abrupt system reset shutting down PA can damage the PA).
 *
 * To accomplish this, WDT1 is configured with the platform WDT timeout value. This
 * configures WDT1 to timeout and raise the interrupt after the platform WDT timeout
 * value. WDT0 is utilized to reset the system, which should occur 0.5
 * seconds after the WDT1 interrupt occurs. The first timeout of WDT0 will raise an
 * interrupt and not reset the system. The reset occurs on the second timeout of the
 * WDT. Therefore, WDT0 must be configured with a timeout value of
 * (WDT1_timeout_val + 0.5) / 2
 */
void plat_secure_wdt_start(void)
{
	uint32_t wdt_clk_freq;
	uint32_t wdt0_timeout_load_val;
	uint32_t wdt0_sp805_timeout_seconds_val = SP805_CALCULATE_TIMEOUT(PLAT_WDT_TIMEOUT_DEFAULT_SEC);
	uint32_t wdt1_timeout_load_val;
	uint32_t wdt1_sp805_timeout_val = PLAT_WDT_TIMEOUT_DEFAULT_SEC;

	assert(wdt1_sp805_timeout_val != 0);
	assert(wdt0_sp805_timeout_seconds_val != 0);

	wdt_clk_freq = clk_get_freq(CLK_CTL, CLK_ID_WDT);

	/* Configure WDT0 to reset 0.5 seconds after the first WDT1 interrupt timeout */
	wdt1_timeout_load_val = wdt_clk_freq * wdt1_sp805_timeout_val;
	wdt0_timeout_load_val = wdt_clk_freq * wdt0_sp805_timeout_seconds_val + CONVERT_FREQ_S_TO_MS(wdt_clk_freq) * SP805_CALCULATE_TIMEOUT(WDT0_OFFSET_MS);

	/* Stop WDT0 and WDT1 before starting them. This ensures the driver unlocks
	 * the hardware before configuring (in case it was already configured by a
	 * previous boot stage).
	 */
	sp805_stop(WDOG_TIMER1_BASE);
	sp805_stop(WDOG_TIMER0_BASE);

	/* Configure and start WDT0 and WDT1.
	 * Configure WDT0 with reset and interrupt enabled, WDT1 with only interrupt enabled
	 */
	sp805_start_interrupt_only(WDOG_TIMER1_BASE, wdt1_timeout_load_val);
	sp805_start(WDOG_TIMER0_BASE, wdt0_timeout_load_val);
}

void plat_secure_wdt_stop(void)
{
	sp805_stop(WDOG_TIMER1_BASE);
	sp805_stop(WDOG_TIMER0_BASE);
}

void plat_secure_wdt_refresh(uint32_t seconds)
{
	uint32_t wdt_clk_freq;
	uint32_t wdt0_timeout_load_val;
	uint32_t wdt0_sp805_timeout_seconds_val = SP805_CALCULATE_TIMEOUT(seconds);
	uint32_t wdt1_timeout_load_val;
	uint32_t wdt1_sp805_timeout_val = seconds;

	if (seconds != 0)
		assert(wdt0_sp805_timeout_seconds_val != 0);

	wdt_clk_freq = clk_get_freq(CLK_CTL, CLK_ID_WDT);

	/* Configure WDT0 to reset 0.5 seconds after the first WDT1 interrupt timeout */
	wdt1_timeout_load_val = wdt_clk_freq * wdt1_sp805_timeout_val;
	wdt0_timeout_load_val = wdt_clk_freq * wdt0_sp805_timeout_seconds_val + CONVERT_FREQ_S_TO_MS(wdt_clk_freq) * SP805_CALCULATE_TIMEOUT(WDT0_OFFSET_MS);

	sp805_refresh(WDOG_TIMER1_BASE, wdt1_timeout_load_val);
	sp805_refresh(WDOG_TIMER0_BASE, wdt0_timeout_load_val);
}

void plat_secure_wdt_ping(void)
{
	struct gpint_settings status;

	/* Check WDT1 status before pinging WDT0 and WDT1
	 * If WDT1 expires but this function gets called between
	 * WDT1 expiring and WDT0 expiring (and resetting),
	 * need to let the system die (since WDT1 caused GPINT1 to fire
	 * which notifies for the shutdown of the radio)
	 */
	status.lower_word = 0;
	adrv906x_gpint_get_status(DIG_CORE_BASE, &status);
	if (!(status.lower_word & WATCHDOG_A55_TIMEOUT_PIPED_1_MASK)) {
		sp805_ping(WDOG_TIMER1_BASE);
		status.lower_word = 0;
		adrv906x_gpint_get_status(DIG_CORE_BASE, &status);
		if (!(status.lower_word & WATCHDOG_A55_TIMEOUT_PIPED_1_MASK))
			sp805_ping(WDOG_TIMER0_BASE);
	}
}
