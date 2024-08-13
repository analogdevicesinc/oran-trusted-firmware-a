/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <errno.h>

#include <lib/mmio.h>

#include <drivers/adi/adrv906x/clk.h>
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
#include <adrv906x_emac_common_def.h>
#include <adrv906x_mmap.h>
#include <adrv906x_otp.h>
#include <plat_boot.h>
#include <plat_cli.h>
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
static int adrv906x_dump_otp_memory(const uintptr_t start_addr, const int size);
static int adrv906x_program_bootrom_in_otp(const uintptr_t mem_ctrl_base, uintptr_t base_addr_image, int num_bytes);

static void plat_end_function(uint8_t *command_buffer, bool help)
{
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
		plat_secure_wdt_stop();
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
		plat_secure_wdt_stop();

		printf("Preparing DDR for ATE testing.\n");
		result = adrv906x_ddr_iterative_init_pre_reset(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred while running DDR pre init:%d\n", result);
		result = adrv906x_ddr_iterative_init_remapping(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, DDR_PRIMARY_CONFIGURATION);
		if (result)
			printf("Error occurred wile running DDR remapping init:%d\n", result);

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
		plat_secure_wdt_stop();
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
		plat_secure_wdt_stop();
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
		plat_secure_wdt_stop();

		/* Configure TZC */
		plat_security_setup();

		result = adrv906x_ddr_init();
		if (result) {
			printf("Error initializing the DDR: %d\n", result);
			return;
		}

		printf("Basic memory test invalidates the cache before running, reset is recommended before attempting any other operations after running.\n");
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
		plat_secure_wdt_stop();

		/* Configure TZC */
		plat_security_setup();

		result = adrv906x_ddr_init();
		if (result) {
			printf("Error initializing the DDR: %d\n", result);
			return;
		}

		printf("Extensive memory test invalidates the cache before running, reset is recommended before attempting any other operations after running.\n");
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

		/* Initialize the clocks and the TZC in case user runs custom training without a normal DDR init */
		clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
		clk_do_mcs(plat_get_dual_tile_enabled(), plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting(), true);
		plat_secure_wdt_stop();

		/* Configure TZC */
		plat_security_setup();

		result = adrv906x_ddr_custom_training_test(DDR_PHY_BASE, sequence_ctrl, train_2d);
		if (result)
			printf("Error occurred during DDR training firmware custom test:%d\n", result);
		else
			printf("DDR training firmware custom test passed.\n");
	}
}

/* Function to route the CLKPLL and Ethernet PLL VCO output to its test points */
static void pll_vco_output_command_function(uint8_t *command_buffer, bool help)
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
			return;
		command_buffer = parse_next_param(10, command_buffer, &xbar_pin);
		if (command_buffer == NULL && (pll == 2 || pll == 3)) {
			printf("Ethernet PLL VCO routing needs a second parameter.\n");
			return;
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
				return;
			}
			break;
		case 3:
			base = SEC_ETH_PLL_BASE;
			mmio_write_32(SEC_EMAC_COMMON_BASE + EMAC_COMMON_CONTROL_OUT_GPIO_SELECTION0, ETH_PLL_16_CLK_DIV16);
			if (!adi_adrv906x_debug_xbar_set_output(SEC_DEBUG_XBAR_SOURCE_CONTROL_BASE, DEBUG_XBAR_SRC_I_ETH_DBG_TO_GPIO_0, xbar_pin)) {
				printf("Error setting debug crossbar output.\n");
				return;
			}
			break;
		default:
			printf("Unsupported pll selected, try again.\n");
			return;
		}

		pll_configure_vco_test_out(base);
		printf("PLL VCO routing complete\n");
	}
}

/* Vtune out command. Will route the Vtune output of the CLKPLL or the ETH PLL to its corresponding test point*/
static void pll_vtune_output_command_function(uint8_t *command_buffer, bool help)
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
			return;

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
			return;
		}

		pll_configure_vtune_test_out(base);
		printf("PLL Vtune routing complete\n");
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

static void bootrom_in_otp_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t num_bytes;

	if (help) {
		printf("bootrominotp <num_bytes>           ");
		printf("Programs the OTP with the BootROM image loaded at 0x0010_0000 of length num_bytes.\n");
	} else {
		/* Add region for reading image to memory map */
		plat_setup_ns_sram_mmap();
		command_buffer = parse_next_param(16, command_buffer, &num_bytes);
		if (command_buffer == NULL)
			return;
		if (adrv906x_program_bootrom_in_otp(OTP_BASE, BOOTROM_SRAM_LOAD_ADDR, num_bytes))
			printf("Error programming BootROM into OTP.\n");
		else
			printf("BootROM programming in OTP successful.\n");
	}
	return;
}

