/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <errno.h>

#include <lib/mmio.h>

#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adi_c2cc.h>
#include <drivers/adi/adi_otp.h>
#include <drivers/adi/adi_twi_i2c.h>
#include <drivers/adi/adrv906x/ldo.h>
#include <drivers/adi/adrv906x/pll.h>
#include <drivers/adi/adrv906x/temperature.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar_defs.h>
#include <drivers/mmc.h>

#include <adrv906x_ddr.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_dual.h>
#include <adrv906x_emac_common_def.h>
#include <adrv906x_mmap.h>
#include <adrv906x_otp.h>
#include <plat_boot.h>
#include <plat_cli.h>
#include <plat_err.h>
#include <plat_security.h>
#include <plat_wdt.h>
#include <platform_def.h>

#define CLK_7G_VCO_HZ                                                     (7864320000LL)
#define CLK_10G_VCO_HZ                                                    (10312500000LL)
#define CLK_11G_VCO_HZ                                                    (11796480000LL)
#define CLK_25G_VCO_HZ                                                    (12890625000LL)

/*
 * BootROM in OTP
 */
#define BOOTROM_IN_OTP_BASE_ADDR 0x700
#define BOOTROM_DATA_BUFFER_SIZE 4
#define BOOTROM_DATA_MASK 0xFFFFFFFF
#define BOOTROM_SRAM_LOAD_ADDR 0x00100000
#define OTP_QRR_BIT_ADDRESS 0x1
#define LAST_SAMANA_OTP_ADDRESS 0x3DFF
#define BYTES_PER_OTP_WRITE 4

/* C2C */
typedef enum {
	C2C_STATE_NOT_CONFIGURED,
	C2C_STATE_CONFIGURED,
	C2C_STATE_TRAINED,
	C2C_STATE_CALIBRATED,
} c2c_state_t;

uint32_t p2s_stats[ADI_C2C_TRIM_MAX] = { 0 };
uint32_t s2p_stats[ADI_C2C_TRIM_MAX] = { 0 };
c2c_state_t c2c_train_state = C2C_STATE_NOT_CONFIGURED;
c2c_mode_t c2c_mode = C2C_MODE_NORMAL;

static int adrv906x_dump_otp_memory(const uintptr_t start_addr, const int size);
static int adrv906x_program_bootrom_in_otp(const uintptr_t mem_ctrl_base, uintptr_t base_addr_image, int num_bytes);

static int plat_end_function(uint8_t *command_buffer, bool help)
{
	return 0;
}

/* ATE test mode. Programs the CLKPLL and then returns to interpreter loop. ATE
 * can then run their scan tests as needed. */
static int ate_test_command_function(uint8_t *command_buffer, bool help)
{
	bool ret = true;

	if (help) {
		printf("atetest                            ");
		printf("Programs and locks the CLKPLL, then exits back to interpreter loop. ATE tests can then be run in background.\n");
	} else {
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();
		printf("CLKPLL programmed, ready for ATE testing.\n");
	}
	return 0;
}

/* ATE DDR command. Will run the DDR ATE firmware and then print the results to the console*/
static int ate_ddr_command_function(uint8_t *command_buffer, bool help)
{
	int result = 0;
	bool ret = true;

	if (help) {
		printf("ateddr                             ");
		printf("Programs the CLK PLL, then runs the DDR ATE firmware and prints the result to the console.\n");
	} else {
		/* Add region for reading ATE to memory map */
		if ((result = plat_setup_ns_sram_mmap()) != 0)
			return result;

		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();

		printf("Preparing DDR for ATE testing.\n");
		result = adrv906x_ddr_iterative_init_pre_reset(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result) {
			printf("Error occurred while running DDR pre init:%d\n", result);
			return result;
		}

		result = adrv906x_ddr_iterative_init_remapping(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result) {
			printf("Error occurred wile running DDR remapping init:%d\n", result);
			return result;
		}

		/* Run DDR firmware */
		result = adrv906x_ddr_ate_test(DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL);
		if (result)
			printf("Error occured while running ATE firmware:%d\n", result);
	}
	return result;
}

