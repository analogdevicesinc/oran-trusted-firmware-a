/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/ddr/ddr.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>


#include "ddr_phy_helpers.h"
#include "ddr_regmap.h"

#define DDR_MTESTMUXSEL_INCREMENTS              0x00004000
#define ATE_FW_FREQ 1600
#define SEQUENCECTRL_ADDR 0x15002C
#define HDTCRL_ADDR 0x150030
#define ONED_TRAINING_REQUIRED_TESTS_MASK 0x0001
#define TWOD_TRAINING_REQUIRED_TESTS_MASK 0x0001
#define ONED_TRAINING_RESERVED_TESTS_MASK 0xFCE0
#define TWOD_TRAINING_RESERVED_TESTS_MASK 0xFF9E

/* Clears the output selected for the DDR observation pin mux. This must also be called every time the mux output is changed. */
static void ddr_mux_clear_output(uintptr_t base_addr_phy)
{
	uintptr_t addr = base_addr_phy + DDRPHYA_ANIB0_P0_ANIB0_P0_MTESTMUXSEL;

	/* Clear the output to the DDR observation pin mux by writing a 0 */
	while (addr < (base_addr_phy + DDRPHYA_ACSM0_P0_ACSM0_P0_ACSMSEQ0X0)) {
		mmio_write_32(addr, 0x0);
		addr += DDR_MTESTMUXSEL_INCREMENTS;
	}
}

/* Sets the DDR mux for the selected source and instance to be routed to the DDR observation pin. */
void ddr_mux_set_output(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint8_t group, uint8_t instance, uint8_t source)
{
	uintptr_t addr = base_addr_phy;

	/* DDR clock should be enabled before this function is called in most cases, enabling here again as insurance */
	clk_set_freq(base_addr_clk, CLK_ID_DDR, (ADI_DDR_FREQ_DEFAULT_MHZ * DDR_MHZ_TO_HZ));
	clk_enable_clock(base_addr_clk, CLK_ID_DDR);

	/* Clean phy reset bits */
	mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x28);
	/* Need to wait at minimum 128 cycles of core DDR clock after clearing PHY reset to allow clocks to sync */
	udelay(1);

	/* These writes enable the digital output for the DDR according to section 9.4.1.1 of the PHY databook */
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_PORCONTROL, 0x1);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_TESTBUMPCNTRL, 0x7);

	/* Clear the current output before attempting to write a new one */
	ddr_mux_clear_output(base_addr_phy);
	/* Calculate addr of mux by adding the offset of the desired instance within the group */
	switch (group) {
	case DDR_MASTER0:
		addr = addr + DDRPHYA_MASTER0_P0_MASTER0_P0_MTESTMUXSEL + (instance * DDR_MTESTMUXSEL_INCREMENTS);
		break;
	case DDR_ANIB:
		addr = addr + DDRPHYA_ANIB0_P0_ANIB0_P0_MTESTMUXSEL + (instance * DDR_MTESTMUXSEL_INCREMENTS);
		break;
	case DDR_DBYTE:
		addr = addr + DDRPHYA_DBYTE0_P0_DBYTE0_P0_MTESTMUXSEL + (instance * DDR_MTESTMUXSEL_INCREMENTS);
		break;
	default:
		ERROR("Invalid group for DDR mux.\n");
		return;
	}

	mmio_write_32(addr, source);
	return;
}


/* Helper function to load the DDR ATE firmware to the PHY */
static void ddr_load_ate_image(uintptr_t base_addr_phy, uintptr_t ate_fw_addr, uintptr_t ate_msg_blk_addr, uintptr_t ate_fw_size, uintptr_t ate_msg_blk_size)
{
	uintptr_t dest_ptr = (base_addr_phy + DDR_PHY_IP_ICCM_INDEX);
	uint16_t *mem_ptr = (uint16_t *)ate_fw_addr;
	int i;

	/* Load the ATE firmware to the PHY */
	for (i = 0; i < (ate_fw_size >> 1); i++) {
		mmio_write_16(dest_ptr, *mem_ptr);
		dest_ptr += 4;
		mem_ptr++;
	}

	/* Load the ATE msg block to the PHY */
	dest_ptr = (base_addr_phy + DDR_PHY_IP_DCCM_INDEX);
	mem_ptr = (uint16_t *)ate_msg_blk_addr;
	for (i = 0; i < (ate_msg_blk_size >> 1); i++) {
		mmio_write_16(dest_ptr, *mem_ptr);
		dest_ptr += 4;
		mem_ptr++;
	}
}