static void otp_qrr_command_function(uint8_t *command_buffer, bool help)
{
	uint32_t qrr;

	if (help) {
		printf("otpqrr                             ");
		printf("Programs the QRR bit in the Samana OTP region\n");
	} else {
		if (otp_write(OTP_BASE, OTP_QRR_BIT_ADDRESS, 0x1, OTP_ECC_ON) != ADI_OTP_SUCCESS) {
			printf("Error programming the QRR bit in OTP.\n");
		} else {
			otp_read(OTP_BASE, OTP_QRR_BIT_ADDRESS, &qrr, OTP_ECC_ON);
			if (qrr != 0x1)
				printf("Error programming the QRR bit in OTP.\n");
			else
				printf("Programmed the QRR bit in OTP successfully.\n");
		}
	}
	return;
}

static void otp_dump_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t start_addr, size;

	if (help) {
		printf("otpdump <addr> <size>              ");
		printf("Dumps (dec)<size> OTP addresses starting from (hex) OTP address\n");
		printf("                                   ");
		printf("i.e. otpdump 0 4 will dump the first four OTP addresses.\n");
	} else {
		command_buffer = parse_next_param(16, command_buffer, &start_addr);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &size);
		if (command_buffer == NULL)
			return;

		if (adrv906x_dump_otp_memory(start_addr, size))
			printf("Failed to dump memory from OTP.\n");
	}
	return;
}

static void program_pll_command_function(uint8_t *command_buffer, bool help)
{
	uintptr_t base_addr, dig_core_base_addr;
	uint64_t pll, freq, pll_freq, ref_clk;
	bool secondary, eth_pll;
	PllSelName_e pll_sel_name;
	int err;

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
			return;
		command_buffer = parse_next_param(10, command_buffer, &freq);
		if (command_buffer == NULL)
			return;
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
			return;
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
			return;
		}

		err = clk_initialize_pll_programming(secondary, eth_pll, plat_get_clkpll_freq_setting(), plat_get_orx_adc_freq_setting());
		if (err) {
			ERROR("Failed preparing pll for programming.\n");
			return;
		}
		printf("Initializing LDO for pll...\n");
		err = ldo_powerup(base_addr, pll_sel_name);
		if (err) {
			ERROR("Failed to initialize LDO %d\n", err);
			return;
		}

		printf("Turning on PLL...\n");
		err = pll_clk_power_init(base_addr, dig_core_base_addr, pll_freq, ref_clk, pll_sel_name);
		if (err) {
			ERROR("Problem powering on pll = %d\n", err);
			return;
		}

		printf("Programming PLL...\n");
		err = pll_program(base_addr, pll_sel_name);
		if (err) {
			ERROR("Problem programming pll = %d\n", err);
			return;
		}
	}
	return;
}

static void sd_read_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t block_offset;
	uint64_t num_blocks;

	if (help) {
		printf("sdread <blk_offset> <num_blks>     ");
		printf("Reads num_blks blocks from offset blk_offset of SD card to SRAM addr 0x0010_0000.\n");
	} else {
		if (plat_get_boot_device() != PLAT_BOOT_DEVICE_SD_0) {
			printf("Error: Boot device must be set to SD_0\n");
			return;
		}

		plat_setup_ns_sram_mmap();
		plat_init_boot_device();

		command_buffer = parse_next_param(10, command_buffer, &block_offset);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &num_blocks);
		if (command_buffer == NULL)
			return;

		if (mmc_read_blocks(block_offset, (uintptr_t)0x00100000, num_blocks * 512) != (num_blocks * 512))
			printf("Error reading from SD card.\n");
		else
			printf("SD card read successful.\n");
	}
	return;
}

static void i2c_detect_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t bus;
	int ret;
	struct adi_i2c_handle hi2c;
	uint8_t i;
	uint8_t dummy;

	if (help) {
		printf("i2cdetect <bus>                    ");
		printf("Finds I2C slaves on the specified bus\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &bus);
		if (command_buffer == NULL)
			return;

		switch (bus) {
		case 0: hi2c.base = I2C_0_BASE; break;
		default: printf("Bus not supported\n"); return;
		}
		hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		hi2c.twi_clk = 40000;

		ret = adi_twi_i2c_init(&hi2c);
		if (ret < 0) return;

		printf("\n");
		printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
		for (i = 0; i < 0x7F; i++) {
			if (i % 0x10 == 0) printf("\n%02x: ", i);
			if (i < 0x03 || i > 0x77) {
				/* Reserved addresses */
				printf("   ");
				continue;
			}
			ret = adi_twi_i2c_read(&hi2c, i, 0, 1, &dummy, 1);
			if (ret < 0)
				printf("-- ");
			else
				printf("%02x ", i);
		}
		printf("\n");
	}
	return;
}

