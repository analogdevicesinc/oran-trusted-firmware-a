/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/fdt_wrappers.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <libfdt.h>
#include <lib/mmio.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include "adrv906x_dev_prof_addrs.h"
#include <platform_def.h>
#include <plat/common/platform.h>
#include <plat_device_profile.h>

static int fw_config_valid = 0;
static void *fw_config_dtb = 0;
static int bootcfg_valid = 0;
static void *bootcfg_dtb = 0;

/* Cached device profile values from FW_CONFIG */
static uint32_t dram_size = 0U;
static uint32_t dram_logical_size = 0U;
static uint32_t sec_dram_size = 0U;
static uint32_t sec_dram_logical_size = 0U;
static uint32_t sec_dram_remap_size = 0U;
static uint32_t clk_pll_freq_setting = 0U;
static uint32_t orx_adc_freq_setting = 0U;
static bool dual_tile_enabled = false;
static bool secondary_linux_enabled = false;
static bool ddr_primary_ecc_enabled = true;
static bool ddr_secondary_ecc_enabled = true;

static int get_fw_config_node(const char *node_name)
{
	int node = -1;

	if (fw_config_valid == 1) {
		node = fdt_path_offset(fw_config_dtb, node_name);
		if (node < 0)
			WARN("Unable to access node '%s' in FW_CONFIG\n", node_name);
	} else {
		WARN("Trying to access uninitialized or corrupt FW_CONFIG\n");
	}

	return node;
}

static int get_bootcfg_node(const char *node_name)
{
	int node = -1;

	if (bootcfg_valid == 1) {
		node = fdt_path_offset(bootcfg_dtb, node_name);
		if (node < 0)
			WARN("Unable to access node '%s' in bootcfg\n", node_name);
	} else {
		WARN("Trying to access uninitialized or corrupt bootcfg\n");
	}

	return node;
}

static int fw_config_prop_exists(const char *node_name, const char *param_name, bool *value)
{
	int node = -1;

	node = get_fw_config_node(node_name);
	if (node < 0)
		return node;

	const struct fdt_property *prop = fdt_get_property(fw_config_dtb, node, param_name, NULL);
	if (value != NULL)
		*value = (prop != NULL);

	return 0;
}

static int get_fw_config_uint32(const char *node_name, const char *param_name, uint32_t *value)
{
	int err = -1;
	int node = -1;

	node = get_fw_config_node(node_name);
	if (node < 0)
		return node;

	err = fdt_read_uint32(fw_config_dtb, node, param_name, value);
	if (err < 0)
		WARN("Unable to read param '%s' from node '%s' in FW_CONFIG\n", param_name, node_name);

	return err;
}

static int get_bootcfg_uint32(const char *node_name, const char *param_name, uint32_t *value)
{
	int err = -1;
	int node = -1;

	node = get_bootcfg_node(node_name);
	if (node < 0)
		return node;

	err = fdt_read_uint32(bootcfg_dtb, node, param_name, value);
	if (err < 0)
		WARN("Unable to read param '%s' from node '%s' in bootcfg\n", param_name, node_name);

	return err;
}

static int get_fw_config_string(const char *node_name, const char *param_name, const void **value)
{
	int err = -1;
	int node = -1;

	node = get_fw_config_node(node_name);
	if (node < 0)
		return node;

	*value = fdt_getprop(fw_config_dtb, node, param_name, &err);
	if (*value == NULL)
		WARN("Unable to read param '%s' from node '%s' in FW_CONFIG\n", param_name, node_name);
	else
		err = 0;

	return err;
}

static int set_fw_config_string(const char *node_name, const char *param_name, const void *value)
{
	int err = -1;
	int node = -1;

	fw_config_dtb = (void *)(uintptr_t)FW_CONFIG_BASE;

	err = fdt_open_into(fw_config_dtb, fw_config_dtb, FW_CONFIG_MAX_SIZE);
	if (err < 0) {
		WARN("Failed to open FW_CONFIG\n");
		return err;
	}
	fw_config_valid = 1;

	node = get_fw_config_node(node_name);
	if (node < 0)
		return node;

	err = fdt_setprop_string(fw_config_dtb, node, param_name, value);
	if (err < 0) {
		WARN("Failed to set param '%s' in node '%s' in FW_CONFIG\n", param_name, node_name);
		return err;
	}

	err = fdt_pack(fw_config_dtb);
	if (err < 0) {
		WARN("Failed to pack FW_CONFIG");
		return err;
	}

	clean_dcache_range((uintptr_t)fw_config_dtb, fdt_blob_size(fw_config_dtb));

	return err;
}

