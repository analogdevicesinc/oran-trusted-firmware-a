/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <lib/mmio.h>

#include <adrv906x_ddr.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_nic_def.h>

#define ATE_FW_ADDR 0x00100000
#define ATE_FW_SIZE 0x7FFF
#define ATE_MSG_BLOCK_ADDR 0x0010D000
#define ATE_MSG_BLOCK_SIZE 0x3FFF

/* Sequence for programming the DDR pad pillar remapping registers in the ddr_adi_interface module, referred to in Yoda as the ddr_cmd_addr_remap, starting with ADDRESS0-16, then CASN, RASN, and WEN. */
static uint8_t ddr_dfi_pad_sequence[DDR_DFI_PAD_SEQUENCE_SIZE] = { 0xC, 0x3, 0x1, 0x8, 0x2, 0xA, 0xE, 0x4, 0xD, 0x19, 0x7, 0x1A, 0x6, 0x9, 0x1E, 0x1E, 0x1E, 0x0, 0x1B, 0x5, 0xB, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
static uint8_t ddr_phy_pad_sequence[DDR_PHY_PAD_SEQUENCE_SIZE] = { \
	0xC, 0x3,  0x1,	 0x8, \
	0x2, 0xA,  0xE,	 0x4, \
	0xD, 0x18, 0x7,	 0x19, \
	0x6, 0x9,  0x0,	 0x0, \
	0x0, 0x0,  0x1a, 0x5, \
	0xb, 0x0,  0x0,	 0x0, \
	0x0, 0x0,  0x0,	 0x0, \
	0x4, 0x6,  0x5,	 0x3, \
	0x0, 0x2,  0x7,	 0x1, \
	0x4, 0x6,  0x7,	 0x3, \
	0x0, 0x2,  0x1,	 0x5 \
};

/* This function programs the remapping register in the NIC to split the 3GB of available address space between the primary and secondary DDRs
 * Any address space not given to the primary is given to the secondary, and vice versa. So, if the primary size was 2GB, then the addresses
 * 0x4000_0000 through 0x9FFF_FFFF would get routed to the primary, and 0xA000_0000 and above would be routed to the secondary. */
static void plat_configure_nic_remap_register(size_t primary_remap_window_size)
{
	/* Only assign a split if the secondary DRAM is present, otherwise eveything needs to be routed to the primary. */
	if (plat_is_secondary_phys_dram_present()) {
		switch (primary_remap_window_size) {
		case DDR_SIZE_0_5GB:
			mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x00000070);
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x00000080);
			break;
		case DDR_SIZE_1GB:
			mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x00000030);
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x000000C0);
			break;
		case DDR_SIZE_1_5GB:
			mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x000000E0);
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x00000010);
			break;
		case DDR_SIZE_2GB:
			mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x000000C0);
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x00000030);
			break;
		case DDR_SIZE_2_5GB:
			mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x00000080);
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x00000070);
			break;
		case DDR_SIZE_3GB:
		default:
			mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x00000000);
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x000000F0);
			break;
		}
		;
	} else {
		mmio_write_32(FABRIC_REGMAP1_REMAP_ADDR, 0x00000000);
		if (plat_get_dual_tile_enabled())
			mmio_write_32(SEC_FABRIC_REGMAP1_REMAP_ADDR, 0x000000F0);
	}
}

int adrv906x_ddr_init(void)
{
	int err = 0;
	bool ecc;

	NOTICE("DDR Physical Size: 0x%lx\n", plat_get_dram_physical_size());
	NOTICE("DDR Logical Size: 0x%lx\n", plat_get_dram_size());
	NOTICE("DDR Primary Remapping Size: 0x%lx\n", plat_get_primary_ddr_remap_window_size());

	if (plat_get_dual_tile_enabled()) {
		NOTICE("DDR Secondary Physical Size: 0x%lx\n", plat_get_secondary_dram_physical_size());
		NOTICE("DDR Secondary Logical Size: 0x%lx\n", plat_get_secondary_dram_size());
		NOTICE("DDR Secondary Remapping Size: 0x%lx\n", plat_get_secondary_ddr_remap_window_size());
		NOTICE("DDR Secondary Base: 0x%lx\n", plat_get_secondary_dram_base());
	}

	plat_configure_nic_remap_register(plat_get_primary_ddr_remap_window_size());
	ecc = plat_is_primary_ecc_enabled();
	err = ddr_init(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, plat_get_dram_physical_size(), plat_get_primary_ddr_remap_window_size(), ddr_dfi_pad_sequence, ddr_phy_pad_sequence, DDR_INIT_FULL, DDR_PRIMARY_CONFIGURATION, ecc);
	if (err) {
		ERROR("Failed to initialize primary DDR %d\n", err);
		return err;
	}

	if (plat_get_dual_tile_enabled()) {
		if (plat_is_secondary_phys_dram_present()) {
			NOTICE("Initializing secondary DDR.\n");
			ecc = plat_is_secondary_ecc_enabled();
			err = ddr_init(SEC_DDR_CTL_BASE, SEC_DDR_PHY_BASE, SEC_DDR_ADI_INTERFACE_BASE, SEC_CLK_CTL, plat_get_secondary_dram_base(), plat_get_secondary_dram_physical_size(), plat_get_secondary_ddr_remap_window_size(), ddr_dfi_pad_sequence, ddr_phy_pad_sequence, DDR_INIT_FULL, DDR_SECONDARY_CONFIGURATION, ecc);
			if (err) {
				ERROR("Failed to initialize secondary DDR %d\n", err);
				plat_set_dual_tile_disabled();
				return err;
			}
		}
	}

	return err;
}

