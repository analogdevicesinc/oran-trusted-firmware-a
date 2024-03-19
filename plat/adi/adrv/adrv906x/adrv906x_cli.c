/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>

#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/pll.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar_defs.h>

#include <adrv906x_ddr.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_mmap.h>
#include <plat_cli.h>
#include <plat_device_profile.h>
#include <platform_def.h>

static void plat_end_function(uint8_t *command_buffer, bool help)
{
	INFO("Could not find command in Adrv906x specific command list\n");
	return;
}

/* ATE test mode. Programs the CLKPLL and then returns to interpreter loop. ATE
 * can then run their scan tests as needed. */
static void ate_test_command_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("atetest                            ");
		printf("Programs and locks the CLKPLL, then exits back to interpreter loop. ATE tests can then be run in background.\n");
	} else {
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		printf("CLKPLL programmed, ready for ATE testing.\n");
	}
	return;
}

/* ATE DDR command. Will run the DDR ATE firmware and then print the results to the console*/
static void ate_ddr_command_function(uint8_t *command_buffer, bool help)
{
	int result;

	if (help) {
		printf("ateddr                             ");
		printf("Programs the CLK PLL, then runs the DDR ATE firmware and prints the result to the console.\n");
	} else {
		/* Add region for reading ATE to memory map */
		plat_setup_ns_sram_mmap();
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);

		/* Run DDR firmware */
		result = adrv906x_ddr_ate_test(DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL);
		if (result)
			printf("Error occured while running ATE firmware:%d\n", result);
	}
	return;
}