/* Function to perform the DDR controller init. Step 1 in the DDR iterative init */
static int ddr_iterative_init_pre_reset_command_function(uint8_t *command_buffer, bool help)
{
	int result = 0;
	bool ret = true;

	if (help) {
		printf("ddrpreinit                         ");
		printf("Performs the DDR init before the DDR hardware is pulled out of reset\n");
	} else {
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();
		printf("Performing pre-reset init for the DDR.\n");
		result = adrv906x_ddr_iterative_init_pre_reset(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred while running DDR pre init:%d\n", result);
	}
	return result;
}

/* Function to run the DDR Phy training and controller update. Step 3 in the DDR iterative init*/
static int ddr_iterative_init_post_reset_command_function(uint8_t *command_buffer, bool help)
{
	int result = 0;

	if (help) {
		printf("ddrpostinit                        ");
		printf("Perform the training portion of the DDR init after reset and remapping\n");
	} else {
		printf("Performing post-reset init for the DDR.\n");
		result = adrv906x_ddr_iterative_init_post_reset(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred while running DDR post init:%d\n", result);
	}
	return result;
}

/* Function to run just the ECC and address remapping portions of the DDR init. Step 2 in the DDR iterative init*/
static int ddr_iterative_init_remapping_command_function(uint8_t *command_buffer, bool help)
{
	int result = 0;

	if (help) {
		printf("ddrremapinit                       ");
		printf("Pulls the DDR out of reset and configures remapping register before DDR training\n");
	} else {
		printf("Performing DDR pin and ECC remapping.\n");
		result = adrv906x_ddr_iterative_init_remapping(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred while running DDR remapping init:%d\n", result);
	}
	return result;
}

/*This command does a standard init of the ddr, non-interactive*/
static int ddr_init_command_function(uint8_t *command_buffer, bool help)
{
	int result = 0;
	bool ret = true;

	if (help) {
		printf("ddrinit                            ");
		printf("Initiates a standard init of the DDR.\n");
	} else {
		/* Program CLK PLL */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();
		printf("Initializing the DDR...\n");
		/*Initialize the DDR*/
		result = adrv906x_ddr_init();
		if (result)
			printf("Error while initializing the DDR:%d\n", result);
	}
	return result;
}

/* This command sets custom parameters used for subsequent DDR init calls */
static int ddr_custom_values_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t parameter;
	ddr_custom_values_t ddr_custom_parameters;

	if (help) {
		printf("ddrvalues <tximp> <txodt> <calrate> <rdptrinit>\n");
		printf("                                   ");
		printf("Set custom values for the TxImpedanceCtrl*_B*_P*, TxODTDRVSTREN_B*_P*, MasterCalRate, ARdPtrInitVal to be used by DDR.\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &parameter);
		if (command_buffer == NULL)
			return -1;
		ddr_custom_parameters.data_tx_impedance_ctrl = (uint32_t)parameter;

		command_buffer = parse_next_param(16, command_buffer, &parameter);
		if (command_buffer == NULL)
			return -1;
		ddr_custom_parameters.data_tx_odt_drive_strength = (uint32_t)parameter;

		command_buffer = parse_next_param(16, command_buffer, &parameter);
		if (command_buffer == NULL)
			return -1;
		ddr_custom_parameters.master_cal_rate = (uint32_t)parameter;

		command_buffer = parse_next_param(16, command_buffer, &parameter);
		if (command_buffer == NULL)
			return -1;
		ddr_custom_parameters.ard_ptr_init_val = (uint32_t)parameter;

		/* Set the parameters for later DDR init */
		printf("Setting DDR custom parameters.\n");
		adrv906x_ddr_set_custom_parameters(&ddr_custom_parameters);
	}

	return 0;
}

/* This command performs the basic mem test for the DDR */
static int ddr_mem_test_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t base_addr_ddr;
	uint64_t size;
	uint64_t restore;
	int result = 0;
	bool ret = true;

	if (help) {
		printf("ddrmemtest <addr> <size> <restore> ");
		printf("Performs a basic memory test of (hex)<size>, starting from (hex)<addr>.\n");
		printf("                                   ");
		printf("Restores the contents of the memory from before the test if <restore> is 1\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &base_addr_ddr);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &size);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &restore);
		if (command_buffer == NULL)
			return -1;

		/* Initialize the DDR before attempting a memory test*/
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();

		/* Configure TZC */
		plat_security_setup();

		result = adrv906x_ddr_init();
		if (result) {
			printf("Error initializing the DDR: %d\n", result);
			return result;
		}

		printf("Basic memory test invalidates the cache before running, reset is recommended before attempting any other operations after running.\n");
		printf("Running the DDR basic mem test...\n");
		result = adrv906x_ddr_mem_test(base_addr_ddr, size, restore);
		if (result)
			printf("Error occurred during DDR basic mem test:%d\n", result);
		else
			printf("Memory test passed.\n");
	}
	return result;
}

/* This command performs the extensive mem test for the DDR */
static int ddr_extensive_mem_test_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t base_addr_ddr;
	uint64_t size;
	int result = 0;
	bool ret = true;

	if (help) {
		printf("ddrextmemtest <addr> <size>        ");
		printf("Performs an extensive (MARCH/Walking 1s) memory test of (hex)<size>, starting from (hex)<addr>.\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &base_addr_ddr);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &size);
		if (command_buffer == NULL)
			return -1;

		/* Initialize the DDR before attempting a memory test*/
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();

		/* Configure TZC */
		plat_security_setup();

		result = adrv906x_ddr_init();
		if (result) {
			printf("Error initializing the DDR: %d\n", result);
			return result;
		}

		printf("Extensive memory test invalidates the cache before running, reset is recommended before attempting any other operations after running.\n");
		printf("Running the DDR extensive mem test...\n");
		result = adrv906x_ddr_extensive_mem_test(base_addr_ddr, size);
		if (result)
			printf("Error occurred during DDR extensive mem test:%d\n", result);
		else
			printf("Extensive memory test passed.\n");
	}
	return result;
}

/* Function to select which DDR signal to send to its observation pin*/
static int  ddr_debug_mux_output_command_function(uint8_t *command_buffer, bool help)
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
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &instance);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &source);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &output);
		if (command_buffer == NULL)
			return -1;

		if (!adi_adrv906x_debug_xbar_set_output(DEBUG_XBAR_SOURCE_CONTROL_BASE, DEBUG_XBAR_SRC_I_DWC_DDRPHY_DTO, output)) {
			printf("Error setting debug crossbar output.\n");
			return -1;
		}
		adrv906x_ddr_mux_set_output(DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, group, instance, source);
	}
	return 0;
}