/* Runs the DDR ATE firmware */
int adrv906x_ddr_ate_test(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk)
{
	int err = 0;

	err = ddr_ate_test(base_addr_phy, base_addr_adi_interface, base_addr_clk, ATE_FW_ADDR, ATE_MSG_BLOCK_ADDR, ATE_FW_SIZE, ATE_MSG_BLOCK_SIZE);
	return err;
}

/* Runs the basic DDR mem test */
int adrv906x_ddr_mem_test(uintptr_t base_addr_ddr, uint32_t size, bool restore)
{
	int err = 0;

	err = ddr_basic_mem_test(base_addr_ddr, size, restore);
	return err;
}

/* Runs the extensive DDR mem test */
int adrv906x_ddr_extensive_mem_test(uintptr_t base_addr_ddr, uint32_t size)
{
	int err = 0;

	err = ddr_extensive_mem_test(base_addr_ddr, size);
	return err;
}

/* Runs a subset of the DDR training tests */
int adrv906x_ddr_custom_training_test(uintptr_t base_addr_phy, uint16_t sequence_ctrl, int train_2d)
{
	int err = 0;
	bool ecc;

	ecc = plat_is_primary_ecc_enabled();
	err = ddr_init(DDR_CTL_BASE, DDR_PHY_BASE, DDR_ADI_INTERFACE_BASE, CLK_CTL, DRAM_BASE, plat_get_dram_physical_size(), plat_get_primary_ddr_remap_window_size(), ddr_dfi_pad_sequence, ddr_phy_pad_sequence, DDR_CUSTOM_TRAINING, DDR_PRIMARY_CONFIGURATION, ecc);
	if (err)
		return err;

	/* Always set the verbosity of the training test messages to the max for custom testing */
	err = ddr_custom_training_test(base_addr_phy, DDR_HDTCTRL_MAX_VERBOSITY, sequence_ctrl, train_2d);
	return err;
}

/* Sets the source routed to the DDR's observation pin. Source numbers can be found in tables 9-3 - 9-5 in the PHY databook */
void adrv906x_ddr_mux_set_output(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint8_t group, uint8_t instance, uint8_t source)
{
	if (group > 2) {
		ERROR("Invalid group, expected value 0-2\n");
		return;
	}

	switch (group) {
	case DDR_MASTER0:
		if (instance != 0) {
			ERROR("Invalid instance. Acceptable values: 0");
			return;
		}
		break;
	case DDR_ANIB:
		if (instance > 11) {
			ERROR("Invalid instance. Acceptable values: 0-11");
			return;
		}
		break;
	case DDR_DBYTE:
		if (instance > 1) {
			ERROR("Invalid instance. Acceptable values: 0-1");
			return;
		}
		break;
	default:
		/* The invalid group should already be caught above, but just incase we'll check again */
		ERROR("Invalid group specified, try again\n");
		return;
	}

	ddr_mux_set_output(base_addr_phy, base_addr_adi_interface, base_addr_clk, group, instance, source);
	return;
}

/* Performs the pre reset init for the DDR init. Used for iterative debugging with the CLI */
int adrv906x_ddr_iterative_init_pre_reset(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, ddr_config_t configuration)
{
	int err = 0;
	bool ecc;

	if (base_addr_ctrl == DDR_CTL_BASE)
		ecc = plat_is_primary_ecc_enabled();
	else
		ecc = plat_is_secondary_ecc_enabled();

	ddr_init(base_addr_ctrl, base_addr_phy, base_addr_adi_interface, base_addr_clk, base_addr_ddr, plat_get_dram_physical_size(), plat_get_primary_ddr_remap_window_size(), ddr_dfi_pad_sequence, ddr_phy_pad_sequence, DDR_PRE_RESET_INIT, configuration, ecc);

	return err;
}

/* Performs the post reset init for the DDR init. Used for iterative debugging with the CLI */
int adrv906x_ddr_iterative_init_post_reset(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, ddr_config_t configuration)
{
	int err = 0;
	bool ecc;

	if (base_addr_ctrl == DDR_CTL_BASE)
		ecc = plat_is_primary_ecc_enabled();
	else
		ecc = plat_is_secondary_ecc_enabled();

	ddr_init(base_addr_ctrl, base_addr_phy, base_addr_adi_interface, base_addr_clk, base_addr_ddr, plat_get_dram_physical_size(), plat_get_primary_ddr_remap_window_size(), ddr_dfi_pad_sequence, ddr_phy_pad_sequence, DDR_POST_RESET_INIT, configuration, ecc);
	return err;
}

/* Performs the remapping and ECC configuration for the DDR init that occurs between the pre and post reset functions */
int adrv906x_ddr_iterative_init_remapping(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, ddr_config_t configuration)
{
	int err = 0;
	bool ecc;

	if (base_addr_ctrl == DDR_CTL_BASE)
		ecc = plat_is_primary_ecc_enabled();
	else
		ecc = plat_is_secondary_ecc_enabled();

	ddr_init(base_addr_ctrl, base_addr_phy, base_addr_adi_interface, base_addr_clk, base_addr_ddr, plat_get_dram_physical_size(), plat_get_primary_ddr_remap_window_size(), ddr_dfi_pad_sequence, ddr_phy_pad_sequence, DDR_REMAP_INIT, configuration, ecc);
	return err;
}
