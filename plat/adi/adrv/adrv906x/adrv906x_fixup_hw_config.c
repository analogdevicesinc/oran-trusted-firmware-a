/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <lib/libfdt/libfdt.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <platform_def.h>
#include <plat_boot.h>
#include <plat_err.h>
#include <plat_fixup_hw_config.h>

static int plat_set_prop_okay(void *hw_config_dtb, char *node_name)
{
	int node = -1;
	int err = -1;

	node = fdt_path_offset(hw_config_dtb, node_name);
	if (node < 0)
		return node;
	err = fdt_setprop_string(hw_config_dtb, node, "status", "okay");
	if (err != 0)
		return err;

	return 0;
}

static int plat_set_prop_disabled(void *hw_config_dtb, char *node_name)
{
	int node = -1;
	int err = -1;

	node = fdt_path_offset(hw_config_dtb, node_name);
	if (node < 0)
		return node;
	err = fdt_setprop_string(hw_config_dtb, node, "status", "disabled");
	if (err != 0)
		return err;

	return 0;
}

/* Enable SystemC-specific devices in U-Boot device tree */
/* TODO: Consider removing if/when SystemC support is removed */
static void plat_enable_sysc_devices(void *hw_config_dtb)
{
	char node_name[MAX_NODE_NAME_LENGTH];

	/* Disable non-SystemC eMMC and SD */
	snprintf(node_name, MAX_NODE_NAME_LENGTH, "/mmc@%08x", SD_0_BASE);
	plat_set_prop_disabled(hw_config_dtb, node_name);
	snprintf(node_name, MAX_NODE_NAME_LENGTH, "/mmc@%08x", EMMC_0_BASE);
	plat_set_prop_disabled(hw_config_dtb, node_name);
	snprintf(node_name, MAX_NODE_NAME_LENGTH, "/phy@%08x", EMMC_0_PHY_BASE);
	plat_set_prop_disabled(hw_config_dtb, node_name);

	/* Enable SystemC eMMC and SD */
	snprintf(node_name, MAX_NODE_NAME_LENGTH, "/mmc@%x", 1);
	plat_set_prop_okay(hw_config_dtb, node_name);
	snprintf(node_name, MAX_NODE_NAME_LENGTH, "/mmc@%x", 0);
	plat_set_prop_okay(hw_config_dtb, node_name);

	/* Disable non-SystemC 1G ethernet */
	snprintf(node_name, MAX_NODE_NAME_LENGTH, "/ethernet@%x", EMAC_1G_BASE);
	plat_set_prop_disabled(hw_config_dtb, node_name);
}

