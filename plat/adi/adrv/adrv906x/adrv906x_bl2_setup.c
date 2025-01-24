/*
 * Copyright (c) 2015-2024, ARM Limited and Contributors. All rights reserved.
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
#include <adrv906x_otp.h>
#include <adrv906x_peripheral_clk_rst.h>
#include <adrv906x_tsgen.h>
#include <platform_def.h>
#include <plat_boot.h>
#include <plat_cli.h>
#include <plat_err.h>
#include <plat_pinctrl.h>
#include <plat_setup.h>
#include <plat_wdt.h>

/* BRINGUP TODO: Remove Ethernet PLL defines */
#define CLK_10G_VCO_HZ                                                    (10312500000LL)
#define CLK_25G_VCO_HZ                                                    (12890625000LL)

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

#if DEBUG == 1
	adi_lifecycle_t lifecycle;
#endif

	if (plat_get_dual_tile_no_c2c_enabled())
		if (plat_get_dual_tile_no_c2c_primary())
			adrv906x_release_secondary_reset();

	if (plat_get_dual_tile_enabled()) {
		NOTICE("Enabling secondary tile.\n");
		err = plat_setup_secondary_mmap(false);
		if (err == 0) {
			err = adrv906x_enable_secondary_tile();
			if (err == 0) {
				NOTICE("Secondary tile is enabled.\n");
			} else {
				plat_error_message("Failed to enable secondary tile %d", err);
				plat_set_dual_tile_disabled();
			}
		} else {
			plat_error_message("Failed to setup mmap for secondary tile %d", err);
			plat_set_dual_tile_disabled();
		}
	} else {
		NOTICE("Secondary tile is not enabled.\n");
	}

	/*
	 * Init the ahb bridge.
	 */
	if (plat_is_hardware())
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
		plat_error_message("Failed to initialize MBias %d", err);
		plat_error_handler(-ENXIO);
	}

	/*
	 * Init the ClkPll LDO driver
	 */
	err = ldo_powerup(CLKPLL_BASE, PLL_CLKGEN_PLL);
	if (err) {
		plat_error_message("Failed to initialize LDO %d", err);
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
		if (plat_is_hardware())
			adrv906x_ahb_init(false);

		/*
		 * Init the m-bias driver on the secondary.
		 */
		err = mbias_init(SEC_DIG_CORE_BASE);
		if (err) {
			plat_error_message("Failed to initialize secondary MBias %d", err);
			plat_set_dual_tile_disabled();
		}

		/*
		 * Init the ClkPll LDO driver on the secondary
		 */
		if (err == 0) {
			err = ldo_powerup(SEC_CLKPLL_BASE, PLL_SEC_CLKGEN_PLL);
			if (err) {
				plat_error_message("Failed to initialize secondary LDO %d", err);
				plat_set_dual_tile_disabled();
			}
		}
	}

#if DEBUG == 1
	/* CLI only enabled for debug builds */
	/* On debug builds, can only enter the CLI in the right lifecycle state and if test_enable=1 and test_control=8 */
	if (plat_is_bootrom_bypass_enabled())
		lifecycle = ADI_LIFECYCLE_UNTESTED;
	else
		lifecycle = adi_enclave_get_lifecycle_state(TE_MAILBOX_BASE);

	if (lifecycle == ADI_LIFECYCLE_UNTESTED || lifecycle == ADI_LIFECYCLE_OPEN_SAMPLE \
	    || lifecycle == ADI_LIFECYCLE_TESTED || lifecycle == ADI_LIFECYCLE_ADI_PROV_ENC \
	    || lifecycle == ADI_LIFECYCLE_ADI_RETURN || lifecycle == ADI_LIFECYCLE_END_OF_LIFE) {
		if (plat_get_test_enable()) {
			if (plat_get_test_control() == ADRV906X_TEST_CONTROL_ENTER_CLI) {
				plat_enter_cli();
				clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
			}
		}
	}
