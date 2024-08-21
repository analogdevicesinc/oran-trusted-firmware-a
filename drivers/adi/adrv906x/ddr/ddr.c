/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <arch/aarch64/arch_helpers.h>
#include <platform_def.h>
#include <common/debug.h>

#include <adrv906x_device_profile.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/ddr/ddr.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "ddr_config.h"
#include "ddr_phy_helpers.h"
#include "ddr_regmap.h"

#define DDR_SAR_REGISTER_INCREMENTS     0x10000000

/* Debug */
/*#define DDR_DEBUG_ENABLE*/
#ifdef DDR_DEBUG_ENABLE
#define DDR_DEBUG(...)          INFO(__VA_ARGS__)
#else
#define DDR_DEBUG(...)
#endif

/* This functions does a "scrub" of the ECC*, doing a test write to every memory location to update the ECC data and avoid read errors from uninitialized inline ECC bits
 *  Function taken from design team log to scrub the ECC after init before any writes to avoid early ECC errors on read */
static ddr_error_t ddr_scrub_ecc(uintptr_t base_addr_ctrl, uint32_t ddr_size, bool is_x16)
{
	uint32_t scrubber_data;
	uint32_t scrub_size;
	int i;

	/* We have to divide the final size by 2 if the DDR is an x16,
	 * after subtracting (size of inline ecc range + 1) from the range
	 * to avoid scrubbing the protected region of the inline ECC since we are scrubbing the full physical DDR always */
	if (is_x16)
		scrub_size = ((ddr_size - (ddr_size >> 3)) / 2) - 1;
	else
		/* x8 can just use size in bytes -1 to avoid protected region */
		scrub_size = (ddr_size - (ddr_size >> 3)) - 1;

	DDR_DEBUG("Beginning DDR scrub.\n");
	if (!plat_is_sysc()) {
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRWDATA0, 0x0000DEAF);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRSTART0, 0x0);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRRANGE0, scrub_size);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCTRL_0, 0x0);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCTRL_1, 0x0);
		scrubber_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRCTL);
		scrubber_data |= 0x5;
		scrubber_data &= 0xFFE000FF;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRCTL, scrubber_data);
		for (i = 0; i < ADI_DDR_ECC_SCRUB_TIMEOUT; i++) {
			if (((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRSTAT) & SBRSTAT_SCRUB_DONE_MASK) >> SBRSTAT_SCRUB_DONE_SHIFT) == 0x1)
				break;
			mdelay(1);
		}

		if (i == ADI_DDR_ECC_SCRUB_TIMEOUT)
			return ERROR_DDR_ECC_SCRUB_FAILED;


		for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
			if (((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRSTAT) & SBRSTAT_SCRUB_BUSY_MASK) >> SBRSTAT_SCRUB_BUSY_SHIFT) == 0x0)
				break;
			mdelay(1);
		}

		if (i == ADI_DDR_CTRL_TIMEOUT)
			return ERROR_DDR_ECC_SCRUB_FAILED;

		scrubber_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRCTL);
		scrubber_data &= 0xFFFFFFFE;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SBRCTL, scrubber_data);

		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCTRL_0, 0x400000FF);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCTRL_1, 0x400000FF);
	}

	DDR_DEBUG("DDR ECC scrub complete.\n");
	return ERROR_DDR_NO_ERROR;
}

/* Basic memory test for the DDR. Note: This test can only be run in BL2 */
ddr_error_t ddr_basic_mem_test(uintptr_t base_addr_ddr, uint32_t size, bool restore)
{
	uintptr_t addr = base_addr_ddr;
	uint32_t readback;
	uint32_t existing;
	uint32_t data = 1;

	flush_dcache_range(base_addr_ddr, (size - 1));
	disable_mmu_el1();
	while (addr < (base_addr_ddr + size)) {
		if (restore)
			/* Read existing to restore later */
			existing = mmio_read_64(addr);

		/* Write data, then read back written data */
		if ((addr % 0x10000) == 0x0)
			printf("Writing to address: %lx with data=%d\n", addr, data);
		mmio_write_64(addr, data);
		readback = mmio_read_64(addr);
		if (readback != data) {
			ERROR("Failed mem test, Addr: %lx\n", addr);
			enable_mmu_el1(0);
			inv_dcache_range(base_addr_ddr, size);
			return ERROR_DDR_BASIC_MEM_TEST_FAILED;
		}

		/* Restore original data */
		if (restore)
			mmio_write_64(addr, existing);
		data = addr % 1024;
		addr += 8;
	}
	INFO("Mem test: %d\n", ERROR_DDR_NO_ERROR);
	enable_mmu_el1(0);
	inv_dcache_range(base_addr_ddr, (size - 1));
	/* If we make it here we have successfully written and read back the same value for every memory location */
	return ERROR_DDR_NO_ERROR;
}