int plat_fixup_hw_config(void *hw_config_dtb)
{
	int num;
	int property_num = -1;
	int err = -1;
	int node = -1;
	int errors = -1;
	int reset_cause = -1;
	uint64_t clk_freq = 0ULL;
	char node_name[MAX_NODE_NAME_LENGTH];

	/* Get the sysclk freq */
	clk_freq = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);

	if (clk_freq != 0) {
		node = fdt_path_offset(hw_config_dtb, "/sysclk");
		if (node < 0)
			return node;

		err = fdt_setprop_u32(hw_config_dtb, node, "clock-frequency", (uint32_t)clk_freq);
		if (err < 0)
			return err;
	}

	/* Get the hsdigclk freq */
	clk_freq = clk_get_freq(CLK_CTL, CLK_ID_HSDIGCLK);

	if (clk_freq != 0) {
		node = fdt_path_offset(hw_config_dtb, "/hsdigclk");
		if (node < 0)
			return node;

		err = fdt_setprop_u32(hw_config_dtb, node, "clock-frequency", (uint32_t)clk_freq);
		if (err < 0)
			return err;
	}

	/* Update the SDHCI mmcclk freq and maximum base clock values based on HSDIG */
	clk_freq = clk_get_freq(CLK_CTL, CLK_ID_EMMC);

	if (clk_freq != 0) {
		node = fdt_path_offset(hw_config_dtb, "/mmcclk");
		if (node < 0)
			return node;

		err = fdt_setprop_u32(hw_config_dtb, node, "clock-frequency", (uint32_t)clk_freq);
		if (err < 0)
			return err;

		snprintf(node_name, MAX_NODE_NAME_LENGTH, "/mmc@%08x", EMMC_0_BASE);
		node = fdt_path_offset(hw_config_dtb, node_name);
		if (node < 0)
			return node;

		err = fdt_setprop_u32(hw_config_dtb, node, "max-frequency", (uint32_t)clk_freq);
		if (err < 0)
			return err;
	}

	/* Add dual-tile information, if applicable */
	if (plat_get_dual_tile_enabled()) {
		int parent = -1;
		uint32_t sec_dram_base;
		size_t sec_dram_size;

		/* Set the dual-tile flag in the boot params */
		parent = fdt_path_offset(hw_config_dtb, "/boot");
		if (parent < 0) {
			plat_error_message("Failed to find '/boot' node in HW_CONFIG %d", parent);
			return parent;
		}
		err = fdt_setprop_u32(hw_config_dtb, parent, "dual-tile", 1);
		if (err < 0) {
			plat_error_message("Failed to set 'dual-tile' property in HW_CONFIG %d", err);
			return err;
		}

		/* If secondary linux is enabled, set the secondary-linux-enabled flag */
		if (plat_get_secondary_linux_enabled()) {
			err = fdt_setprop_u32(hw_config_dtb, parent, "secondary-linux-enabled", 1);
			if (err < 0) {
				plat_error_message("Failed to set 'secondary-linux-enabled' property in HW_CONFIG %d", err);
				return err;
			}
		}

		/* Add secondary's DRAM information */

		sec_dram_base = plat_get_secondary_dram_base();
		sec_dram_size = plat_get_secondary_dram_size();

		snprintf(node_name, sizeof(node_name), "memory-secondary");
		node = fdt_add_subnode(hw_config_dtb, 0, node_name);

		err = fdt_setprop_string(hw_config_dtb, node, "device_type", "memory");
		if (err < 0)
			return err;

		err = fdt_setprop_u32(hw_config_dtb, node, "#address-cells", 1);
		if (err < 0)
			return err;

		err = fdt_setprop_u32(hw_config_dtb, node, "#size-cells", 1);
		if (err < 0)
			return err;

		/* Set 'secure-status' prop to 'disabled' so OP-TEE will ignore it */
		err = fdt_setprop_string(hw_config_dtb, node, "secure-status", "disabled");
		if (err < 0)
			return err;

		parent = fdt_path_offset(hw_config_dtb, "/");
		if (parent < 0) {
			plat_error_message("Failed to find root node in HW_CONFIG %d", parent);
			return parent;
		}

		err = fdt_appendprop_addrrange(hw_config_dtb, parent, node, "reg", sec_dram_base, sec_dram_size);
		if (err < 0) {
			plat_error_message("Failed to add 'reg' prop in HW_CONFIG %d", err);
			return err;
		}
	}

	/* Update platform entry for U-boot device tree */
	node = fdt_path_offset(hw_config_dtb, "/boot");
	if (node < 0)
		return node;

	if (plat_is_sysc() == true)
		err = fdt_setprop_string(hw_config_dtb, node, "plat", "sysc");
	else if (plat_is_protium() == true)
		err = fdt_setprop_string(hw_config_dtb, node, "plat", "protium");
	else if (plat_is_palladium() == true)
		err = fdt_setprop_string(hw_config_dtb, node, "plat", "palladium");
	else
		err = fdt_setprop_string(hw_config_dtb, node, "plat", "asic");

	if (err != 0)
		return err;

	/* TODO: Consider removing if/when SystemC support is removed */
	if (plat_is_sysc() == true)
		plat_enable_sysc_devices(hw_config_dtb);

	/* Copy reset cause from TF-A device tree to U-boot device tree */
	reset_cause = plat_get_fw_config_reset_cause();
	node = fdt_path_offset(hw_config_dtb, "/boot");
	if (node < 0)
		return node;
	err = fdt_setprop_u32(hw_config_dtb, node, "reset-cause", reset_cause);
	if (err < 0)
		return err;

	/* Copy TF-A device tree error logs to U-boot device tree */
	/* Get number of errors from TF-A device tree */
	errors = plat_get_fw_config_error_num();
	if (errors < 0) {
		INFO("Unable to get number of errors from FW_CONFIG\n");
	} else {
		/* Get boot node */
		node = fdt_path_offset(hw_config_dtb, "/boot");
		if (node < 0)
			return node;

		/* Add error-log node under boot node in U-boot device tree */
		snprintf(node_name, sizeof(node_name), "error-log");
		node = fdt_add_subnode(hw_config_dtb, node, node_name);
		if (node < 0) {
			INFO("Error adding /boot/error-log node to HW_CONFIG\n");
			return node;
		}

		/* Copy over each error-log property to U-boot device tree */
		for (num = 0; num <= errors; num++) {
			int length;
			char *name = "";
			const void *prop;

			/* Get property from fw_config */
			prop = get_fw_config_error_log_prop(&property_num, &name, &length);
			if (prop == NULL) {
				INFO("Unable to read error-log property from FW_CONFIG\n");
				continue;
			}

			/* Add property to hw_config */
			err = fdt_setprop(hw_config_dtb, node, name, prop, length);
			if (err < 0)
				INFO("Unable to copy property %s from FW_CONFIG to HW_CONFIG\n", name);
		}
	}

	return 0;
}