static void handle_fw_config_read_error(char *param_name, int err)
{
	ERROR("Unable to get %s from FW_CONFIG %d\n", param_name, err);
	plat_error_handler(err);
}

static void handle_fw_config_write_error(char *param_name, int err)
{
	ERROR("Unable to set %s in FW_CONFIG %d\n", param_name, err);
	plat_error_handler(err);
}

/* Returns the size of the DDR portioned out for the inline ECC data for a given DDR size */
static size_t plat_get_ddr_ecc_region_size(size_t size)
{
	return size >> 3;
}

/* Returns the maximum combined DDR size for Adrv906x */
static size_t plat_get_max_combined_ddr_size()
{
	return MAX_DDR_SIZE; /* 3 GB */
}

/* Check for common mistakes in the FW device tree before trying to calculate DDR sizes */
static void plat_check_ddr_fw_configs_values()
{
	/* Primary DDR size bigger than the max size even if secondary tile isn't present */
	if (dram_logical_size > MAX_DDR_SIZE) {
		ERROR("Primary DDR Size is too large, max is 3GB, check the ddr logical-size entry in the FW_CONFIG file.");
		plat_error_handler(-EINVAL);
	}

	/* Secondary DDR has a maximum supported size of 1.5GB for the logical size, physical size can be larger if excess memory is unused */
	if (sec_dram_logical_size > MAX_SECONDARY_DDR_SIZE) {
		ERROR("Secondary logical size value larger than max 1.5GB, check the ddr-secondary logical-size in the FW_CONFIG file.");
		plat_error_handler(-EINVAL);
	}

	if (sec_dram_remap_size > MAX_SECONDARY_DDR_SIZE) {
		ERROR("Secondary remapping value larger than max 1.5GB, check the ddr-secondary remap-size in the FW_CONFIG file.");
		plat_error_handler(-EINVAL);
	}
}