/* This test implements the MARCH-X algorithm for testing DDR memory, first writing 0 to all locations,
 * then traveling up and back down the memory, reading either 0 or 1 and writing the opposite right after the read
 * to the same location. This algorithm will detect any stuck at fault, address faults, and coupling faults between two different bytes. */
static ddr_error_t ddr_march_test(uintptr_t base_addr_ddr, uint32_t size)
{
	uintptr_t addr = base_addr_ddr;
	uintptr_t max_addr = (base_addr_ddr + size - 8);

	/* Clear cache and disable MMU so accesses actually go out to the physical DDR */
	flush_dcache_range(base_addr_ddr, (size - 1));
	disable_mmu_el1();

	/* Write 0 to all locations */
	while (addr < (base_addr_ddr + size)) {
		if ((addr % 0x1000000) == 0x0)
			printf("Writing to address: %lx with 0\n", addr);
		mmio_write_64(addr, 0x0);
		addr += 8;
	}

	addr = base_addr_ddr;
	/* Read 0, Write 1 ascending up the memory */
	while (addr < (base_addr_ddr + size)) {
		if (mmio_read_64(addr) != 0x0) {
			ERROR("Extensive mem march test failed writing 0, addr = %lx\n", addr);
			enable_mmu_el1(0);
			inv_dcache_range(base_addr_ddr, size);
			return ERROR_DDR_EXTENSIVE_MEM_TEST_FAILED;
		}
		mmio_write_64(addr, 0xFF);
		if ((addr % 0x1000000) == 0x0)
			printf("Writing to address: %lx with 0xFF\n", addr);
		addr += 8;
	}

	addr = max_addr;
	/* Read 1, Write 0 descending down the memory */
	while (addr >= base_addr_ddr) {
		if (mmio_read_64(addr) != 0xFF) {
			ERROR("Extensive mem march test failed writing 1, addr = %lx\n", addr);
			enable_mmu_el1(0);
			inv_dcache_range(base_addr_ddr, size);
			return ERROR_DDR_EXTENSIVE_MEM_TEST_FAILED;
		}
		mmio_write_64(addr, 0x0);
		if ((addr % 0x1000000) == 0x0)
			printf("Writing to address: %lx with 0x0\n", addr);
		addr -= 8;
	}

	addr = base_addr_ddr;
	/* Read 0 in every memory location */
	while (addr < (base_addr_ddr + size)) {
		if (mmio_read_64(addr) != 0x0) {
			ERROR("Extensive mem march test failed reading 0, addr = %lx\n", addr);
			enable_mmu_el1(0);
			inv_dcache_range(base_addr_ddr, size);
			return ERROR_DDR_EXTENSIVE_MEM_TEST_FAILED;
		}
		addr += 8;
	}

	enable_mmu_el1(0);
	inv_dcache_range(base_addr_ddr, (size - 1));
	return ERROR_DDR_NO_ERROR;
}

/* This memory tests "walks" a 1 through every bit in the memory, and checks if any other bits change. If
 * another bit changes, that means there is a coupling fault in the hardware within the word at the specified address.
 * This test does not check for coupling faults between words, just within a word */
