/*
 * Copyright (c) 2015-2024, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <drivers/generic_delay_timer.h>
#include <lib/libc/errno.h>
#include <plat/common/platform.h>
#include <plat_pinctrl.h>

#include <drivers/adi/adi_te_interface.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/adrv906x_gpio.h>
#ifdef TEST_FRAMEWORK
#include <drivers/adi/test_framework.h>
#endif

#include <adrv906x_board.h>
#include <adrv906x_boot.h>
#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_mmap.h>
#include <adrv906x_otp.h>
#include <adrv906x_peripheral_clk_rst.h>
#include <adrv906x_tsgen.h>
#include <platform_def.h>
#include <plat_boot.h>
#include <plat_console.h>
#include <plat_err.h>
#include <plat_setup.h>
#include <plat_te.h>
#include <plat_wdt.h>

/*
 * Check that BL_RAM is above NS_SRAM
 * Check that SHARED_RAM is below NS_SRAM
 */
CASSERT(BL_RAM_BASE >= NS_SRAM_BASE + NS_SRAM_SIZE, assert_bl_ram_above_ns_region);
CASSERT(SHARED_RAM_BASE + SHARED_RAM_SIZE <= NS_SRAM_BASE, assert_shared_ram_below_ns_region);

void plat_bl1_early_setup(void)
{
	/* Initialize the clock framework */
	clk_init(CLK_CTL, CLKPLL_BASE, DIG_CORE_BASE, plat_pre_clk_switch, plat_post_clk_switch);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_DEVCLK, DEVCLK_FREQ_DFLT);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_ROSC, ROSC_FREQ_DFLT);

	/* Setup the watchdog timer after the clock framework is initialized.
	 * bl1_early_platform_setup() will do this for us later, but given that Adrv906x has so
	 * much low-level, early clock initialization, it is necessary to start it here, now.
	 */
	plat_secure_wdt_start();

	/* Enable CCS600_TSGEN timestamp generator module */
	plat_tsgen_enable(PLAT_TSGEN_BASE);

	/* Enable system timer.
	 * bl1_platform_setup() will do this for us later, but since the early clock initialization
	 * in Adrv906x needs to utilize the delay timer, it is necessary to start it here, now.
	 */
	generic_delay_timer_init();

	/* Bring up the external clock chip, if not already done */
	if (plat_get_boot_clk_sel() == CLK_SEL_ROSC_CLK)
		plat_clkdev_init();

	if (plat_is_devclk_available() == false) {
		/* TODO: Remove additional clock init for bootrom bypass after bringup */
		clk_init_devclk(CLK_CTL, DIG_CORE_BASE);
		if (plat_is_devclk_available() == false) {
			/* Expected to be booting from external device clock, but hardware
			 * has detected that the device clock signal is not present.
			 * This is unrecoverable. Halt the boot.
			 */
			plat_console_boot_init();
			plat_error_message("No device clock signal present.");
			plat_board_system_reset();
		}
	}

	/* Ensure the current clock source is devclk.
	 * Note that the BootROM may have already done this.
	 * We do this regardless  because it calls plat_pre_clk_switch(),
	 * which will deinitialize the current boot device (which is desired here).
	 */
	clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
}

void plat_bl1_setup(void)
{
#ifdef TEST_FRAMEWORK
	/* Function to test driver code.
	 * Keep this first to skip all platform initialization.
	 */
	test_main();
	while (1);
#endif

	/* Init OTP driver */
	adrv906x_otp_init_driver();

	/* Initialize GPIO framework */
	adrv906x_gpio_init(GPIO_MODE_SECURE_BASE);

	/* Setup memory region for host boot */
	if (plat_get_boot_device() == PLAT_BOOT_DEVICE_HOST)
		plat_setup_ns_sram_mmap();

	/* Do board-specific setup */
	plat_board_bl1_setup();
}

void plat_enclave_mailbox_init(void)
{
	plat_dprof_init();

	/* Initialize TE mailbox */
	adi_enclave_mailbox_init(TE_MAILBOX_BASE);
}