/* Function to select and run a subset of the DDR training firmware tests */
static int ddr_custom_training_test_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t sequence_ctrl;
	uint64_t train_2d;
	int result = 0;
	bool ret = true;

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
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &train_2d);
		if (command_buffer == NULL)
			return -1;

		/* Initialize the clocks and the TZC in case user runs custom training without a normal DDR init */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		ret = clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		if (ret == false)
			return -1;

		plat_secure_wdt_stop();

		/* Configure TZC */
		plat_security_setup();

		result = adrv906x_ddr_custom_training_test(DDR_PHY_BASE, sequence_ctrl, train_2d);
		if (result)
			printf("Error occurred during DDR training firmware custom test:%d\n", result);
		else
			printf("DDR training firmware custom test passed.\n");
	}
	return result;
}

/* Function to route the CLKPLL and Ethernet PLL VCO output to its test points */
static int pll_vco_output_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t pll;
	uint64_t xbar_pin = 0;
	uintptr_t base;

	if (help) {
		printf("pllvcoout <pll> <output>           ");
		printf("Routes the pll vco signal out to its corresponding test pin\n");
		printf("                                   ");
		printf("<pll> values: 0=CLKPLL,1=SEC_CLK_PLL,2=ETH_PLL,3=SEC_ETH_PLL.\n");
		printf("                                   ");
		printf("Test pin values: J21= Primary CLKPLL, J24 = Secondary CLKPLL, = Primary ETH PLL, =Secondary ETH PLL)\n");
		printf("                                   ");
		printf("<output> -  Chooses the DEBUG XBAR pin to route the signal to, 0-7. (Only applicable for ETH PLLs)\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &pll);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &xbar_pin);
		if (command_buffer == NULL && (pll == 2 || pll == 3)) {
			printf("Ethernet PLL VCO routing needs a second parameter.\n");
			return -1;
		}

		switch (pll) {
		case 0:
			base = CLKPLL_BASE;
			break;
		case 1:
			base = SEC_CLKPLL_BASE;
			break;
		case 2:
			base = ETH_PLL_BASE;
			mmio_write_32(EMAC_COMMON_BASE + EMAC_COMMON_CONTROL_OUT_GPIO_SELECTION0, ETH_PLL_16_CLK_DIV16);
			if (!adi_adrv906x_debug_xbar_set_output(DEBUG_XBAR_SOURCE_CONTROL_BASE, DEBUG_XBAR_SRC_I_ETH_DBG_TO_GPIO_0, xbar_pin)) {
				printf("Error setting debug crossbar output.\n");
				return -1;
			}
			break;
		case 3:
			base = SEC_ETH_PLL_BASE;
			mmio_write_32(SEC_EMAC_COMMON_BASE + EMAC_COMMON_CONTROL_OUT_GPIO_SELECTION0, ETH_PLL_16_CLK_DIV16);
			if (!adi_adrv906x_debug_xbar_set_output(SEC_DEBUG_XBAR_SOURCE_CONTROL_BASE, DEBUG_XBAR_SRC_I_ETH_DBG_TO_GPIO_0, xbar_pin)) {
				printf("Error setting debug crossbar output.\n");
				return -1;
			}
			break;
		default:
			printf("Unsupported pll selected, try again.\n");
			return -1;
		}

		pll_configure_vco_test_out(base);
		printf("PLL VCO routing complete\n");
	}
	return 0;
}

/* Vtune out command. Will route the Vtune output of the CLKPLL or the ETH PLL to its corresponding test point*/
static int pll_vtune_output_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t pll;
	uintptr_t base;

	if (help) {
		printf("pllvtuneout <pll>                  ");
		printf("Routes the pll vtune signal out to its corresponding test pin\n");
		printf("                                   ");
		printf("<pll> values: 0=CLKPLL,1=SEC_CLK_PLL,2=ETH_PLL,3=SEC_ETH_PLL.\n");
		printf("                                   ");
		printf("Test pin values: J803=Primary CLKPLL, J807=Secondary CLKPLL, J804=Primary ETH PLL, J808=Secondary ETH PLL)\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &pll);
		if (command_buffer == NULL)
			return -1;

		switch (pll) {
		case 0:
			base = CLKPLL_BASE;
			break;
		case 1:
			base = SEC_CLKPLL_BASE;
			break;
		case 2:
			base = ETH_PLL_BASE;
			break;
		case 3:
			base = SEC_ETH_PLL_BASE;
			break;
		default:
			printf("Unsupported pll selected, try again.\n");
			return -1;
		}

		pll_configure_vtune_test_out(base);
		printf("PLL Vtune routing complete\n");
	}
	return 0;
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
static int xbar_show_command_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("debugmuxshow                       ");
		printf("Prints the current status of the debug crossbar mux outputs.\n");
	} else {
		print_debug_crossbar_status();
	}
	return 0;
}

/* DEBUG MUX OUT command. Routes a debug mux source to the selected output */
static int xbar_output_command_function(uint8_t *command_buffer, bool help)
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
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &output);
		if (command_buffer == NULL)
			return -1;

		if (!adi_adrv906x_debug_xbar_set_output(base_addr, source, output)) {
			printf("Error setting debug crossbar output.\n");
			return -1;
		} else {
			print_debug_crossbar_status();
		}
	}
	return 0;
}