static ddr_error_t ddr_walking_test(uintptr_t base_addr_ddr, uint32_t size)
{
	uintptr_t addr = base_addr_ddr;

	flush_dcache_range(base_addr_ddr, (size - 1));
	disable_mmu_el1();
	while (addr < (base_addr_ddr + size)) {
		if ((addr % 0x1000000) == 0x0)
			printf("Writing to address: %lx\n", addr);
		uint32_t data = 0x1;
		for (int i = 0; i < 32; i++) {
			mmio_write_64(addr, data);
			if (mmio_read_64(addr) != data) {
				ERROR("Extensive mem walking test failed, addr = %lx, bit offset = %d\n", addr, i);
				enable_mmu_el1(0);
				inv_dcache_range(base_addr_ddr, size);
				return ERROR_DDR_EXTENSIVE_MEM_TEST_FAILED;
			}
			data = data << 1;
		}
		mmio_write_64(addr, 0x0);
		addr += 8;
	}

	enable_mmu_el1(0);
	inv_dcache_range(base_addr_ddr, (size - 1));
	return ERROR_DDR_NO_ERROR;
}

/* This function runs the long version of the memory test on the DDR starting at base_addr_ddr and testing up to the size number of bytes.
 * It runs two tests, a general march test using the MARCH-X algorithm to check for address faults, stuck at faults, and some coupling faults between two different bytes, and
 * a walking 1s test to find coupling faults within a specific byte. This test will take a long time, so it is not recommended to run on Protium, SystemC
 * and actual Silicon are more manageable.
 */
ddr_error_t ddr_extensive_mem_test(uintptr_t base_addr_ddr, uint32_t size)
{
	ddr_error_t return_val;

	INFO("Starting DDR March test...\n");
	return_val = ddr_march_test(base_addr_ddr, size);
	INFO("DDR March Test: %d\n", return_val);
	if (return_val != ERROR_DDR_NO_ERROR)
		return return_val;

	INFO("Starting DDR Walking test...\n");
	return_val = ddr_walking_test(base_addr_ddr, size);

	INFO("DDR Walking Test: %d\n", return_val);
	if (return_val != ERROR_DDR_NO_ERROR)
		return return_val;

	return return_val;
}

/* Configures the SARBASE and SARSIZE registers for the DDR based on provided base address and size */
static ddr_error_t ddr_configure_remapping_registers(uintptr_t base_addr_ctrl, uintptr_t base_addr_ddr, uint32_t ddr_size)
{
	uint32_t sar_base;
	uint32_t sar_size;

	DDR_DEBUG("Configuring DDR Remapping registers\n");
	sar_base = base_addr_ddr / DDR_SAR_REGISTER_INCREMENTS; /* According to the design team, both the SARBASE and SARSIZE registers use increments of 256 MB */
	if (base_addr_ddr % DDR_SAR_REGISTER_INCREMENTS) {
		ERROR("DDR Base must be a multiple of 256 MB, 0x10000000\n");
		return ERROR_DDR_CTRL_INIT_FAILED;
	}
	sar_size = (ddr_size / DDR_SAR_REGISTER_INCREMENTS) - 1;
	/* Avoid underflow when ddr_size = 0 */
	if (ddr_size == 0U)
		sar_size = 0U;
	if (ddr_size % DDR_SAR_REGISTER_INCREMENTS) {
		ERROR("DDR Size must be a multiple of 256 MB, 0x10000000\n");
		return ERROR_DDR_CTRL_INIT_FAILED;
	}
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SARBASE0, sar_base);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_SARSIZE0, sar_size);
	DDR_DEBUG("DDR Remap register configuration complete\n");

	return ERROR_DDR_NO_ERROR;
}