void plat_dprof_init(void)
{
	int err = -1;
	int bootcfg_clk_pll_result = -1;
	int bootcfg_orx_adc_result = -1;

	fw_config_dtb = (void *)(uintptr_t)FW_CONFIG_BASE;
	fw_config_valid = 0;

	bootcfg_dtb = (void *)(uintptr_t)BOOTCFG_BASE;
	bootcfg_valid = 0;

	err = fdt_check_header(fw_config_dtb);
	if (err < 0) {
		ERROR("Invalid FW_CONFIG detected %d\n", err);
		plat_error_handler(err);
	}

	fw_config_valid = 1;

	err = fdt_check_header(bootcfg_dtb);
	if (err < 0)
		INFO("Bootcfg is not present\n");
	else
		bootcfg_valid = 1;

	/* Get DRAM size */
	err = get_fw_config_uint32("/ddr", "physical-size", &dram_size);
	if (err != 0)
		handle_fw_config_read_error("DRAM size", err);

	err = get_fw_config_uint32("/ddr", "logical-size", &dram_logical_size);
	if (err != 0)
		handle_fw_config_read_error("DRAM logical size", err);

	err = fw_config_prop_exists("/ddr", "ecc", &ddr_primary_ecc_enabled);
	if (err != 0)
		handle_fw_config_read_error("DDR ECC enabled", err);

	if (bootcfg_valid == 1) {
		/* Get clock PLL freq setting from bootcfg */
		bootcfg_clk_pll_result = get_bootcfg_uint32("/clk-pll", "freq", &clk_pll_freq_setting);
		if (bootcfg_clk_pll_result != 0) {
			err = get_fw_config_uint32("/clk-pll", "freq", &clk_pll_freq_setting);
			if (err != 0)
				handle_fw_config_read_error("Clock PLL freq setting", err);
			else
				NOTICE("Using clk-pll frequency value %d from FW_CONFIG\n", clk_pll_freq_setting);
		}
		/* Get ORX ADC freq setting from bootcfg */
		bootcfg_orx_adc_result = get_bootcfg_uint32("/orx-adc", "freq", &orx_adc_freq_setting);
		if (bootcfg_orx_adc_result != 0) {
			err = get_fw_config_uint32("/orx-adc", "freq", &orx_adc_freq_setting);
			if (err != 0)
				handle_fw_config_read_error("ORX ADC freq setting", err);
			else
				NOTICE("Using orx-adc frequency value %d from FW_CONFIG\n", orx_adc_freq_setting);
		}
		/* Verify mcs config */
		if (clk_verify_config(clk_pll_freq_setting, orx_adc_freq_setting)) {
			/* Set clk-pll and orx-adc freq from bootcfg */
			if (bootcfg_clk_pll_result == 0)
				NOTICE("Using clk-pll frequency value %d from bootcfg\n", clk_pll_freq_setting);
			if (bootcfg_orx_adc_result == 0)
				NOTICE("Using orx-adc frequency value %d from bootcfg\n", orx_adc_freq_setting);
		} else {
			/* Set clk-pll and orx-adc freq from FW_CONFIG */
			ERROR("Invalid config of clk-pll freq %d and orx-adc freq %d, using freq values from FW_CONFIG\n", clk_pll_freq_setting, orx_adc_freq_setting);
			bootcfg_clk_pll_result = -1;
			bootcfg_orx_adc_result = -1;
		}
	}

	if (bootcfg_clk_pll_result != 0 && bootcfg_orx_adc_result != 0) {
		/* Get clock PLL freq setting from FW_CONFIG */
		err = get_fw_config_uint32("/clk-pll", "freq", &clk_pll_freq_setting);
		if (err != 0)
			handle_fw_config_read_error("Clock PLL freq setting", err);
		else
			NOTICE("Using clk-pll frequency value %d from FW_CONFIG\n", clk_pll_freq_setting);

		/* Get ORX ADC freq setting from FW_CONFIG */
		err = get_fw_config_uint32("/orx-adc", "freq", &orx_adc_freq_setting);
		if (err != 0)
			handle_fw_config_read_error("ORX ADC freq setting", err);
		else
			NOTICE("Using orx-adc frequency value %d from FW_CONFIG\n", orx_adc_freq_setting);
	}

	err = fw_config_prop_exists("/", "dual_tile", &dual_tile_enabled);
	if (err != 0)
		handle_fw_config_read_error("Dual tile configuration", err);

	err = fw_config_prop_exists("/", "secondary_linux_enabled", &secondary_linux_enabled);
	if (err != 0)
		handle_fw_config_read_error("Secondary linux configuration", err);

	if (dual_tile_enabled) {
		/* Get Secondary DRAM size */
		err = get_fw_config_uint32("/ddr-secondary", "physical-size", &sec_dram_size);
		if (err != 0)
			handle_fw_config_read_error("Secondary DRAM size", err);

		err = get_fw_config_uint32("/ddr-secondary", "logical-size", &sec_dram_logical_size);
		if (err != 0)
			handle_fw_config_read_error("Secondary DRAM logical size", err);

		err = get_fw_config_uint32("/ddr-secondary", "remap-size", &sec_dram_remap_size);
		if (err != 0)
			handle_fw_config_read_error("Secondary DRAM remap size", err);

		err = fw_config_prop_exists("/ddr-secondary", "ecc", &ddr_secondary_ecc_enabled);
		if (err != 0)
			handle_fw_config_read_error("Secondary DDR ECC enabled", err);
	}

	plat_check_ddr_fw_configs_values();
}

bool plat_get_dual_tile_enabled(void)
{
	return dual_tile_enabled;
}

void plat_set_dual_tile_disabled(void)
{
	int err = -1;
	int node = -1;
	bool success = false;

	/* Set the local cached copy to disabled */
	dual_tile_enabled = false;

	/* Then update the FW_CONFIG file so that downstream bootloaders (BL31)
	 * will pick up the change.
	 */
	err = fdt_open_into(fw_config_dtb, fw_config_dtb, FW_CONFIG_MAX_SIZE);
	if (!err) {
		node = get_fw_config_node("/");
		if (node >= 0) {
			err = fdt_delprop(fw_config_dtb, node, "dual_tile");
			if (!err) {
				err = fdt_pack(fw_config_dtb);
				if (!err) {
					clean_dcache_range((uintptr_t)fw_config_dtb, fdt_blob_size(fw_config_dtb));
					success = true;
				}
			}
		}
	}

	if (!success)
		ERROR("Failed to disable dual-tile mode\n");
}