/* DEBUG MUX MAP command. Configures the debug mux according a predefined map number (see drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.c) */
static int xbar_map_command_function(uint8_t *command_buffer, bool help)
{
	uintptr_t base_addr = DEBUG_XBAR_SOURCE_CONTROL_BASE;
	uint64_t map_num;

	if (help) {
		printf("debugmuxmap <map_number>           ");
		printf("Configures the debug crossbar mux with a predefined map number (0...n).\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &map_num);
		if (command_buffer == NULL)
			return -1;

		if (!adi_adrv906x_debug_xbar_set_default_map(base_addr, map_num)) {
			printf("Error setting debug crossbar map.\n");
			return -1;
		} else {
			print_debug_crossbar_status();
		}
	}
	return 0;
}

static int bootrom_in_otp_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t num_bytes;
	int result = 0;

	if (help) {
		printf("bootrominotp <num_bytes>           ");
		printf("Programs the OTP with the BootROM image loaded at 0x0010_0000 of length num_bytes.\n");
	} else {
		/* Add region for reading image to memory map */
		if ((result = plat_setup_ns_sram_mmap()) != 0)
			return result;

		command_buffer = parse_next_param(16, command_buffer, &num_bytes);
		if (command_buffer == NULL)
			return -1;

		if ((result = adrv906x_program_bootrom_in_otp(OTP_BASE, BOOTROM_SRAM_LOAD_ADDR, num_bytes)) != 0)
			printf("Error programming BootROM into OTP.\n");
		else
			printf("BootROM programming in OTP successful.\n");
	}
	return result;
}

static int otp_qrr_command_function(uint8_t *command_buffer, bool help)
{
	uint32_t qrr;

	if (help) {
		printf("otpqrr                             ");
		printf("Programs the QRR bit in the Samana OTP region\n");
	} else {
		if (otp_write(OTP_BASE, OTP_QRR_BIT_ADDRESS, 0x1, OTP_ECC_ON) != ADI_OTP_SUCCESS) {
			printf("Error programming the QRR bit in OTP.\n");
			return -1;
		} else {
			otp_read(OTP_BASE, OTP_QRR_BIT_ADDRESS, &qrr, OTP_ECC_ON);
			if (qrr != 0x1) {
				printf("Error programming the QRR bit in OTP.\n");
				return -1;
			} else {
				printf("Programmed the QRR bit in OTP successfully.\n");
			}
		}
	}
	return 0;
}

static int otp_dump_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t start_addr, size;
	int result = 0;

	if (help) {
		printf("otpdump <addr> <size>              ");
		printf("Dumps (dec)<size> OTP addresses starting from (hex) OTP address\n");
		printf("                                   ");
		printf("i.e. otpdump 0 4 will dump the first four OTP addresses.\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &start_addr);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &size);
		if (command_buffer == NULL)
			return -1;

		if ((result = adrv906x_dump_otp_memory(start_addr, size)) != 0)
			printf("Failed to dump memory from OTP.\n");
	}
	return result;
}

static void print_temp(float temp)
{
	printf("%d.%d C\n", (int)(temp), ((int)(temp * 10)) % 10);
}

static int pll_temp_sensors_read_function(uint8_t *command_buffer, bool help)
{
	int err = 0;

	if (help) {
		printf("plltemp                            ");
		printf("Gets ClockPLL and EthernetPLL temperatures, including the secondary if available\n");
		printf("                                   ");
	} else {
		float clkpll_temp, ethpll_temp;
		if (tempr_read(&clkpll_temp, &ethpll_temp) == 0) {
			printf("ClkPLL:     "); print_temp(clkpll_temp);
			printf("EthPLL:     "); print_temp(ethpll_temp);
		} else {
			printf("Error reading PLL temperature sensors\n");
			err = 1;
		}
		if (plat_get_dual_tile_enabled()) {
			float sec_clkpll_temp, sec_ethpll_temp;
			if (tempr_read(&sec_clkpll_temp, &sec_ethpll_temp) == 0) {
				printf("Sec ClkPLL: "); print_temp(sec_clkpll_temp);
				printf("Sec EthPLL: "); print_temp(sec_ethpll_temp);
			} else {
				printf("Error reading secondary PLL temperature sensors\n");
				err = 1;
			}
		}
	}
	return err;
}