/* Function to perform the DDR controller init. Step 1 in the DDR iterative init */
static void ddr_iterative_init_pre_reset_command_function(uint8_t *command_buffer, bool help)
{
	int result;

	if (help) {
		printf("ddrpreinit                         ");
		printf("Performs the DDR init before the DDR hardware is pulled out of reset\n");
	} else {
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		printf("Performing pre-reset init for the DDR.\n");
		result = adrv906x_ddr_iterative_init_pre_reset(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred while running DDR pre init:%d\n", result);
	}
}

/* Function to run the DDR Phy training and controller update. Step 3 in the DDR iterative init*/
static void ddr_iterative_init_post_reset_command_function(uint8_t *command_buffer, bool help)
{
	int result;

	if (help) {
		printf("ddrpostinit                        ");
		printf("Perform the training portion of the DDR init after reset and remapping\n");
	} else {
		printf("Performing post-reset init for the DDR.\n");
		result = adrv906x_ddr_iterative_init_post_reset(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred while running DDR post init:%d\n", result);
	}
}

/* Function to run just the ECC and address remapping portions of the DDR init. Step 2 in the DDR iterative init*/
static void ddr_iterative_init_remapping_command_function(uint8_t *command_buffer, bool help)
{
	int result;

	if (help) {
		printf("ddrremapinit                       ");
		printf("Pulls the DDR out of reset and configures remapping register before DDR training\n");
	} else {
		printf("Performing DDR pin and ECC remapping.\n");
		result = adrv906x_ddr_iterative_init_remapping(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred wile running DDR remapping init:%d\n", result);
	}
}

/*This command does a standard init of the ddr, non-interactive*/
static void ddr_init_command_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("ddrinit                            ");
		printf("Initiates a standard init of the DDR.\n");
	} else {
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		printf("Initializing the DDR...\n");
		/*Initialize the DDR*/
		adrv906x_ddr_init();
	}
}

/* This command performs the basic mem test for the DDR */
static void ddr_mem_test_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t base_addr_ddr;
	uint64_t size;
	uint64_t restore;
	int result;

	if (help) {
		printf("ddrmemtest <addr> <size> <restore> ");
		printf("Performs a basic memory test of (hex)<size>, starting from (hex)<addr>.\n");
		printf("                                   ");
		printf("Restores the contents of the memory from before the test if <restore> is 1\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &base_addr_ddr);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &size);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &restore);
		if (command_buffer == NULL)
			return;
		/* Initialize the DDR before attempting a memory test*/
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		result = adrv906x_ddr_init();
		if (result) {
			printf("Error initializing the DDR: %d\n", result);
			return;
		}

		printf("Basic memory test invalidates the cache before running, reset is recommended before attempting any other opertaions after running.\n");
		printf("Running the DDR basic mem test...\n");
		result = adrv906x_ddr_mem_test(base_addr_ddr, size, restore);
		if (result)
			printf("Error occurred during DDR basic mem test:%d\n", result);
		else
			printf("Memory test passed.\n");
	}
}

/* This command performs the extensive mem test for the DDR */
static void ddr_extensive_mem_test_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t base_addr_ddr;
	uint64_t size;
	int result;

	if (help) {
		printf("ddrextmemtest <addr> <size>        ");
		printf("Performs an extensive (MARCH/Walking 1s) memory test of (hex)<size>, starting from (hex)<addr>.\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &base_addr_ddr);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &size);
		if (command_buffer == NULL)
			return;
		/* Initialize the DDR before attempting a memory test*/
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		result = adrv906x_ddr_init();
		if (result) {
			printf("Error initializing the DDR: %d\n", result);
			return;
		}

		printf("Extensive memory test invalidates the cache before running, reset is recommended before attempting any other opertaions after running.\n");
		printf("Running the DDR extensive mem test...\n");
		result = adrv906x_ddr_extensive_mem_test(base_addr_ddr, size);
		if (result)
			printf("Error occurred during DDR extensive mem test:%d\n", result);
		else
			printf("Extensive memory test passed.\n");
	}
}

/* Function to select which DDR signal to send to its observation pin*/
static void  ddr_debug_mux_output_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t group;
	uint64_t instance;
	uint64_t source;
	uint64_t output;

	if (help) {
		printf("ddrmux <group> <inst> <src> <out>  ");
		printf("Routes source from instance of selected group to DDR observation pin.\n");
		printf("                                   ");
		printf("<group>, 0=MASTER, 1=ANIB, 2=DBYTE\n");
		printf("                                   ");
		printf("<inst>, 0-11 for ANIB, 0 or 1 for DBYTE, 0 for MASTER\n");
		printf("                                   ");
		printf("<src>, DDR mux select in hex for desired source, see table 9-3 - 9-5 in PHY databook for values for each signal\n");
		printf("                                   ");
		printf("<out>, Output pin of the debug mux to route the DDR mux output for observation\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &group);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &instance);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &source);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &output);
		if (command_buffer == NULL)
			return;
		if (!adi_adrv906x_debug_xbar_set_output(DEBUG_XBAR_SOURCE_CONTROL_BASE, DEBUG_XBAR_SRC_I_DWC_DDRPHY_DTO, output)) {
			printf("Error setting debug crossbar output.\n");
			return;
		}
		adrv906x_ddr_mux_set_output(DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, group, instance, source);
	}
}

/* Function to select and run a subset of the DDR training firmware tests */
static void ddr_custom_training_test_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t sequence_ctrl;
	uint64_t train_2d;
	int result;

	if (help) {
		printf("ddrtrain <seq> <train2d>           ");
		printf("Runs DDR training firmware with custom set of tests selected.\n");
		printf("                                   ");
		printf("<seq>, SequenceCtrl value in hex for selecting DDR training tests to run. 1=Test will be run, 0= Test skipped\n");
		printf("                                   ");
		printf("See DDR PHY training note section 4 or message block header for list of available tests.\n");
		printf("                                   ");
		printf("<train2D>, Value to signal whether tests are from the 1D or 2D suite of firmware tests. 1=2D, 0=1D \n");
		printf("                                   ");
		printf("NOTE: This test should only be run on an initialized DDR. Run the ddrinit command before attempting to use this command\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &sequence_ctrl);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &train_2d);
		if (command_buffer == NULL)
			return;
		result = adrv906x_ddr_custom_training_test(DDR_PHY_BASE, sequence_ctrl, train_2d);
		if (result)
			printf("Error occurred during DDR training firmware custom test:%d\n", result);
		else
			printf("DDR training firmware custom test passed.\n");
	}
}

/* Function to route the CLKPLL's VCO output to its test points */
static void clkpll_vco_output_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t base = CLKPLL_BASE;

	if (help) {
		printf("clkvcoout                          ");
		printf("Routes the clkpll vco signal out to test pin J21.\n");
	} else {
		pll_configure_vco_test_out(base);
		printf("CLKPLL VCO routing complete\n");
	}
}