/* Programs the DDR DFI Pad pinmux registers in the ddr_adi_interface module based on the sequence given in ddr_dfi_pad_sequence */
static void ddr_dfi_pad_pillar_remapping(uintptr_t base_addr_adi_interface, uint8_t ddr_dfi_pad_sequence[])
{
	/* DFI Interface changes */
	DDR_DEBUG("Performing DDR DFI PAD remapping.\n");
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS0, ddr_dfi_pad_sequence[0]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS1, ddr_dfi_pad_sequence[1]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS2, ddr_dfi_pad_sequence[2]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS3, ddr_dfi_pad_sequence[3]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS4, ddr_dfi_pad_sequence[4]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS5, ddr_dfi_pad_sequence[5]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS6, ddr_dfi_pad_sequence[6]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS7, ddr_dfi_pad_sequence[7]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS8, ddr_dfi_pad_sequence[8]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS9, ddr_dfi_pad_sequence[9]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS10, ddr_dfi_pad_sequence[10]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS11, ddr_dfi_pad_sequence[11]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS12, ddr_dfi_pad_sequence[12]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS13, ddr_dfi_pad_sequence[13]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS14, ddr_dfi_pad_sequence[14]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS15, ddr_dfi_pad_sequence[15]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS16, ddr_dfi_pad_sequence[16]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTADDRESS17, ddr_dfi_pad_sequence[17]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTCASN, ddr_dfi_pad_sequence[18]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTRASN, ddr_dfi_pad_sequence[19]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTWEN, ddr_dfi_pad_sequence[20]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTACTN, ddr_dfi_pad_sequence[21]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTBANK0, ddr_dfi_pad_sequence[22]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTBANK1, ddr_dfi_pad_sequence[23]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTBANK2, ddr_dfi_pad_sequence[24]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTBG0, ddr_dfi_pad_sequence[25]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTBG1, ddr_dfi_pad_sequence[26]);
	mmio_write_32(base_addr_adi_interface + DDR_CMD_ADDR_REMAP_DFI_SWZHWTPARITYIN, ddr_dfi_pad_sequence[27]);
}

/* Programs the DDR DFI Pad pinmux registers in the ddr_adi_interface module based on the sequence given in ddr_phy_pad_sequence */
static void ddr_phy_pad_pillar_remapping(uintptr_t base_addr_phy, uint8_t ddr_phy_pad_sequence[])
{
	/* PHY Engine HW Swizzle changes */
	DDR_DEBUG("Performing DDR PHY PAD remapping\n");
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS0, ddr_phy_pad_sequence[0]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS1, ddr_phy_pad_sequence[1]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS2, ddr_phy_pad_sequence[2]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS3, ddr_phy_pad_sequence[3]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS4, ddr_phy_pad_sequence[4]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS5, ddr_phy_pad_sequence[5]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS6, ddr_phy_pad_sequence[6]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS7, ddr_phy_pad_sequence[7]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS8, ddr_phy_pad_sequence[8]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS9, ddr_phy_pad_sequence[9]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS10, ddr_phy_pad_sequence[10]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS11, ddr_phy_pad_sequence[11]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS12, ddr_phy_pad_sequence[12]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS13, ddr_phy_pad_sequence[13]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS14, ddr_phy_pad_sequence[14]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS15, ddr_phy_pad_sequence[15]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTADDRESS17, ddr_phy_pad_sequence[17]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTCASN, ddr_phy_pad_sequence[18]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTRASN, ddr_phy_pad_sequence[19]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTWEN, ddr_phy_pad_sequence[20]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTACTN, ddr_phy_pad_sequence[21]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTBANK0, ddr_phy_pad_sequence[22]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTBANK1, ddr_phy_pad_sequence[23]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTBANK2, ddr_phy_pad_sequence[24]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTBG0, ddr_phy_pad_sequence[25]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTBG1, ddr_phy_pad_sequence[26]);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_HWTSWIZZLEHWTPARITYIN, ddr_phy_pad_sequence[27]);

	/* The PHY Engine DQ Swizzle changes cannot be turned on for PHY training, according to Synopsys. After testing it was determined
	 *  that the changes also were not needed for normal operating mode, so the changes were removed completely. */
}

/**
 *******************************************************************************
 * Function: ddr_init
 *
 * @brief      Initialize the DDR Controller
 *
 * @details    Initialize the DDR Controller
 *
 * Parameters:
 * @param [in]    None
 *
 * Reference to other related functions
 *
 * Notes: This currently calls the register read/writes from the test vector.
 * See the README in /tools/ddr for more information on regenerating test vector
 * files.
 *
 *******************************************************************************/