static int program_pll_command_function(uint8_t *command_buffer, bool help)
{
	uintptr_t base_addr, dig_core_base_addr;
	uint64_t pll, freq, pll_freq, ref_clk;
	bool secondary, eth_pll;
	PllSelName_e pll_sel_name;
	int result = 0;

	if (help) {
		printf("programpll <pll> <freq>            ");
		printf("Programs the pll selected with <pll> with the frequency selected with <freq>\n");
		printf("                                   ");
		printf("<pll> values: 0=CLKPLL,1=SEC_CLK_PLL,2=ETH_PLL,3=SEC_ETH_PLL.\n");
		printf("                                   ");
		printf("<freq> values: 7=7GHz,10=10GHz,11=11GHz,25=25GHz\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &pll);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &freq);
		if (command_buffer == NULL)
			return -1;

		switch (pll) {
		case 0:
			base_addr = CLKPLL_BASE;
			dig_core_base_addr = DIG_CORE_BASE;
			pll_sel_name = PLL_CLKGEN_PLL;
			ref_clk = DEVCLK_FREQ_DFLT;
			secondary = false;
			eth_pll = false;
			break;
		case 1:
			base_addr = SEC_CLKPLL_BASE;
			dig_core_base_addr = SEC_DIG_CORE_BASE;
			pll_sel_name = PLL_SEC_CLKGEN_PLL;
			ref_clk = DEVCLK_FREQ_DFLT;
			secondary = true;
			eth_pll = false;
			break;
		case 2:
			base_addr = ETH_PLL_BASE;
			dig_core_base_addr = DIG_CORE_BASE;
			pll_sel_name = PLL_ETHERNET_PLL;
			ref_clk = ETH_REF_CLK_DFLT;
			secondary = false;
			eth_pll = true;
			break;
		case 3:
			base_addr = SEC_ETH_PLL_BASE;
			dig_core_base_addr = SEC_DIG_CORE_BASE;
			pll_sel_name = PLL_SEC_ETHERNET_PLL;
			ref_clk = ETH_REF_CLK_DFLT;
			secondary = true;
			eth_pll = true;
			break;
		default:
			printf("Unsupported pll selected, try again.\n");
			return -1;
		}

		switch (freq) {
		case 7:
			pll_freq = CLK_7G_VCO_HZ;
			break;
		case 10:
			pll_freq = CLK_10G_VCO_HZ;
			break;
		case 11:
			pll_freq = CLK_11G_VCO_HZ;
			break;
		case 25:
			pll_freq = CLK_25G_VCO_HZ;
			break;
		default:
			printf("Unsupported frequency for pll, please try again.\n");
			return -1;
		}

		result = clk_initialize_pll_programming(secondary, eth_pll, plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting());
		if (result) {
			plat_error_message("Failed preparing pll for programming.");
			return result;
		}
		printf("Initializing LDO for pll...\n");
		result = ldo_powerup(base_addr, pll_sel_name);
		if (result) {
			plat_error_message("Failed to initialize LDO %d", result);
			return result;
		}

		printf("Turning on PLL...\n");
		result = pll_clk_power_init(base_addr, dig_core_base_addr, pll_freq, ref_clk, pll_sel_name);
		if (result) {
			plat_error_message("Problem powering on pll = %d", result);
			return result;
		}

		printf("Programming PLL...\n");
		result = pll_program(base_addr, pll_sel_name);
		if (result)
			plat_error_message("Problem programming pll = %d", result);
	}
	return result;
}

static int sd_read_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t block_offset;
	uint64_t num_blocks;
	int result = 0;

	if (help) {
		printf("sdread <blk_offset> <num_blks>     ");
		printf("Reads num_blks blocks from offset blk_offset of SD card to SRAM addr 0x0010_0000.\n");
	} else {
		if (plat_get_boot_device() != PLAT_BOOT_DEVICE_SD_0) {
			printf("Error: Boot device must be set to SD_0\n");
			return -1;
		}

		if ((result = plat_setup_ns_sram_mmap()) != 0)
			return result;

		plat_init_boot_device();

		command_buffer = parse_next_param(10, command_buffer, &block_offset);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &num_blocks);
		if (command_buffer == NULL)
			return -1;

		if (mmc_read_blocks(block_offset, (uintptr_t)0x00100000, num_blocks * 512) != (num_blocks * 512)) {
			printf("Error reading from SD card.\n");
			return -1;
		} else {
			printf("SD card read successful.\n");
		}
	}
	return result;
}

static int i2c_detect_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t bus;
	int result = 0;
	struct adi_i2c_handle hi2c;
	uint8_t i;
	uint8_t dummy;

	if (help) {
		printf("i2cdetect <bus>                    ");
		printf("Finds I2C slaves on the specified bus\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &bus);
		if (command_buffer == NULL)
			return -1;

		switch (bus) {
		case 0:
			hi2c.base = I2C_0_BASE;
			break;
		default:
			printf("Bus not supported\n");
			return -1;
		}
		hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		hi2c.twi_clk = 40000;

		result = adi_twi_i2c_init(&hi2c);
		if (result < 0)
			return result;

		printf("\n");
		printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
		for (i = 0; i < 0x7F; i++) {
			if (i % 0x10 == 0) printf("\n%02x: ", i);
			if (i < 0x03 || i > 0x77) {
				/* Reserved addresses */
				printf("   ");
				continue;
			}
			result = adi_twi_i2c_read(&hi2c, i, 0, 1, &dummy, 1);
			if (result < 0) {
				printf("-- ");
				return result;
			} else {
				printf("%02x ", i);
			}
		}
		printf("\n");
	}
	return result;
}

static int i2c_read_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t bus;
	uint64_t slave;
	uint64_t addr;
	uint64_t addr_len;
	uint64_t num_bytes;
	int result = 0;
	struct adi_i2c_handle hi2c;
	uint8_t i;
	uint8_t buffer[100] = { 0 };

	if (help) {
		printf("i2cread <b> <s> <a> <l> <n>        ");
		printf("Reads (dec)<n> bytes from address (hex)<a> (<l> address length) of slave (hex)<s> in I2C bus <b>\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &bus);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &slave);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &addr);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &addr_len);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &num_bytes);
		if (command_buffer == NULL)
			return -1;

		switch (bus) {
		case 0:
			hi2c.base = I2C_0_BASE;
			break;
		default:
			printf("Bus not supported\n");
			return -1;
		}
		hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		hi2c.twi_clk = 40000;

		result = adi_twi_i2c_init(&hi2c);
		if (result < 0)
			return result;

		printf("Read slave 0x%lx address 0x%lx %ld bytes\n", slave, addr, num_bytes);
		result = adi_twi_i2c_read(&hi2c, slave, addr, addr_len, buffer, num_bytes);
		if (result < 0) {
			printf("Read error: %d\n", result);
			return result;
		}

		for (i = 0; i < num_bytes; i++)
			printf("0x%02x ", buffer[i]);
		printf("\n");
	}
	return result;
}