#endif

	/* Report the GPINT status (which is sticky after a warm reset)
	 * Do this here, before MCS, which triggers the clock PLL unlock
	 * GPINT status to be set.
	 */
	memset(&settings, 0, sizeof(settings));
	adrv906x_gpint_get_status(DIG_CORE_BASE, &settings);
	NOTICE("GPINT status: U: 0x%08lx L: 0x%08lx\n", settings.upper_word, settings.lower_word);

	/* Multi-Chip Sync */
	INFO("Performing MCS.\n");
	if (!clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), false)) {
		/* Secondary tile in no-c2c case needs to stop and not reset after MCS failure */
		if (plat_get_dual_tile_no_c2c_enabled() && !plat_get_dual_tile_no_c2c_primary()) {
			plat_secure_wdt_stop();
			while (1);
		} else {
			plat_error_message("MCS failed.");
			plat_error_handler(-ENXIO);
		}
	}
	INFO("MCS complete.\n");

	/* Init Primary GPINT */
	adrv906x_gpint_init(DIG_CORE_BASE);

	/* Configure pinmux for GPINT0 on Primary */
	plat_secure_pinctrl_set_group(gpint0_pin_grp, gpint0_pin_grp_members, true, PINCTRL_BASE);

	/* Enable GPINT0 for CLK PLL un-lock on Primary */
	settings.upper_word = CLKPLL_PLL_LOCKED_SYNC_MASK | L4_ECC_ERR_INTR_0_MASK | L4_ECC_ERR_INTR_1_MASK | L4_ECC_ERR_INTR_2_MASK;
	settings.lower_word = GIC_ERR_INT_MASK | NERRIRQ_0_MASK | NERRIRQ_1_MASK | NERRIRQ_2_MASK | NERRIRQ_3_MASK | NERRIRQ_4_MASK;
	adrv906x_gpint_enable(DIG_CORE_BASE, GPINT0, &settings);

	/* Enable GPINT1 for CLK PLL un-lock and WDT1 on Primary */
	settings.upper_word = CLKPLL_PLL_LOCKED_SYNC_MASK;
	settings.lower_word = WATCHDOG_A55_TIMEOUT_PIPED_1_MASK;
	adrv906x_gpint_enable(DIG_CORE_BASE, GPINT1, &settings);

	/* Primary GPINT in dual-tile or dual-no-c2c */
	if (plat_get_dual_tile_enabled() || plat_get_dual_tile_no_c2c_enabled()) {
		/* Configure pinmux for Secondary to Primary pin on Primary */
		plat_secure_pinctrl_set_group(secondary_to_primary_pin_grp, secondary_to_primary_pin_grp_members, true, PINCTRL_BASE);

		/* On Primary, enable GPINT0 for GPINT Interrupt Secondary to Primary */
		settings.upper_word = 0;
		settings.lower_word = GPINT_INTERRUPT_SECONDARY_TO_PRIMARY_MASK;
		adrv906x_gpint_enable(DIG_CORE_BASE, GPINT0, &settings);

		/* On Primary, enable GPINT1 for GPINT Interrupt Secondary to Primary */
		settings.upper_word = 0;
		settings.lower_word = GPINT_INTERRUPT_SECONDARY_TO_PRIMARY_MASK;
		adrv906x_gpint_enable(DIG_CORE_BASE, GPINT1, &settings);
	}

	/* Secondary GPINT in dual-tile */
	if (plat_get_dual_tile_enabled()) {
		/* Init GPINT on Secondary */
		adrv906x_gpint_init(SEC_DIG_CORE_BASE);

		/* Configure pinmux on Secondary */
		plat_secure_pinctrl_set_group(gpint0_pin_grp, gpint0_pin_grp_members, true, SEC_PINCTRL_BASE);
		plat_secure_pinctrl_set_group(secondary_to_primary_pin_grp, secondary_to_primary_pin_grp_members, true, SEC_PINCTRL_BASE);

		/* On Secondary, enable GPINT0 for CLK PLL un-lock and GPINT Interrupt Secondary to Primary */
		settings.upper_word = CLKPLL_PLL_LOCKED_SYNC_MASK;
		settings.lower_word = GPINT_INTERRUPT_SECONDARY_TO_PRIMARY_MASK;
		adrv906x_gpint_enable(SEC_DIG_CORE_BASE, GPINT0, &settings);

		/* On Secondary, enable GPINT1 for CLK PLL un-lock and GPINT Interrupt Secondary to Primary */
		settings.upper_word = CLKPLL_PLL_LOCKED_SYNC_MASK;
		settings.lower_word = GPINT_INTERRUPT_SECONDARY_TO_PRIMARY_MASK;
		adrv906x_gpint_enable(SEC_DIG_CORE_BASE, GPINT1, &settings);
	}

	if (plat_get_dual_tile_enabled()) {
		INFO("Beginning training to enable C2C hi-speed AXI bridge.\n");
		if (!adrv906x_c2c_enable_high_speed()) {
			plat_error_message("Failed to enable C2C hi-speed AXI bridge.");
			/* TODO: Training is expected to fail on SystemC for now. */
			if (!plat_is_sysc())
				plat_set_dual_tile_disabled();
		} else {
			NOTICE("Enabled C2C hi-speed AXI bridge.\n");
		}
	}

	if (plat_is_hardware()) {
		uint64_t pll_freq;
		NOTICE("Initializing Ethernet PLL.\n");
		if (plat_get_ethpll_freq_setting())
			pll_freq = CLK_25G_VCO_HZ;
		else
			pll_freq = CLK_10G_VCO_HZ;
		err = clk_initialize_pll_programming(false, true, plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting());
		if (err) {
			plat_error_message("Failed preparing eth pll for programming.");
			plat_error_handler(-ENXIO);
		}

		if (!err) {
			err = ldo_powerup(ETH_PLL_BASE, PLL_ETHERNET_PLL);
			if (err) {
				plat_error_message("Failed to initialize Ethernet LDO %d", err);
				plat_error_handler(-ENXIO);
			}
		}

		if (!err) {
			err = pll_clk_power_init(ETH_PLL_BASE, DIG_CORE_BASE, pll_freq, ETH_REF_CLK_DFLT, PLL_ETHERNET_PLL);
			if (err) {
				plat_error_message("Problem powering on Ethernet pll = %d", err);
				plat_error_handler(-ENXIO);
			}
		}

		if (!err) {
			err = pll_program(ETH_PLL_BASE, PLL_ETHERNET_PLL);
			if (err) {
				plat_error_message("Problem programming Ethernet pll = %d", err);
				plat_error_handler(-ENXIO);
			}
		}

		if (plat_get_dual_tile_enabled()) {
			err = clk_initialize_pll_programming(true, true, plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting());
			if (err) {
				plat_error_message("Failed preparing eth pll for programming.");
				plat_error_handler(-ENXIO);
			}

			if (!err) {
				err = ldo_powerup(SEC_ETH_PLL_BASE, PLL_ETHERNET_PLL);
				if (err) {
					plat_error_message("Failed to initialize Ethernet LDO %d", err);
					plat_error_handler(-ENXIO);
				}
			}

			if (!err) {
				err = pll_clk_power_init(SEC_ETH_PLL_BASE, DIG_CORE_BASE, pll_freq, ETH_REF_CLK_DFLT, PLL_ETHERNET_PLL);
				if (err) {
					plat_error_message("Problem powering on Ethernet pll = %d", err);
					plat_error_handler(-ENXIO);
				}
			}

			if (!err) {
				err = pll_program(SEC_ETH_PLL_BASE, PLL_ETHERNET_PLL);
				if (err) {
					plat_error_message("Problem programming Ethernet pll = %d", err);
					plat_error_handler(-ENXIO);
				}
			}
		}
	}

	if (plat_check_ddr_size()) {
		plat_error_message("Total DRAM size too large, max %lx", MAX_DDR_SIZE);
		plat_error_handler(-ENXIO);
	}

	NOTICE("Initializing DDR.\n");
	err = adrv906x_ddr_init();
	if (err != 0) {
		plat_error_message("Failed to enable DDR %d", err);
		plat_error_handler(-ENXIO);
	}

	/* Skip printing clock info on Protium and Palladium since it is time consuming */
	if (!plat_is_protium() && !plat_is_palladium()) {
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
				plat_error_message("Failed to load secondary image %d", err);
				plat_set_dual_tile_disabled();
			}
		} else {
			plat_error_message("Secondary DRAM size 0x%lx is too small. Must be at least 0x%lx bytes.", dram_size, SEC_DRAM_SIZE_MIN);
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
	/* Init OTP driver */
	adrv906x_otp_init_driver();

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