ddr_error_t ddr_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, uint32_t ddr_size, uint32_t ddr_remap_size, uint8_t ddr_dfi_pad_sequence[], uint8_t ddr_phy_pad_sequence[], ddr_init_stages_t stage, ddr_config_t configuration, bool ecc)
{
	ddr_error_t rtn_val = ERROR_DDR_NO_ERROR;
	bool is_x16;
	int i;

	if (configuration == DDR_PRIMARY_CONFIGURATION)
		is_x16 = DDR_PRIMARY_ECC_ISX16;
	else
		is_x16 = DDR_SECONDARY_ECC_ISX16;

	if ((stage == DDR_INIT_FULL) || (stage == DDR_CUSTOM_TRAINING) || (stage == DDR_PRE_RESET_INIT)) {
		clk_set_freq(base_addr_clk, CLK_ID_DDR, (ADI_DDR_FREQ_DEFAULT_MHZ * DDR_MHZ_TO_HZ));    /* Set dividers before enabling clock */
		clk_enable_clock(base_addr_clk, CLK_ID_DDR);                                            /* Enable all of the DDR clocks before running the init */

		/* This is for the master's DDR */
		/* Clear first CTL APB reset bit */
		DDR_DEBUG("Clearing DDR controller reset.\n");
		mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x2f);
		/* Need to wait at minimum 128 cycles of core DDR clock after clearing APB reset to allow clocks to sync, Synopsys recommends 1us for standard */
		udelay(1);

		/* Allow platforms to skip ddr dfi pad remapping if not needed */
		if (ddr_dfi_pad_sequence != NULL)
			ddr_dfi_pad_pillar_remapping(base_addr_adi_interface, ddr_dfi_pad_sequence);

		rtn_val = ddr_function_configurations[configuration].pre_reset_function(base_addr_ctrl, ecc);

		/* Registers that must be set if doing training to enable self-refresh */
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x20);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT0, 0x30020002);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
		for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
			if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWSTAT) == 0x00000001)
				break;
			else
				mdelay(1);
		}

		if (i == ADI_DDR_CTRL_TIMEOUT)
			return ERROR_DDR_CTRL_INIT_FAILED;
	}

	if ((stage == DDR_INIT_FULL) || (stage == DDR_CUSTOM_TRAINING) || (stage == DDR_REMAP_INIT)) {
		if (rtn_val == ERROR_DDR_NO_ERROR)
			rtn_val = ddr_configure_remapping_registers(base_addr_ctrl, base_addr_ddr, ddr_remap_size);

		if (rtn_val == ERROR_DDR_NO_ERROR) {
			DDR_DEBUG("Releasing DDR PHY reset.\n");
			/* Clean phy reset bits */
			mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x28);
			/* Need to wait at minimum 128 cycles of core DDR clock after clearing PHY reset to allow clocks to sync */
			udelay(1);
			/* Clean other two CTL reset bits and deassert the reset bits */
			mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x00);
			udelay(1);

			/* Allow platforms to skip ddr phy pad remapping if not needed */
			if (ddr_phy_pad_sequence != NULL)
				ddr_phy_pad_pillar_remapping(base_addr_phy, ddr_phy_pad_sequence);

			/* Synopsys documentation requires reset bit to remain low for at least 200 us before attempting the init sequence */
			udelay(200);
		}
	}

	if ((stage == DDR_INIT_FULL) || (stage == DDR_POST_RESET_INIT)) {
		if (rtn_val == ERROR_DDR_NO_ERROR) {
			rtn_val = ddr_post_reset_init(base_addr_ctrl, base_addr_phy, base_addr_adi_interface, base_addr_clk, stage, configuration);
			update_umctl2_timing_values(base_addr_ctrl, DDR_PSTATE0);
		}

		if (rtn_val == ERROR_DDR_NO_ERROR && ecc)
			rtn_val = ddr_scrub_ecc(base_addr_ctrl, ddr_size, is_x16);
	} else if (stage == DDR_CUSTOM_TRAINING) {
		rtn_val = ddr_post_reset_init(base_addr_ctrl, base_addr_phy, base_addr_adi_interface, base_addr_clk, stage, configuration);
	}

	return rtn_val;
}