static int i2c_write_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t bus;
	uint64_t slave;
	uint64_t addr;
	uint64_t addr_len;
	uint64_t aux;
	uint64_t num_bytes;
	int result = 0;
	struct adi_i2c_handle hi2c;
	uint8_t i;
	uint8_t buffer[100] = { 0 };

	if (help) {
		printf("i2cwrite <b> <s> <a> <l> <n> <b1> ... <bN> ");
		printf("Writes (dec)<n> bytes (hex)<bX> to address (hex)<a> (<l> address length) of slave (hex)<s> in I2C bus <b>\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &bus);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &slave);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &addr);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &addr_len);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &num_bytes);
		if (command_buffer == NULL)
			return -1;

		for (i = 0; i < num_bytes; i++) {
			command_buffer = parse_next_param(16, command_buffer, &aux);
			if (command_buffer == NULL)
				return -1;

			if (aux > 0xff) {
				printf("Invalid byte: 0x%lx\n", aux);
				return -1;
			}
			buffer[i] = aux;
		}

		switch (bus) {
		case 0:
			hi2c.base = I2C_0_BASE;
			break;
		default:
			printf("Bus not supported\n");
			return -1;
		}
		hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		hi2c.twi_clk = 40000;

		result = adi_twi_i2c_init(&hi2c);
		if (result < 0)
			return result;

		printf("Write slave 0x%lx address 0x%lx %ld bytes\n", slave, addr, num_bytes);
		result = adi_twi_i2c_write(&hi2c, slave, addr, addr_len, buffer, num_bytes);
		if (result < 0) {
			printf("Write error: %d\n", result);
			return result;
		}
	}
	return result;
}

static void c2c_train_dump_stats(void)
{
	int i, j, k;
	uint32_t *all_stats[2] = { p2s_stats, s2p_stats };
	uint32_t *stats;

	for (i = 0; i < 2; i++) {
		stats = all_stats[i];
		printf("\n%s statistics\n--------------\n", i ? "S2P" : "P2S");
		printf("RAW data:\n");
		for (j = 0; j < 8; j++) {
			for (k = 0; k < (ADI_C2C_TRIM_MAX / 8); k++)
				printf("%08x ", stats[j * 8 + k]);
			printf("\n");
		}
		printf("Per-lane and combined:\n");
		for (j = 0; j < ADI_C2C_LANE_COUNT; j++) {
			printf("Lane %d:   ", j);
			for (k = 0; k < ADI_C2C_TRIM_MAX; k++)
				printf("%d", ADI_C2C_GET_LANE_STAT(stats, k, j) != 0);
			printf("\n");
		}
		printf("Combined: ");
		for (k = 0; k < ADI_C2C_TRIM_MAX; k++)
			printf("%d", ADI_C2C_GET_COMBINED_STAT(stats, k) != 0);
		printf("\n");

		/* Dump only p2s stats */
		if (c2c_mode != C2C_MODE_NORMAL)
			break;
	}
}

static int c2c_setup_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t mode = 0;

	if (help) {
		printf("c2csetup [<mode>]                  ");
		printf("Configures C2C training\n");
		printf("                                   ");
		printf("<mode> values: 0=NORMAL, 1=EXTERNAL_LOOPBACK, 2=PHYDIG_LOOPBACK\n");
		printf("                                   ");
		printf("Sequence: c2csetup -> c2ctrain -> c2cactivate --(loopback mode)--> c2ctest \n");
	} else {
		/* Optional parameter to set c2c mode (default is 0) */
		command_buffer = parse_next_param(10, command_buffer, &mode);

		if ((mode < C2C_MODE_NORMAL) || (mode > C2C_MODE_PHYDIG_LOOPBACK)) {
			printf("Invalid mode selected\n");
			return -1;
		}

		if ((mode != C2C_MODE_NORMAL) && (plat_get_dual_tile_enabled())) {
			printf("Loopback mode is intended for single tile build\n");
			return -1;
		}

		if ((mode == C2C_MODE_NORMAL) && (!plat_get_dual_tile_enabled())) {
			printf("Normal mode is intended for dual tile build\n");
			return -1;
		}

		/* Store C2C mode */
		c2c_mode = mode;

		/* Set DEV_CLK */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);

		/* Perform MCS */
		if (!clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true))
			return -1;

		/* Init C2C (low speed) */
		adi_c2cc_init(C2CC_BASE, SEC_C2CC_BASE, c2c_mode);
		adi_c2cc_enable();

		/* Setup C2C training phase */
		if (!adi_c2cc_setup_train(adrv906x_c2cc_get_training_settings()))
			return -1;

		plat_secure_wdt_stop();

		c2c_train_state = C2C_STATE_CONFIGURED;
	}
	return 0;
}

static int c2c_train_command_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("c2ctrain                           ");
		printf("Performs C2C training (LFSR)\n");
	} else {
		/* c2csetup must be run first. If calibration was already applied, c2csetup must be re-run again */
		if ((c2c_train_state < C2C_STATE_CONFIGURED) || (c2c_train_state > C2C_STATE_TRAINED)) {
			printf("Please run c2csetup first\n");
			return -1;
		}

		/* Run calibration */
		if (!adi_c2cc_run_train(p2s_stats, s2p_stats))
			return -1;

		/* Dump training combined statistics */
		c2c_train_dump_stats();

		c2c_train_state = C2C_STATE_TRAINED;
	}
	return 0;
}

