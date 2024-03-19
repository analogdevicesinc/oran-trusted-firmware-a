/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <plat/common/platform.h>

#include <drivers/adi/adi_te_interface.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/mbias.h>
#include <drivers/adi/adrv906x/ldo.h>
#include <drivers/adi/adrv906x/pll.h>
#include <drivers/adi/adrv906x/temperature.h>
#include <drivers/adi/adrv906x/adrv906x_gpio.h>
#include <drivers/spi_mem.h>

#include <adrv906x_ahb.h>
#include <adrv906x_board.h>
#include <adrv906x_boot.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_ddr.h>
#include <adrv906x_dual.h>
#include <adrv906x_gpint.h>
#include <adrv906x_mmap.h>
#include <adrv906x_peripheral_clk_rst.h>
#include <adrv906x_tsgen.h>
#include <platform_def.h>
#include <plat_boot.h>
#include <plat_cli.h>
#include <plat_device_profile.h>
#include <plat_pinctrl.h>
#include <plat_setup.h>

/* Sandbox for BL2 hardware initialization.
 * TODO: Clean this up when hardware init is finalized
 */
static void init(void)
{
	size_t dram_size = 0;
	int err = 0;
	extern const plat_pinctrl_settings gpint0_pin_grp[];
	extern const size_t gpint0_pin_grp_members;
	extern const plat_pinctrl_settings secondary_to_primary_pin_grp[];
	extern const size_t secondary_to_primary_pin_grp_members;
	struct gpint_settings settings;
	adi_lifecycle_t lifecycle;

	if (plat_get_dual_tile_enabled()) {
		NOTICE("Enabling secondary tile.\n");
		err = plat_setup_secondary_mmap(false);
		if (err == 0) {
			err = adrv906x_enable_secondary_tile();
			if (err == 0) {
				NOTICE("Secondary tile is enabled.\n");
			} else {
				ERROR("Failed to enable secondary tile %d\n", err);
				plat_set_dual_tile_disabled();
			}
		} else {
			ERROR("Failed to setup mmap for secondary tile %d\n", err);
			plat_set_dual_tile_disabled();
		}
	} else {
		NOTICE("Secondary tile is not enabled.\n");
	}

	/*
	 * Init the ahb bridge.
	 */
	/* TODO: Enable after silicon bringup */
	if (plat_is_sysc())
		adrv906x_ahb_init(true);

	/*
	 * Init the temp sensor driver .
	 * This only needs to be called once to init both the primary and secondary tile
	 */
	tempr_init();

	/*
	 * Init the m-bias driver.
	 */
	err = mbias_init(DIG_CORE_BASE);
	if (err) {
		ERROR("Failed to initialize MBias %d\n", err);
		plat_error_handler(-ENXIO);
	}

	/*
	 * Init the ClkPll LDO driver
	 */
	err = ldo_powerup(CLKPLL_BASE);
	if (err) {
		ERROR("Failed to initialize LDO %d\n", err);
		plat_error_handler(-ENXIO);
	}

	if (plat_get_dual_tile_enabled()) {
		/* Initialize the secondary clock driver and switch secondary to devclk */
		clk_init(SEC_CLK_CTL, SEC_CLKPLL_BASE, SEC_DIG_CORE_BASE, NULL, NULL);
		clk_notify_src_freq_change(SEC_CLK_CTL, CLK_SRC_DEVCLK, DEVCLK_FREQ_DFLT);
		clk_notify_src_freq_change(SEC_CLK_CTL, CLK_SRC_ROSC, ROSC_FREQ_DFLT);
		clk_set_src(SEC_CLK_CTL, CLK_SRC_DEVCLK);

		/*
		 * Init the ahb bridge.
		 */
		/* TODO: Enable after silicon bringup */
		if (plat_is_sysc())
			adrv906x_ahb_init(false);

		/*
		 * Init the m-bias driver on the secondary.
		 */
		err = mbias_init(SEC_DIG_CORE_BASE);
		if (err) {
			ERROR("Failed to initialize secondary MBias %d\n", err);
			plat_set_dual_tile_disabled();
		}

		/*
		 * Init the ClkPll LDO driver on the secondary
		 */
		if (err == 0) {
			err = ldo_powerup(SEC_CLKPLL_BASE);
			if (err) {
				ERROR("Failed to initialize secondary LDO %d\n", err);
				plat_set_dual_tile_disabled();
			}
		}
	}

	/* Can only enter the CLI in the right lifecycle state and if test_enable=1 and test_control=8*/
	lifecycle = adi_enclave_get_lifecycle_state(TE_MAILBOX_BASE);
	if (lifecycle != ADI_LIFECYCLE_CUST1_PROV_HOST \
	    && lifecycle != ADI_LIFECYCLE_DEPLOYED \
	    && lifecycle != ADI_LIFECYCLE_CUST1_RETURN) {
		if (plat_get_test_enable()) {
			if (plat_get_test_control() == ADRV906X_TEST_CONTROL_ENTER_CLI) {
				plat_enter_cli();
				clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
			}
		}
	}

	/* Multi-Chip Sync */
	INFO("Performing MCS.\n");
	if (!clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), false)) {
		ERROR("MCS failed.\n");
		plat_error_handler(-ENXIO);
	}
	INFO("MCS complete.\n");

	/* Init Primary GPINT and enable warm reset */
	adrv906x_gpint_init(DIG_CORE_BASE);
	adrv906x_gpint_warm_reset_enable();

	/* Enable GPINT0 for CLK PLL un-lock on Primary*/
	settings.upper_word = CLKPLL_PLL_LOCKED_SYNC_MASK;
	settings.lower_word = 0;
	adrv906x_gpint_enable(DIG_CORE_BASE, GPINT0, &settings);
	plat_secure_pinctrl_set_group(gpint0_pin_grp, gpint0_pin_grp_members, true, PINCTRL_BASE);

	if (plat_get_dual_tile_enabled()) {
		/* Init GPINT on Secondary */
		adrv906x_gpint_init(SEC_DIG_CORE_BASE);

		/* Enable GPINT0 for CLK PLL un-lock on Secondary*/
		settings.upper_word = CLKPLL_PLL_LOCKED_SYNC_MASK;
		settings.lower_word = 0;
		adrv906x_gpint_enable(SEC_DIG_CORE_BASE, GPINT0, &settings);
		plat_secure_pinctrl_set_group(gpint0_pin_grp, gpint0_pin_grp_members, true, SEC_PINCTRL_BASE);

		/* Enable GPINT0 on Primary for GPINT Interrupt Secondary to Primary */
		plat_secure_pinctrl_set_group(secondary_to_primary_pin_grp, secondary_to_primary_pin_grp_members, true, PINCTRL_BASE);
		settings.upper_word = 0;
		settings.lower_word = GPINT_INTERRUPT_SECONDARY_TO_PRIMARY_MASK;
		adrv906x_gpint_enable(DIG_CORE_BASE, GPINT0, &settings);
	}

	if (plat_get_dual_tile_enabled()) {
		INFO("Beginning training to enable C2C hi-speed AXI bridge.\n");
		if (!adrv906x_c2c_enable_high_speed()) {
			ERROR("Failed to enable C2C hi-speed AXI bridge.\n");
			/* TODO: Training is expected to fail on SystemC for now. */
			if (!plat_is_sysc())
				plat_set_dual_tile_disabled();
		} else {
			NOTICE("Enabled C2C hi-speed AXI bridge.\n");
		}
	}

	if (plat_check_ddr_size()) {
		ERROR("Total DRAM size too large, max %lx\n", MAX_DDR_SIZE);
		plat_error_handler(-ENXIO);
	}

	NOTICE("Initializing DDR.\n");
	err = adrv906x_ddr_init();
	if (err != 0) {
		ERROR("Failed to enable DDR %d\n", err);
		plat_error_handler(-ENXIO);
	}

	/* Skip printing clock info on Protium and Palladium since it is time consuming */
	if (plat_is_sysc() == true) {
		clk_print_info(CLK_CTL);
		if (plat_get_dual_tile_enabled())
			clk_print_info(SEC_CLK_CTL);
	}

	/* Load the secondary image (bootloader) on secondary-linux-enabled systems */
	if (plat_get_secondary_linux_enabled()) {
		/* Verify secondary DRAM size is sufficient for Linux */
		dram_size = plat_get_secondary_dram_size();
		if (dram_size >= SEC_DRAM_SIZE_MIN) {
			NOTICE("Loading secondary image.\n");
			err = adrv906x_load_secondary_image();
			if (err == 0) {
				NOTICE("Secondary image load complete.\n");
			} else {
				ERROR("Failed to load secondary image %d\n", err);
				plat_set_dual_tile_disabled();
			}
		} else {
			ERROR("Secondary DRAM size 0x%lx is too small. Must be at least 0x%lx bytes.\n", dram_size, SEC_DRAM_SIZE_MIN);
			plat_set_dual_tile_disabled();
		}
	}

	return;
}

void plat_bl2_early_setup(void)
{
	/* Initialize the clock framework */
	clk_init(CLK_CTL, CLKPLL_BASE, DIG_CORE_BASE, plat_pre_clk_switch, plat_post_clk_switch);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_DEVCLK, DEVCLK_FREQ_DFLT);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_ROSC, ROSC_FREQ_DFLT);

	/* Enable CCS600_TSGEN timestamp generator module */
	plat_tsgen_enable(PLAT_TSGEN_BASE);
}

void plat_bl2_setup(void)
{
	/* Setup the device profile */
	plat_dprof_init();

	/* Initialize GPIO framework */
	adrv906x_gpio_init(GPIO_MODE_SECURE_BASE);

	/* Perform BL2-specific hardware init */
	init();

	/* Setup memory region for host boot */
	if (plat_get_boot_device() == PLAT_BOOT_DEVICE_HOST)
		plat_setup_ns_sram_mmap();

	/* Do board-specific setup */
	plat_board_bl2_setup();
}