static void i2c_read_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t bus;
	uint64_t slave;
	uint64_t addr;
	uint64_t addr_len;
	uint64_t num_bytes;
	int ret;
	struct adi_i2c_handle hi2c;
	uint8_t i;
	uint8_t buffer[100] = { 0 };

	if (help) {
		printf("i2cread <b> <s> <a> <l> <n>        ");
		printf("Reads (dec)<n> bytes from address (hex)<a> (<l> address length) of slave (hex)<s> in I2C bus <b>\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &bus);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &slave);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &addr);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &addr_len);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &num_bytes);
		if (command_buffer == NULL)
			return;

		switch (bus) {
		case 0: hi2c.base = I2C_0_BASE; break;
		default: printf("Bus not supported\n"); return;
		}
		hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		hi2c.twi_clk = 40000;

		ret = adi_twi_i2c_init(&hi2c);
		if (ret < 0) return;

		printf("Read slave 0x%lx address 0x%lx %ld bytes\n", slave, addr, num_bytes);
		ret = adi_twi_i2c_read(&hi2c, slave, addr, addr_len, buffer, num_bytes);
		if (ret < 0) {
			printf("Read error: %d\n", ret);
			return;
		}

		for (i = 0; i < num_bytes; i++)
			printf("0x%02x ", buffer[i]);
		printf("\n");
	}
	return;
}

static void i2c_write_command_function(uint8_t *command_buffer, bool help)
{
	uint64_t bus;
	uint64_t slave;
	uint64_t addr;
	uint64_t addr_len;
	uint64_t aux;
	uint64_t num_bytes;
	int ret;
	struct adi_i2c_handle hi2c;
	uint8_t i;
	uint8_t buffer[100] = { 0 };

	if (help) {
		printf("i2cwrite <b> <s> <a> <l> <n> <b1> ... <bN> ");
		printf("Writes (dec)<n> bytes (hex)<bX> to address (hex)<a> (<l> address length) of slave (hex)<s> in I2C bus <b>\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &bus);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &slave);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(16, command_buffer, &addr);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &addr_len);
		if (command_buffer == NULL)
			return;
		command_buffer = parse_next_param(10, command_buffer, &num_bytes);
		if (command_buffer == NULL)
			return;
		for (i = 0; i < num_bytes; i++) {
			command_buffer = parse_next_param(16, command_buffer, &aux);
			if (command_buffer == NULL)
				return;
			if (aux > 0xff) {
				printf("Invalid byte: 0x%lx\n", aux);
				return;
			}
			buffer[i] = aux;
		}

		switch (bus) {
		case 0: hi2c.base = I2C_0_BASE; break;
		default: printf("Bus not supported\n"); return;
		}
		hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		hi2c.twi_clk = 40000;

		ret = adi_twi_i2c_init(&hi2c);
		if (ret < 0) return;

		printf("Write slave 0x%lx address 0x%lx %ld bytes\n", slave, addr, num_bytes);
		ret = adi_twi_i2c_write(&hi2c, slave, addr, addr_len, buffer, num_bytes);
		if (ret < 0) {
			printf("Write error: %d\n", ret);
			return;
		}
	}
	return;
}

cli_command_t plat_command_list[] = {
	{ "atetest",	   ate_test_command_function			  },
	{ "ateddr",	   ate_ddr_command_function			  },
	{ "bootrominotp",  bootrom_in_otp_command_function		  },
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
	{ "i2cdetect",	   i2c_detect_command_function			  },
	{ "i2cread",	   i2c_read_command_function			  },
	{ "i2cwrite",	   i2c_write_command_function			  },
	{ "otpdump",	   otp_dump_command_function			  },
	{ "otpqrr",	   otp_qrr_command_function			  },
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
			ERROR("Error programming buffer into OTP at OTP address %x.\n", addr);
			return -EIO;
		}

		/* Read back what we just wrote and compare */
		if (otp_read_burst(mem_ctrl_base, addr, readback, length, OTP_ECC_ON) != ADI_OTP_SUCCESS) {
			ERROR("Error reading back OTP memory at OTP address %x.\n", addr);
			return -EIO;
		}

		for (i = 0; i < length; i++) {
			if (data[i] != readback[i]) {
				ERROR("Mismatch between written and readback data, aborting programming.\n");
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
		ERROR("Failed to set QRR bit.\n");
		return -EIO;
	}

	return ADI_OTP_SUCCESS;
}