static int c2c_activate_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t min_eye_width = 5;
	struct adi_c2cc_training_settings *params = adrv906x_c2cc_get_training_settings();
	struct adi_c2cc_trim_settings forced_trim;
	struct adi_c2cc_trim_settings *trim = NULL;
	uint8_t all_data[2 * (ADI_C2C_LANE_COUNT + 1)]; /* 4 data lanes + 1 rx clock, per tile */
	uint64_t data;
	bool forced = true;
	int i;

	if (help) {
		printf("c2cactivate [<eye_width> [<p2s_rx> <p2s_l0> <p2s_l1> <p2s_l2> <p2s_l3> <s2p_rx> <s2p_l0> <s2p_l1> <s2p_l2> <s2p_l3>]]\n");
		printf("                                   ");
		printf("Analyzes training (unless it is bypassed by forcing trim values) and enables high speed mode.\n");
		printf("                                   ");
		printf("<eye width> values: odd number in the range 3-63 (default 5)\n");
		printf("                                   ");
		printf("<x2x_rx> values: 0 - 63\n");
		printf("                                   ");
		printf("<x2x_lx> values: 0 - 15\n");
		printf("                                   ");
		printf("Notes: p2s_rx is applied to secondary rx clk and s2p_rx is applied to primary rx clk\n");
		printf("                                   ");
		printf("       p2s_lx is applied to primary tx data and s2p_lx is applied to primary rx data\n");
		printf("                                   ");
		printf("       In loopback mode, s2p_xxx data is ignored and secondary becomes primary (ie: p2s_rx is applied to primary rx clk)\n");
		printf("                                   ");
		printf("       If x2x_xx data is provided, eye_width is ignored\n");
	} else {
		/* Optional parameter to set eye width (default is 5) */
		command_buffer = parse_next_param(10, command_buffer, &min_eye_width);
		if (command_buffer != NULL) {
			for (i = 0; i < (2 * (ADI_C2C_LANE_COUNT + 1)); i++) {
				command_buffer = parse_next_param(10, command_buffer, &data);
				if (command_buffer == NULL) {
					forced = false;
					break;
				}
				all_data[i] = data;
			}

			if (forced) {
				forced_trim.p2s_trim = all_data[0];
				forced_trim.p2s_trim_delays[0] = all_data[1];
				forced_trim.p2s_trim_delays[1] = all_data[2];
				forced_trim.p2s_trim_delays[2] = all_data[3];
				forced_trim.p2s_trim_delays[3] = all_data[4];
				forced_trim.s2p_trim = all_data[5];
				forced_trim.s2p_trim_delays[0] = all_data[6];
				forced_trim.s2p_trim_delays[1] = all_data[7];
				forced_trim.s2p_trim_delays[2] = all_data[8];
				forced_trim.s2p_trim_delays[3] = all_data[9];

				if ((forced_trim.p2s_trim > 63) || (forced_trim.s2p_trim > 63) ||
				    (forced_trim.p2s_trim_delays[0] > 15) || (forced_trim.p2s_trim_delays[1] > 15) ||
				    (forced_trim.p2s_trim_delays[2] > 15) || (forced_trim.p2s_trim_delays[3] > 15) ||
				    (forced_trim.s2p_trim_delays[0] > 15) || (forced_trim.s2p_trim_delays[1] > 15) ||
				    (forced_trim.s2p_trim_delays[2] > 15) || (forced_trim.s2p_trim_delays[3] > 15)) {
					printf("Invalid trim values\n");
					return -1;
				}

				/* use forced trim values */
				trim = &forced_trim;
			}
		}

		/* c2cstrain must be run first */
		if (c2c_train_state < C2C_STATE_TRAINED) {
			printf("Please run c2ctrain first\n");
			return -1;
		}

		if (((min_eye_width % 2) == 0) || (min_eye_width < 3))
			return -1;

		if (!adi_c2cc_process_train_data_and_apply(min_eye_width, trim, p2s_stats, s2p_stats, &(params->tx_clk)))
			return -1;

		c2c_train_state = C2C_STATE_CALIBRATED;
	}
	return 0;
}

static int c2c_test_command_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("c2ctest                            ");
		printf("Tests C2C in loopback mode (LFSR)\n");
	} else {
		/* c2ctest applies only to loopback mode */
		if (c2c_mode == C2C_MODE_NORMAL) {
			printf("c2ctest does not apply to c2c normal mode\n");
			return -1;
		}

		/* c2csactivate must be run first */
		if (c2c_train_state < C2C_STATE_CALIBRATED) {
			printf("Please run c2cactivate first\n");
			return -1;
		}

		if (!adi_c2cc_run_loopback_test())
			return -1;

		printf("Test complete (PASS)\n");
	}

	return 0;
}