bool plat_get_secondary_linux_enabled(void)
{
	return dual_tile_enabled && secondary_linux_enabled;
}

/* Returns the logical DRAM size that is used by U-Boot and Linux, which may or may not fill the actual physical size of the DDR.
 * The "logical" size is calculated by taking the smallest size between the physical size of the DDR - the memory reserved for ECC,
 * the specified logical size from the device tree, and the DDR remapping size again from the device tree. In the event that there
 * is no physical secondary DDR present, any memory carved out of the primary's physical DDR for the secondary is also subtracted.
 * The remaining size is whatever is left for the primary to utilize and is returned for memory allocation in the later bootloaders.
 */
size_t plat_get_dram_size()
{
	static bool warned = false;
	size_t sec_region_size;
	size_t ecc_size;
	size_t size;

	if (!plat_is_secondary_phys_dram_present())
		sec_region_size = plat_get_secondary_dram_size();
	else
		sec_region_size = 0;
	ecc_size = plat_get_ddr_ecc_region_size(plat_get_dram_physical_size());
	size = MIN((plat_get_dram_physical_size() - ecc_size), plat_get_primary_ddr_remap_window_size());
	size = MIN((size_t)dram_logical_size, size);
	if (size < sec_region_size) {
		ERROR("Calculated remaining DDR size is less than 0, check defines in device tree/ensure primary DDR size is large enough to handle region allocated for secondary tile.\n");
		plat_error_handler(-EINVAL);
	}
	size = size - sec_region_size;
	if (size < dram_logical_size) {
		/*Keep track of if we have already warned about the DDR size so we don't spam the logs*/
		if (!warned) {
			warned = true;
			NOTICE("Specified DDR size of 0x%x is too large, truncating to 0x%x.\n", (uint32_t)dram_logical_size, (uint32_t)size);
		}
	}

	return size;
}

/* Returns the actual physical size of the DDR on the primary tile */
size_t plat_get_dram_physical_size()
{
	return (size_t)dram_size;
}

/* Returns the logical DRAM size used by U-BOOT and Linux for the secondary tile, which can be less than the actual physical size.
 * The "logical" size is calculated by taking the smallest size between the physical size of the DDR - the memory reserved for ECC,
 * the specified logical size from the device tree, and the DDR remapping size from the device tree. In the event that there is no
 * physical DRAM present on the secondary tile, this function returns the specified logical size from the device tree.
 */
size_t plat_get_secondary_dram_size()
{
	static bool warned = false;
	size_t phys_size;
	size_t ecc_size;
	size_t size;

	if (plat_is_secondary_phys_dram_present()) {
		phys_size = plat_get_secondary_dram_physical_size();
		ecc_size = plat_get_ddr_ecc_region_size(phys_size);
		size = MIN(phys_size - ecc_size, plat_get_secondary_ddr_remap_window_size());
		size = MIN((size_t)sec_dram_logical_size, size);
		if (size < sec_dram_logical_size) {
			if (!warned) {
				warned = true;
				NOTICE("Specified secondary DDR size of 0x%x is too large, truncating to 0x%x.\n", (uint32_t)sec_dram_logical_size, (uint32_t)size);
			}
		}
		return size;
	} else {
		return (size_t)sec_dram_logical_size;
	}
}

/* Returns the physical size of the secondary's DDR as specified by the device tree.*/
size_t plat_get_secondary_dram_physical_size()
{
	return (size_t)sec_dram_size;
}

/* Return the base address of the secondary's DDR. If there is no physical secondary DRAM present, this function returns the base of
 * the DDR + the logical size of the primary DDR. If there is a secondary tile physical DDR present, then this returns the base + the
 * amount of the DDR address space mapped to the primary, giving the rest of it to the secondary. This is to handle cases where the DDR
 * size falls between two of the allowed remapping sizes, in which case it would be rounded up to the next closest size, and therefore
 * the secondary base should be based on this rounded size instead of the actual size. */
size_t plat_get_secondary_dram_base()
{
	if (!plat_is_secondary_phys_dram_present())
		return (size_t)(DRAM_BASE + plat_get_dram_size());
	else
		return (size_t)(DRAM_BASE + plat_get_primary_ddr_remap_window_size());
}

