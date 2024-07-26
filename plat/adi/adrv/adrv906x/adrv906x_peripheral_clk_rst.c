/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/adi/adi_qspi.h>
#include <drivers/adi/adi_sdhci.h>
#include <drivers/generic_delay_timer.h>

#include <adrv906x_boot.h>
#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_peripheral_clk_rst.h>
#include <plat_console.h>
#include <plat_boot.h>
#include <plat_wdt.h>

static bool is_boot_dev_init = false;

void plat_pre_clk_switch(void)
{
	/* Save the state of boot device initialization, before
	 * we explicitly deinit it here
	 */
	is_boot_dev_init = plat_is_boot_dev_initialized();

	/* peripheral storage deinit */
	plat_deinit_boot_device();

	/* boot console uart deinit */
	plat_console_boot_end();
}

void plat_post_clk_switch(void)
{
	/* timer init */
	generic_delay_timer_init();

	/* Reinitialize the watchdog timer */
	plat_secure_wdt_start();

	/* boot console uart init */
	plat_console_boot_init();

	/* Reinitialize the boot device if it was previously
	 * initialized before the clock switch
	 */
	if (is_boot_dev_init)
		plat_init_boot_device();
}