cli_command_t plat_command_list[] = {
	{ "atetest",	   ate_test_command_function			  },
	{ "ateddr",	   ate_ddr_command_function			  },
	{ "bootrominotp",  bootrom_in_otp_command_function		  },
	{ "c2cactivate",   c2c_activate_command_function		  },
	{ "c2csetup",	   c2c_setup_command_function			  },
	{ "c2ctest",	   c2c_test_command_function			  },
	{ "c2ctrain",	   c2c_train_command_function			  },
	{ "ddrinit",	   ddr_init_command_function			  },
	{ "ddrextmemtest", ddr_extensive_mem_test_command_function	  },
	{ "ddrmemtest",	   ddr_mem_test_command_function		  },
	{ "ddrmux",	   ddr_debug_mux_output_command_function	  },
	{ "ddrpreinit",	   ddr_iterative_init_pre_reset_command_function  },
	{ "ddrpostinit",   ddr_iterative_init_post_reset_command_function },
	{ "ddrremapinit",  ddr_iterative_init_remapping_command_function  },
	{ "ddrtrain",	   ddr_custom_training_test_command_function	  },
	{ "ddrvalues",	   ddr_custom_values_command_function		  },
	{ "debugmuxshow",  xbar_show_command_function			  },
	{ "debugmuxout",   xbar_output_command_function			  },
	{ "debugmuxmap",   xbar_map_command_function			  },
	{ "i2cdetect",	   i2c_detect_command_function			  },
	{ "i2cread",	   i2c_read_command_function			  },
	{ "i2cwrite",	   i2c_write_command_function			  },
	{ "otpdump",	   otp_dump_command_function			  },
	{ "otpqrr",	   otp_qrr_command_function			  },
	{ "plltemp",	   pll_temp_sensors_read_function		  },
	{ "pllvcoout",	   pll_vco_output_command_function		  },
	{ "pllvtuneout",   pll_vtune_output_command_function		  },
	{ "programpll",	   program_pll_command_function			  },
	{ "sdread",	   sd_read_command_function			  },
	{ "end",	   plat_end_function				  }
};

static int adrv906x_dump_otp_memory(const uintptr_t start_addr, int size)
{
	uint32_t addr = start_addr;
	uint32_t readback;

	while ((addr <= LAST_SAMANA_OTP_ADDRESS) && (size > 0)) {
		if (otp_read(OTP_BASE, addr, &readback, OTP_ECC_ON) != ADI_OTP_SUCCESS) {
			printf("Failed to read from OTP memory at address %x\n", addr);
			return -EIO;
		} else {
			printf("0x%x: %x\n", addr, readback);
		}
		addr++;
		size--;
	}

	return ADI_OTP_SUCCESS;
}

static int adrv906x_program_bootrom_in_otp(const uintptr_t mem_ctrl_base, uintptr_t base_addr_image, int num_bytes)
{
	uint32_t addr = BOOTROM_IN_OTP_BASE_ADDR;
	uintptr_t image_addr = base_addr_image;
	int num_chunks = (num_bytes / (BOOTROM_DATA_BUFFER_SIZE * BYTES_PER_OTP_WRITE)) + ((num_bytes % (BOOTROM_DATA_BUFFER_SIZE * BYTES_PER_OTP_WRITE)) ? 1 : 0);
	uint32_t chunk_count = 1;
	uint32_t data[BOOTROM_DATA_BUFFER_SIZE];
	uint32_t readback[BOOTROM_DATA_BUFFER_SIZE];
	int length, i;

	while (num_bytes > 0) {
		length = 0;
		for (i = 0; i < BOOTROM_DATA_BUFFER_SIZE; i++) {
			/* If there are less than 3 bytes in the image left, we have to set remaining data in temp_data to 0 since OTP writes in 32 bit chunks */
			if (num_bytes > 3) {
				/* Read data from image in memory */
				data[i] = mmio_read_32(image_addr);
			} else {
				/* Read data, then 0 out anything that isn't actual image data so we don't write garbage to OTP */
				data[i] = mmio_read_32(image_addr);
				data[i] = data[i] & (BOOTROM_DATA_MASK >> ((BYTES_PER_OTP_WRITE - num_bytes) * 8));
			}

			/* Update the count and address for next read + length for write_burst */
			num_bytes -= BYTES_PER_OTP_WRITE;
			length++;
			image_addr += BYTES_PER_OTP_WRITE;

			/* Break out early if no more bytes in image to read */
			if (num_bytes <= 0)
				break;
		}

		/* Write the buffer to OTP */
		if (otp_write_burst(mem_ctrl_base, addr, data, length, OTP_ECC_ON) != ADI_OTP_SUCCESS) {
			plat_error_message("Error programming buffer into OTP at OTP address %x.", addr);
			return -EIO;
		}

		/* Read back what we just wrote and compare */
		if (otp_read_burst(mem_ctrl_base, addr, readback, length, OTP_ECC_ON) != ADI_OTP_SUCCESS) {
			plat_error_message("Error reading back OTP memory at OTP address %x.", addr);
			return -EIO;
		}

		for (i = 0; i < length; i++) {
			if (data[i] != readback[i]) {
				plat_error_message("Mismatch between written and readback data, aborting programming.");
				return -EIO;
			}
		}

		INFO("Bootrom Programming: Chunk %d of %d complete, programmed to OTP address %x.\n", chunk_count, num_chunks, addr);
		/* Move on to the next set of data */
		chunk_count++;
		addr += length;
	}

	/* Write the QRR bit to 1 */
	otp_write(mem_ctrl_base, OTP_QRR_BIT_ADDRESS, 0x1, OTP_ECC_ON);
	otp_read(mem_ctrl_base, OTP_QRR_BIT_ADDRESS, &readback[0], OTP_ECC_ON);
	if (readback[0] != 0x1) {
		plat_error_message("Failed to set QRR bit.");
		return -EIO;
	}

	return ADI_OTP_SUCCESS;
}