/* Vtune out commands. Will route the Vtune output of the CLKPLL to its corresponding test points*/
static void clkpll_vtune_output_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t base = CLKPLL_BASE;

	if (help) {
		printf("clkvtuneout                        ");
		printf("Routes the clkpll vtune signal out to test pins J803.\n");
	} else {
		pll_configure_vtune_test_out(base);
		printf("CLKPLL Vtune routing complete\n");
	}
}

/* Help function to print the debug crossbar current configuration */
static void print_debug_crossbar_status(void)
{
	uint32_t i;

	printf("Debug Xbar Mux:\n");
	printf("---------------\n");
	for (i = 0; i < DEBUG_XBAR_OUT_COUNT; i++) {
		adi_adrv906x_debug_xbar_source_t source = adi_adrv906x_debug_xbar_get_source(DEBUG_XBAR_SOURCE_CONTROL_BASE, i);
		printf("Output %i -> Source %i\n", i, source);
	}
}

/* DEBUG MUX SHOW command */
static void xbar_show_command_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("debugmuxshow                       ");
		printf("Prints the current status of the debug crossbar mux outputs.\n");
	} else {
		print_debug_crossbar_status();
	}
	return;
}

/* DEBUG MUX OUT command. Routes a debug mux source to the selected output */
static void xbar_output_command_function(uint8_t *command_buffer, bool help)
{
	uintptr_t base_addr = DEBUG_XBAR_SOURCE_CONTROL_BASE;
	uint64_t source;
	uint64_t output;

	if (help) {
		printf("debugmuxout <source> <output>      ");
		printf("Selects a debug crossbar mux output from the sources array.\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &source);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &output);
		if (command_buffer == NULL)
			return;
		if (!adi_adrv906x_debug_xbar_set_output(base_addr, source, output))
			printf("Error setting debug crossbar output.\n");
		else
			print_debug_crossbar_status();
	}
	return;
}

/* DEBUG MUX MAP command. Configures the debug mux according a predefined map number (see drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.c) */
static void xbar_map_command_function(uint8_t *command_buffer, bool help)
{
	uintptr_t base_addr = DEBUG_XBAR_SOURCE_CONTROL_BASE;
	uint64_t map_num;

	if (help) {
		printf("debugmuxmap <map_number>           ");
		printf("Configures the debug crossbar mux with a predefined map number (0...n).\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &map_num);
		if (command_buffer == NULL)
			return;
		if (!adi_adrv906x_debug_xbar_set_default_map(base_addr, map_num))
			printf("Error setting debug crossbar map.\n");
		else
			print_debug_crossbar_status();
	}
	return;
}

cli_command_t plat_command_list[] = {
	{ "atetest",	   ate_test_command_function			  },
	{ "ateddr",	   ate_ddr_command_function			  },
	{ "clkvcoout",	   clkpll_vco_output_command_function		  },
	{ "clkvtuneout",   clkpll_vtune_output_command_function		  },
	{ "ddrinit",	   ddr_init_command_function			  },
	{ "ddrextmemtest", ddr_extensive_mem_test_command_function	  },
	{ "ddrmemtest",	   ddr_mem_test_command_function		  },
	{ "ddrmux",	   ddr_debug_mux_output_command_function	  },
	{ "ddrpreinit",	   ddr_iterative_init_pre_reset_command_function  },
	{ "ddrpostinit",   ddr_iterative_init_post_reset_command_function },
	{ "ddrremapinit",  ddr_iterative_init_remapping_command_function  },
	{ "ddrtrain",	   ddr_custom_training_test_command_function	  },
	{ "debugmuxshow",  xbar_show_command_function			  },
	{ "debugmuxout",   xbar_output_command_function			  },
	{ "debugmuxmap",   xbar_map_command_function			  },
	{ "end",	   plat_end_function				  }
};