/* Returns the size of the primary's DDR remap window, which is portioned in 0.5 GB increments. All of the remap space is
 * given to either the primary or the secondary. If the secondary size is 1GB, then the NIC will give the remaining 2GB address space to the
 * primary, even Linux is unaware of the extra space or does not need it. */
size_t plat_get_primary_ddr_remap_window_size()
{
	return (size_t)plat_get_max_combined_ddr_size() - plat_get_secondary_ddr_remap_window_size();
}

/* Returns the size of the secondary's DDR remap window, which is portioned in 0.5 GB increments.*/
size_t plat_get_secondary_ddr_remap_window_size()
{
	if (plat_is_secondary_phys_dram_present())
		return (size_t)sec_dram_remap_size;
	else
		return 0U;
}

/* Returns true if there is a physical DRAM chip assigned to the secondary tile. A physical size of 0 specified
 * for the secondary tile is assumed to mean that a physical DDR will not be present for the secondary tile.*/
bool plat_is_secondary_phys_dram_present()
{
	return plat_get_secondary_dram_physical_size() != 0;
}

/* Checks to make sure the combined primary + secondary tile sizes are not bigger than the 3GB max for Adrv906x*/
bool plat_check_ddr_size(void)
{
	return (plat_get_dram_size() + plat_get_secondary_dram_size()) > MAX_DDR_SIZE;
}

/* Returns true if ECC on primary DDR is enabled */
bool plat_is_primary_ecc_enabled()
{
	return ddr_primary_ecc_enabled;
}

/* Returns true if ECC on secondary DDR is enabled*/
bool plat_is_secondary_ecc_enabled()
{
	return ddr_secondary_ecc_enabled;
}

/*
 * Returns the Clock PLL frequency setting
 * 0 for 7GHz, 1 for 11GHz
 */
uint32_t plat_get_clkpll_freq_setting(void)
{
	assert(clk_pll_freq_setting <= 1U);
	if (clk_pll_freq_setting > 1U)
		clk_pll_freq_setting = 0U;
	return (uint32_t)clk_pll_freq_setting;
}

/*
 * Returns the ORX ADC frequency setting
 * 0 - 3932 MHz ( 7GHz / 2)
 * 1 - 7864 MHz ( 7GHz / 1)
 * 2 - 5898 MHz (11GHz / 2)
 * 3 - 2949 MHz (11GHz / 4)
 */
uint32_t plat_get_orx_adc_freq_setting(void)
{
	assert(orx_adc_freq_setting <= 3U);
	if (orx_adc_freq_setting > 3U)
		orx_adc_freq_setting = 0U;
	return (uint32_t)orx_adc_freq_setting;
}

/*
 * Returns the current boot slot ID(e.g.'a')
 */
const char *plat_get_boot_slot(void)
{
	const void *data = NULL;
	int err = -1;

	err = get_fw_config_string("/boot-slot", "slot", &data);
	if (err != 0)
		handle_fw_config_read_error("boot slot", err);

	return (const char *)data;
}

/*
 * Sets the current boot slot ID
 */
void plat_set_boot_slot(const char *slot)
{
	int err = -1;

	err = set_fw_config_string("/boot-slot", "slot", slot);
	if (err != 0)
		handle_fw_config_write_error("boot slot", err);
}

unsigned int plat_get_syscnt_freq2(void)
{
	return clk_get_freq(CLK_CTL, CLK_ID_TIMER);
}

bool plat_is_sysc(void)
{
	uint8_t reg = mmio_read_8(PLATFORM_ID_REG);

	if (((reg & PLATFORM_ID_MASK) >> PLATFORM_ID_SHIFT) == 0x1U)
		return true;
	else
		return false;
}

bool plat_is_protium(void)
{
	uint8_t reg = mmio_read_8(PLATFORM_ID_REG);

	if (((reg & PLATFORM_ID_MASK) >> PLATFORM_ID_SHIFT) == 0x3U)
		return true;
	else
		return false;
}

bool plat_is_palladium(void)
{
	uint8_t reg = mmio_read_8(PLATFORM_ID_REG);

	if (((reg & PLATFORM_ID_MASK) >> PLATFORM_ID_SHIFT) == 0x2U)
		return true;
	else
		return false;
}