/* This functions waits for the DDR ATE firmware to signal it is done. This is separated from the training firmware
 *  wait function to keep the functionality of the ATE firmware and the training firmware distinct since they
 *  use different protocols for signaling completion */
static int ddr_ate_wait_for_done(uintptr_t base_addr_phy)
{
	int i;

	for (i = 0; i < DDR_PHY_TRAINING_TIMEOUT_US; i++) {
		if ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) == 0x0)
			return ERROR_DDR_NO_ERROR;
		udelay(1);
	}

	return ERROR_DDR_PHY_FW_FAILED;
}

/* Runs the DDR ATE firmware and prints the values of the test seclection and test results area of the DDR message block */
ddr_error_t ddr_ate_test(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t ate_fw_addr, uintptr_t ate_msg_blk_addr, uint32_t ate_fw_size, uint32_t ate_msg_blk_size)
{
	ddr_error_t rtn_val = ERROR_DDR_NO_ERROR;
	uintptr_t result = base_addr_phy + DDR_PHY_IP_DCCM_INDEX;

	clk_set_freq(base_addr_clk, CLK_ID_DDR, (ADI_DDR_FREQ_DEFAULT_MHZ * DDR_MHZ_TO_HZ));
	clk_enable_clock(base_addr_clk, CLK_ID_DDR);

	/* Clear first CTL APB reset bit */
	mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x2f);
	/* Need to wait at minimum 128 cycles of core DDR clock after clearing APB reset to allow clocks to sync, Synopsys recommends 1us for standard */
	udelay(1);
	mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x28);
	/* Need to wait at minimum 128 cycles of core DDR clock after clearing PHY reset to allow clocks to sync */
	udelay(1);
	mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x00);
	udelay(1);

	/* Always run the ATE firmware at the highest frequency possible */
	NOTICE("Enabling DDR clocks...\n");
	rtn_val = phy_enable_power_and_clocks(base_addr_adi_interface, base_addr_clk, ATE_FW_FREQ);

	if (rtn_val == ERROR_DDR_NO_ERROR) {
		/* Load the firmware and msg block into the PHY */
		ddr_load_ate_image(base_addr_phy, ate_fw_addr, ate_msg_blk_addr, ate_fw_size, ate_msg_blk_size);

		/* Enable the PHY for testing and wait for done */
		NOTICE("Running ATE firmware...\n");
		phy_enable_micro_ctrl(base_addr_phy);
		rtn_val = ddr_ate_wait_for_done(base_addr_phy);
	}

	if (rtn_val == ERROR_DDR_NO_ERROR) {
		/* Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
		mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);
		/* Print out test results. */
		NOTICE("Test selection register(1=Test Run, 0=Test skipped): 0x%x\n", mmio_read_16(result));
		result += 4;
		NOTICE("Test results register(1=Pass, 0=Fail): 0x%x\n", mmio_read_16(result));
	}

	return rtn_val;
}

/* Runs the normal DDR training firmware with the selected subset of training tests.
 * This function assumes that the DDR has been initialized at least once, and thus that
 * the IMEM/DMEM have been loaded and are still in memory*/
ddr_error_t ddr_custom_training_test(uintptr_t base_addr_phy, uint8_t hdt_ctrl, uint16_t sequence_ctrl, int train_2d)
{
	ddr_error_t result;
	uint16_t tests = sequence_ctrl;

	if (train_2d) {
		/* 2D training requires some tests to always run, set them here */
		tests |= TWOD_TRAINING_REQUIRED_TESTS_MASK;
		/* 2D training requires some bits of SequenceCtrl to be 0, set them here */
		tests &= ~TWOD_TRAINING_RESERVED_TESTS_MASK;
	} else { /* Same thing if we are doing 1D training tests */
		tests |= ONED_TRAINING_REQUIRED_TESTS_MASK;
		tests &= ~ONED_TRAINING_RESERVED_TESTS_MASK;
	}

	/* Overwrite SequenceCtrl with new value */
	NOTICE("Setting sequence_ctrl to 0x%x.\n", tests);
	mmio_write_16(base_addr_phy + SEQUENCECTRL_ADDR, tests);

	/* Set verbosity of messages coming out of the DDR Phy */
	mmio_write_8(base_addr_phy + HDTCRL_ADDR, hdt_ctrl);

	NOTICE("Starting tests...\n");
	/* Enable uCtrl and wait for DONE message */
	phy_enable_micro_ctrl(base_addr_phy);
	result = phy_wait_for_done(base_addr_phy, train_2d);

	return result;
}
