/*
 * Copyright (c) 2024, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/fdt_wrappers.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <libfdt.h>
#include <lib/mmio.h>

#include <adrv906x_def.h>
#include <adrv906x_board.h>
#include <adrv906x_device_profile.h>
#include "adrv906x_dev_prof_addrs.h"
#include <adrv906x_otp.h>
#include <platform_def.h>
#include <plat/common/platform.h>
#include <plat_device_profile.h>
#include <plat_err.h>
#include <plat_trusted_boot.h>

#define FW_CONFIG_SECURE_LIST_MAX_SIZE          FW_CONFIG_PIN_NUM_MAX   /* pin list is larger than periph list */

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
static uint32_t eth_pll_freq_setting = 0U;
static bool dual_tile_enabled = false;
static bool secondary_linux_enabled = false;
static bool ddr_primary_ecc_enabled = true;
static bool ddr_secondary_ecc_enabled = true;
static bool dual_tile_no_c2c_primary = false;
static bool dual_tile_no_c2c_secondary = false;
static uint8_t eth_mac[ETH_LEN] = { 0 };
static const char zero_mac[ETH_LEN] = { 0 };
static uint8_t mac_otp[ETH_LEN] = { 0 };
static bool bootrom_bypass_enabled = false;
static bool secure_peripherals[FW_CONFIG_PERIPH_NUM_MAX];
static bool secure_pins[FW_CONFIG_PIN_NUM_MAX];

static int get_fw_config_node(const char *node_name, bool warn)
{
	int node = -1;

	if (fw_config_valid == 1) {
		node = fdt_path_offset(fw_config_dtb, node_name);
		if (node < 0) {
			if (warn)
				plat_warn_message("Unable to access node '%s' in FW_CONFIG", node_name);
			else
				INFO("Unable to access node '%s' in FW_CONFIG\n", node_name);
		}
	} else {
		if (warn)
			plat_warn_message("Trying to access uninitialized or corrupt FW_CONFIG");
		else
			INFO("Trying to access uninitialized or corrupt FW_CONFIG\n");
	}

	return node;
}

/* Get next fw config error-log property */
const void *get_fw_config_error_log_prop(int *property_offset, char **name, int *length)
{
	int node = -1;
	const char *label;
	const void *property;

	if (fw_config_valid == 1) {
		node = fdt_path_offset(fw_config_dtb, "/error-log");
		if (node < 0) {
			INFO("Unable to access node '%s' in FW_CONFIG\n", "/error-log");
		} else {
			/* Get error-log property */
			if (*property_offset < 0) {
				// Get first property
				*property_offset = fdt_first_property_offset(fw_config_dtb, node);
				property = fdt_getprop_by_offset(fw_config_dtb, *property_offset, &label, length);
				*name = (char *)label;
			} else {
				/* Get next property */
				*property_offset = fdt_next_property_offset(fw_config_dtb, *property_offset);
				property = fdt_getprop_by_offset(fw_config_dtb, *property_offset, &label, length);
				*name = (char *)label;
			}

			/* Return property */
			return property;
		}
	} else {
		INFO("Trying to access unitialized or corrupt FW_CONFIG\n");
	}

	return NULL;
}

static int get_bootcfg_node(const char *node_name)
{
	int node = -1;

	if (bootcfg_valid == 1) {
		node = fdt_path_offset(bootcfg_dtb, node_name);
		if (node < 0)
			INFO("Unable to access node '%s' in bootcfg\n", node_name);
	} else {
		INFO("Trying to access uninitialized or corrupt bootcfg\n");
	}

	return node;
}

static int fw_config_prop_exists(const char *node_name, const char *param_name, bool *value)
{
	int node = -1;

	node = get_fw_config_node(node_name, true);
	if (node < 0)
		return node;

	const struct fdt_property *prop = fdt_get_property(fw_config_dtb, node, param_name, NULL);
	if (value != NULL)
		*value = (prop != NULL);

	return 0;
}

static int get_fw_config_uint32(const char *node_name, const char *param_name, uint32_t *value, bool warn)
{
	int err = -1;
	int node = -1;

	node = get_fw_config_node(node_name, warn);
	if (node < 0)
		return node;

	err = fdt_read_uint32(fw_config_dtb, node, param_name, value);
	if (err < 0) {
		if (warn)
			plat_warn_message("Unable to read param '%s' from node '%s' in FW_CONFIG", param_name, node_name);
		else
			INFO("Unable to read param '%s' from node '%s' in FW_CONFIG\n", param_name, node_name);
	}

	return err;
}

static int set_fw_config_uint32(const char *node_name, const char *param_name, uint32_t value, bool warn)
{
	int err = -1;
	int node = -1;

	fw_config_dtb = (void *)(uintptr_t)FW_CONFIG_BASE;

	err = fdt_open_into(fw_config_dtb, fw_config_dtb, FW_CONFIG_MAX_SIZE);
	if (err < 0) {
		if (warn)
			plat_warn_message("Failed to open FW_CONFIG");
		else
			INFO("Failed to open FW_CONFIG\n");
		return err;
	}
	fw_config_valid = 1;

	node = get_fw_config_node(node_name, warn);
	if (node < 0)
		return node;

	err = fdt_setprop_u32(fw_config_dtb, node, param_name, value);
	if (err < 0) {
		if (warn)
			plat_warn_message("Failed to set param '%s' in node '%s' in FW_CONFIG", param_name, node_name);
		else
			INFO("Failed to set param '%s' in node '%s' in FW_CONFIG\n", param_name, node_name);
		return err;
	}

	err = fdt_pack(fw_config_dtb);
	if (err < 0) {
		if (warn)
			plat_warn_message("Failed to pack FW_CONFIG");
		else
			INFO("Failed to pack FW_CONFIG\n");
		return err;
	}

	clean_dcache_range((uintptr_t)fw_config_dtb, fdt_blob_size(fw_config_dtb));

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
		plat_warn_message("Unable to read param '%s' from node '%s' in bootcfg", param_name, node_name);

	return err;
}

static int get_bootcfg_mac(const char *node_name, const char *param_name, uint8_t *value)
{
	int err = -1;
	int node = -1;

	node = get_bootcfg_node(node_name);
	if (node < 0)
		return node;

	err = fdtw_read_bytes(bootcfg_dtb, node, param_name, ETH_LEN, value);
	if (err < 0)
		plat_warn_message("Unable to read param '%s' from node '%s' in bootcfg", param_name, node_name);

	return err;
}

static int get_fw_config_string(const char *node_name, const char *param_name, const void **value, bool warn)
{
	int err = -1;
	int node = -1;

	node = get_fw_config_node(node_name, warn);
	if (node < 0)
		return node;

	*value = fdt_getprop(fw_config_dtb, node, param_name, &err);
	if (*value == NULL) {
		if (warn)
			plat_warn_message("Unable to read param '%s' from node '%s' in FW_CONFIG", param_name, node_name);
		else
			INFO("Unable to read param '%s' from node '%s' in FW_CONFIG\n", param_name, node_name);
	} else {
		err = 0;
	}

	return err;
}

static int set_fw_config_string(const char *node_name, const char *param_name, const void *value, bool warn)
{
	int err = -1;
	int node = -1;

	fw_config_dtb = (void *)(uintptr_t)FW_CONFIG_BASE;

	err = fdt_open_into(fw_config_dtb, fw_config_dtb, FW_CONFIG_MAX_SIZE);
	if (err < 0) {
		if (warn)
			plat_warn_message("Failed to open FW_CONFIG");
		else
			INFO("Failed to open FW_CONFIG\n");
		return err;
	}
	fw_config_valid = 1;

	node = get_fw_config_node(node_name, warn);
	if (node < 0)
		return node;

	err = fdt_setprop_string(fw_config_dtb, node, param_name, value);
	if (err < 0) {
		if (warn)
			plat_warn_message("Failed to set param '%s' in node '%s' in FW_CONFIG", param_name, node_name);
		else
			INFO("Failed to set param '%s' in node '%s' in FW_CONFIG\n", param_name, node_name);
		return err;
	}

	err = fdt_pack(fw_config_dtb);
	if (err < 0) {
		if (warn)
			plat_warn_message("Failed to pack FW_CONFIG");
		else
			INFO("Failed to pack FW_CONFIG\n");
		return err;
	}

	clean_dcache_range((uintptr_t)fw_config_dtb, fdt_blob_size(fw_config_dtb));

	return err;
}

static void handle_fw_config_read_error(char *param_name, int err)
{
	plat_error_message("Unable to get %s from FW_CONFIG %d", param_name, err);
	plat_error_handler(err);
}

static void handle_fw_config_write_error(char *param_name, int err)
{
	plat_error_message("Unable to set %s in FW_CONFIG %d", param_name, err);
	plat_error_handler(err);
}

/* Returns the size of the DDR portioned out for the inline ECC data for a given DDR size */
static size_t plat_get_ddr_ecc_region_size(size_t size)
{
	return size >> 3;
}

/* Returns the maximum combined DDR size for Adrv906x */
static size_t plat_get_max_combined_ddr_size(void)
{
	return MAX_DDR_SIZE; /* 3 GB */
}

static int plat_get_secure_partitioning(void)
{
	int node;
	int ret;
	int len;
	int i;
	uint32_t sec_list[FW_CONFIG_SECURE_LIST_MAX_SIZE];
	char *periph_name[FW_CONFIG_PERIPH_NUM_MAX] = {
		"UART1", "UART3", "UART4",
		"SPI0",	 "SPI1",  "SPI2", "SPI3",  "SPI4", "SPI5",
		"I2C0",	 "I2C1",  "I2C2", "I2C3",  "I2C4", "I2C5","I2C6", "I2C7"
	};

	/* Default value: non-secure peripherals and pins */
	memset(secure_peripherals, 0, sizeof(secure_peripherals));
	memset(secure_pins, 0, sizeof(secure_pins));

	node = get_fw_config_node("/secure-partitioning", true);
	if (node < 0)
		/* No secure partitioning present in FW_COFNIG */
		return 0;

	/* Get secure peripheral list */
	if (NULL != fdt_getprop(fw_config_dtb, node, "peripherals", &len)) {
		/* Length in 32-bit unit */
		len = NCELLS(len);

		if ((unsigned int)len > FW_CONFIG_PERIPH_NUM_MAX) {
			plat_error_message("Secure peripherals list in FW_CONFIG is too large (%d > %d)", len, FW_CONFIG_PERIPH_NUM_MAX);
			return -1;
		}

		ret = fdt_read_uint32_array(fw_config_dtb, node, "peripherals", len, sec_list);
		if (ret == 0) {
			for (i = 0; i < len; i++) {
				uint32_t index = sec_list[i];

				if (index >= FW_CONFIG_PERIPH_NUM_MAX) {
					plat_error_message("FW_CONFIG: invalid peripheral id (%d)", index);
					return -1;
				} else {
					INFO("FW_CONFIG: peripheral %s is secure\n", periph_name[index]);
					secure_peripherals[index] = true;
				}
			}
		}
	}

	/* Get secure pins list */
	if (NULL != fdt_getprop(fw_config_dtb, node, "pins", &len)) {
		/* Length in 32-bit unit */
		len = NCELLS(len);

		if ((unsigned int)len > FW_CONFIG_PIN_NUM_MAX) {
			plat_error_message("Secure pins list in FW_CONFIG is too large (%d > %d)", len, FW_CONFIG_PIN_NUM_MAX);
			return -1;
		}

		ret = fdt_read_uint32_array(fw_config_dtb, node, "pins", len, sec_list);
		if (ret == 0) {
			for (i = 0; i < len; i++) {
				uint32_t index = sec_list[i];

				if (index >= FW_CONFIG_PIN_NUM_MAX) {
					plat_error_message("FW_CONFIG: invalid pin id (%d)", index);
					return -1;
				} else {
					INFO("FW_CONFIG: pin %d is secure\n", index);
					secure_pins[index] = true;
				}
			}
		}
	}

	return 0;
}

/* Check for common mistakes in the FW device tree before trying to calculate DDR sizes */
static void plat_check_ddr_fw_configs_values(void)
{
	/* Primary DDR size bigger than the max size even if secondary tile isn't present */
	if (dram_logical_size > MAX_DDR_SIZE) {
		plat_error_message("Primary DDR Size is too large, max is 3GB, check the ddr logical-size entry in the FW_CONFIG file.");
		plat_error_handler(-EINVAL);
	}

	/* Secondary DDR has a maximum supported size of 1.5GB for the logical size, physical size can be larger if excess memory is unused */
	if (sec_dram_logical_size > MAX_SECONDARY_DDR_SIZE) {
		plat_error_message("Secondary logical size value larger than max 1.5GB, check the ddr-secondary logical-size in the FW_CONFIG file.");
		plat_error_handler(-EINVAL);
	}

	if (sec_dram_remap_size > MAX_SECONDARY_DDR_SIZE) {
		plat_error_message("Secondary remapping value larger than max 1.5GB, check the ddr-secondary remap-size in the FW_CONFIG file.");
		plat_error_handler(-EINVAL);
	}
}

static int get_mac_from_otp(uint32_t index, uint8_t **mac)
{
	int err = -1;

	if (index >= MAX_NUM_MACS)
		return -1;
	if (plat_is_bootrom_bypass_enabled()) {
		/* Calling function expects return to be 0 unless OTP read fails, so we need to return a 0, but set the mac address to 0 to get the intended result of no MAC address from OTP */
		*mac = (uint8_t *)zero_mac;
		return 0;
	} else {
		err = adrv906x_otp_get_mac_addr(OTP_BASE, index + 1, mac_otp);

		if (err == 0)
			*mac = mac_otp;

		return err;
	}
}

static int get_mac_from_bootcfg(int index, uint8_t **mac)
{
	int err = -1;
	char mac_name[MAX_NODE_NAME_LENGTH];

	if (index >= MAX_NUM_MACS)
		return -1;

	if (bootcfg_valid == 1) {
		snprintf(mac_name, sizeof(mac_name), "/emac%d", index + 1);
		err = get_bootcfg_mac(mac_name, "mac-address", eth_mac);
	}

	/* MAC in bootcfg is optional. Return zero mac if it is not found */
	if (err == 0)
		*mac = eth_mac;
	else
		*mac = (uint8_t *)zero_mac;

	return 0;
}

static int is_zero_mac(uint8_t *mac)
{
	return (mac[0] == 0) && (mac[1] == 0) && (mac[2] == 0) && \
	       (mac[3] == 0) && (mac[4] == 0) && (mac[5] == 0);
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
		plat_error_message("Invalid FW_CONFIG detected %d", err);
		plat_error_handler(err);
	}
	fw_config_valid = 1;

	err = fdt_check_header(bootcfg_dtb);
	if (err < 0)
		INFO("Bootcfg is not present\n");
	else
		bootcfg_valid = 1;

	/* Get DRAM size */
	err = get_fw_config_uint32("/ddr", "physical-size", &dram_size, true);
	if (err != 0)
		handle_fw_config_read_error("DRAM size", err);

	err = get_fw_config_uint32("/ddr", "logical-size", &dram_logical_size, true);
	if (err != 0)
		handle_fw_config_read_error("DRAM logical size", err);

	err = fw_config_prop_exists("/ddr", "ecc", &ddr_primary_ecc_enabled);
	if (err != 0)
		handle_fw_config_read_error("DDR ECC enabled", err);

	if (bootcfg_valid == 1) {
		/* Get clock PLL freq setting from bootcfg */
		bootcfg_clk_pll_result = get_bootcfg_uint32("/clk-pll", "freq", &clk_pll_freq_setting);
		if (bootcfg_clk_pll_result != 0) {
			err = get_fw_config_uint32("/clk-pll", "freq", &clk_pll_freq_setting, true);
			if (err != 0)
				handle_fw_config_read_error("Clock PLL freq setting", err);
			else
				NOTICE("Using clk-pll frequency value %d from FW_CONFIG\n", clk_pll_freq_setting);
		}
		/* Get ORX ADC freq setting from bootcfg */
		bootcfg_orx_adc_result = get_bootcfg_uint32("/orx-adc", "freq", &orx_adc_freq_setting);
		if (bootcfg_orx_adc_result != 0) {
			err = get_fw_config_uint32("/orx-adc", "freq", &orx_adc_freq_setting, true);
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
			plat_error_message("Invalid config of clk-pll freq %d and orx-adc freq %d, using freq values from FW_CONFIG", clk_pll_freq_setting, orx_adc_freq_setting);
			bootcfg_clk_pll_result = -1;
			bootcfg_orx_adc_result = -1;
		}
	}

	if (bootcfg_clk_pll_result != 0 && bootcfg_orx_adc_result != 0) {
		/* Get clock PLL freq setting from FW_CONFIG */
		err = get_fw_config_uint32("/clk-pll", "freq", &clk_pll_freq_setting, true);
		if (err != 0)
			handle_fw_config_read_error("Clock PLL freq setting", err);
		else
			NOTICE("Using clk-pll frequency value %d from FW_CONFIG\n", clk_pll_freq_setting);

		/* Get ORX ADC freq setting from FW_CONFIG */
		err = get_fw_config_uint32("/orx-adc", "freq", &orx_adc_freq_setting, true);
		if (err != 0)
			handle_fw_config_read_error("ORX ADC freq setting", err);
		else
			NOTICE("Using orx-adc frequency value %d from FW_CONFIG\n", orx_adc_freq_setting);
	}

	err = get_fw_config_uint32("/eth-pll", "freq", &eth_pll_freq_setting, true);
	if (err != 0)
		handle_fw_config_read_error("Ethernet PLL freq setting", err);
	else
		NOTICE("Using eth-pll frequency value %d from FW_CONFIG\n", eth_pll_freq_setting);

	err = fw_config_prop_exists("/", "dual-tile", &dual_tile_enabled);
	if (err != 0)
		handle_fw_config_read_error("Dual tile configuration", err);

	err = fw_config_prop_exists("/", "secondary-linux-enabled", &secondary_linux_enabled);
	if (err != 0)
		handle_fw_config_read_error("Secondary linux configuration", err);

	err = fw_config_prop_exists("/", "bootrom_bypass", &bootrom_bypass_enabled);
	if (err != 0)
		handle_fw_config_read_error("Bootrom bypass configuration", err);

	if (dual_tile_enabled) {
		/* Get Secondary DRAM size */
		err = get_fw_config_uint32("/ddr-secondary", "physical-size", &sec_dram_size, true);
		if (err != 0)
			handle_fw_config_read_error("Secondary DRAM size", err);

		err = get_fw_config_uint32("/ddr-secondary", "logical-size", &sec_dram_logical_size, true);
		if (err != 0)
			handle_fw_config_read_error("Secondary DRAM logical size", err);

		err = get_fw_config_uint32("/ddr-secondary", "remap-size", &sec_dram_remap_size, true);
		if (err != 0)
			handle_fw_config_read_error("Secondary DRAM remap size", err);

		err = fw_config_prop_exists("/ddr-secondary", "ecc", &ddr_secondary_ecc_enabled);
		if (err != 0)
			handle_fw_config_read_error("Secondary DDR ECC enabled", err);
	}

	err = fw_config_prop_exists("/", "dual-tile-no-c2c-primary", &dual_tile_no_c2c_primary);
	if (err != 0)
		handle_fw_config_read_error("Dual no C2C Primary configuration", err);
	err = fw_config_prop_exists("/", "dual-tile-no-c2c-secondary", &dual_tile_no_c2c_secondary);
	if (err != 0)
		handle_fw_config_read_error("Dual no C2C Secondary configuration", err);

	if (dual_tile_enabled && (dual_tile_no_c2c_primary || dual_tile_no_c2c_secondary)) {
		plat_error_message("Invalid configuration: dual-tile and dual-no-c2c cannot be enabled simultaneously");
		plat_error_handler(-EINVAL);
	}

	plat_check_ddr_fw_configs_values();

	err = plat_get_secure_partitioning();
	if (err != 0)
		handle_fw_config_read_error("Secure partitioning configuration", err);
}

bool plat_get_dual_tile_no_c2c_enabled(void)
{
	return dual_tile_no_c2c_primary || dual_tile_no_c2c_secondary;
}

bool plat_get_dual_tile_no_c2c_primary(void)
{
	return dual_tile_no_c2c_primary;
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
		node = get_fw_config_node("/", true);
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
		plat_error_message("Failed to disable dual-tile mode");
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
size_t plat_get_dram_size(void)
{
	static bool warned = false;
	size_t sec_region_size;
	size_t ecc_size;
	size_t size;

	if (!plat_is_secondary_phys_dram_present())
		sec_region_size = plat_get_secondary_dram_size();
	else
		sec_region_size = 0;

	if (plat_is_primary_ecc_enabled())
		ecc_size = plat_get_ddr_ecc_region_size(plat_get_dram_physical_size());
	else
		ecc_size = 0;

	size = MIN((plat_get_dram_physical_size() - ecc_size), plat_get_primary_ddr_remap_window_size());
	size = MIN((size_t)dram_logical_size, size);
	if (size < sec_region_size) {
		plat_error_message("Calculated remaining DDR size is less than 0, check defines in device tree/ensure primary DDR size is large enough to handle region allocated for secondary tile.");
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
size_t plat_get_dram_physical_size(void)
{
	if (plat_is_sysc())
		if (dram_size > 0x20000000)
			return 0x20000000;
	return (size_t)dram_size;
}

/* Returns the logical DRAM size used by U-BOOT and Linux for the secondary tile, which can be less than the actual physical size.
 * The "logical" size is calculated by taking the smallest size between the physical size of the DDR - the memory reserved for ECC,
 * the specified logical size from the device tree, and the DDR remapping size from the device tree. In the event that there is no
 * physical DRAM present on the secondary tile, this function returns the specified logical size from the device tree.
 */
size_t plat_get_secondary_dram_size(void)
{
	static bool warned = false;
	size_t phys_size;
	size_t ecc_size;
	size_t size;

	if (plat_is_secondary_phys_dram_present()) {
		phys_size = plat_get_secondary_dram_physical_size();
		if (plat_is_secondary_ecc_enabled())
			ecc_size = plat_get_ddr_ecc_region_size(plat_get_secondary_dram_physical_size());
		else
			ecc_size = 0;
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
size_t plat_get_secondary_dram_physical_size(void)
{
	if (plat_is_sysc())
		if (sec_dram_size > 0x20000000)
			return 0x20000000;
	return (size_t)sec_dram_size;
}

/* Return the base address of the secondary's DDR. If there is no physical secondary DRAM present, this function returns the base of
 * the DDR + the logical size of the primary DDR. If there is a secondary tile physical DDR present, then this returns the base + the
 * amount of the DDR address space mapped to the primary, giving the rest of it to the secondary. This is to handle cases where the DDR
 * size falls between two of the allowed remapping sizes, in which case it would be rounded up to the next closest size, and therefore
 * the secondary base should be based on this rounded size instead of the actual size. */
size_t plat_get_secondary_dram_base(void)
{
	if (!plat_is_secondary_phys_dram_present())
		return (size_t)(DRAM_BASE + plat_get_dram_size());
	else
		return (size_t)(DRAM_BASE + plat_get_primary_ddr_remap_window_size());
}

/* Returns the size of the primary's DDR remap window, which is portioned in 0.5 GB increments. All of the remap space is
 * given to either the primary or the secondary. If the secondary size is 1GB, then the NIC will give the remaining 2GB address space to the
 * primary, even Linux is unaware of the extra space or does not need it. */
size_t plat_get_primary_ddr_remap_window_size(void)
{
	return (size_t)plat_get_max_combined_ddr_size() - plat_get_secondary_ddr_remap_window_size();
}

/* Returns the size of the secondary's DDR remap window, which is portioned in 0.5 GB increments.*/
size_t plat_get_secondary_ddr_remap_window_size(void)
{
	if (plat_is_secondary_phys_dram_present())
		return (size_t)sec_dram_remap_size;
	else
		return 0U;
}

/* Returns true if there is a physical DRAM chip assigned to the secondary tile. A physical size of 0 specified
 * for the secondary tile is assumed to mean that a physical DDR will not be present for the secondary tile.*/
bool plat_is_secondary_phys_dram_present(void)
{
	return plat_get_secondary_dram_physical_size() != 0;
}

/* Checks to make sure the combined primary + secondary tile sizes are not bigger than the 3GB max for Adrv906x*/
bool plat_check_ddr_size(void)
{
	return (plat_get_dram_size() + plat_get_secondary_dram_size()) > MAX_DDR_SIZE;
}

bool *plat_get_secure_peripherals(uint32_t *len)
{
	*len = FW_CONFIG_PERIPH_NUM_MAX;
	return secure_peripherals;
}

bool *plat_get_secure_pins(uint32_t *len)
{
	*len = FW_CONFIG_PIN_NUM_MAX;
	return secure_pins;
}

/* Returns true if ECC on primary DDR is enabled */
bool plat_is_primary_ecc_enabled(void)
{
	/* ECC must be disabled on Protium and Palladium */
	if (plat_is_protium() || plat_is_palladium())
		return false;
	else
		return ddr_primary_ecc_enabled;
}

/* Returns true if ECC on secondary DDR is enabled*/
bool plat_is_secondary_ecc_enabled(void)
{
	/* ECC must be disabled on Protium and Palladium */
	if (plat_is_protium() || plat_is_palladium())
		return false;
	else
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
 * Returns the Ethernet PLL frequency setting
 * 0 for 10GHz, 1 for 25GHz
 */
uint32_t plat_get_ethpll_freq_setting(void)
{
	assert(eth_pll_freq_setting <= 1U);
	if (eth_pll_freq_setting > 1U)
		eth_pll_freq_setting = 0U;
	return (uint32_t)eth_pll_freq_setting;
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

#pragma weak plat_board_get_mac
int plat_board_get_mac(uint32_t index, uint8_t **mac)
{
	*mac = (uint8_t *)zero_mac;

	return 0;
}

int plat_get_num_macs(void)
{
	return dual_tile_enabled ? MAX_NUM_MACS : (MAX_NUM_MACS / 2);
}

/*
 * Returns the mac address assigned to the indexed interface:
 * 0 - Primary 1G interface
 * 1 - Primary first 10/25G interface
 * 2 - Primary second 10/25G interface
 * 3 - Secondary 1G interface
 * 4 - Secondary first 10/25G interface
 * 5 - Secondary second 10/25G interface
 *
 * If the mac address is not configured anywhere, it returns 00:00:00:00:00:00
 */
int plat_get_mac_setting(uint32_t index, uint8_t **mac)
{
	int err;
	uint8_t *aux;

	err = plat_board_get_mac(index, mac);
	if ((err == 0) && (!is_zero_mac(*mac))) {
		aux = *mac;
		INFO("Mac %d read from custom hook: %02x:%02x:%02x:%02x:%02x:%02x\n",
		     index + 1, aux[0], aux[1], aux[2], aux[3], aux[4], aux[5]);
		return 0;
	}

	if (err == 0) {
		err = get_mac_from_bootcfg(index, mac);
		if ((err == 0) && (!is_zero_mac(*mac))) {
			aux = *mac;
			INFO("Mac %d read from bootcfg: %02x:%02x:%02x:%02x:%02x:%02x\n",
			     index + 1, aux[0], aux[1], aux[2], aux[3], aux[4], aux[5]);
			return 0;
		}
	}

	if (err == 0) {
		err = get_mac_from_otp(index, mac);
		if ((err == 0) && (!is_zero_mac(*mac))) {
			aux = *mac;
			INFO("Mac %d read from otp: %02x:%02x:%02x:%02x:%02x:%02x\n",
			     index + 1, aux[0], aux[1], aux[2], aux[3], aux[4], aux[5]);
			return 0;
		}
	}

	if (err == 0)
		if (is_zero_mac(*mac))
			/* MAC not found */
			err = ENOATTR;

	return err;
}

/*
 * Returns the current boot slot ID(e.g.'a')
 */
const char *plat_get_boot_slot(void)
{
	const void *data = NULL;
	int err = -1;

	err = get_fw_config_string("/boot-slot", "slot", &data, true);
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

	err = set_fw_config_string("/boot-slot", "slot", slot, true);
	if (err != 0)
		handle_fw_config_write_error("boot slot", err);
}

/*
 * Returns the nv counter for anti-rollback
 */
uint32_t plat_get_fw_config_rollback_ctr(void)
{
	uint32_t data;
	int err = -1;

	err = get_fw_config_uint32("/anti-rollback", "nv-ctr", &data, true);
	if (err != 0)
		handle_fw_config_read_error("nv counter", err);

	return data;
}

/*
 * Sets the nv counter from the FIP certificate for anti-rollback
 */
void plat_set_fw_config_rollback_ctr(void)
{
	int err = -1;

	err = set_fw_config_uint32("/anti-rollback", "nv-ctr", plat_get_cert_nv_ctr(), true);
	if (err != 0)
		handle_fw_config_write_error("nv counter", err);
}

/*
 * Gets the TE rollback counter
 */
uint32_t plat_get_fw_config_te_rollback_ctr(void)
{
	uint32_t data;
	int err = -1;

	err = get_fw_config_uint32("/anti-rollback", "te-rollback-ctr", &data, true);
	if (err != 0)
		handle_fw_config_read_error("te rollback counter", err);

	return data;
}

/*
 * Sets the TE rollback counter
 */
void plat_set_fw_config_te_rollback_ctr(uint32_t ctr)
{
	int err = -1;

/* Workaround for testing in debug mode:
 * Use any TE anti-rollback value already defined in FW_CONFIG instead of the function parameter.
 * This allows us to modify the TE rollback counter from the test framework.
 */
#if DEBUG == 1
	uint32_t data;
	if (get_fw_config_uint32("/anti-rollback", "te-rollback-ctr", &data, true) == 0) ctr = data;
#endif

	err = set_fw_config_uint32("/anti-rollback", "te-rollback-ctr", ctr, true);
	if (err != 0)
		handle_fw_config_write_error("te rollback counter", err);
}

/*
 * Get the anti-rollback enforcement counter from OTP
 */
int plat_get_enforcement_counter(unsigned int *nv_ctr)
{
	return adrv906x_otp_get_rollback_counter(OTP_BASE, nv_ctr);
}

/*
 * Add error message to device tree
 */
void plat_set_fw_config_error_log(char *input)
{
	int err = -1;
	int node = -1;
	uint32_t error_num = 0;
	char name[MAX_NODE_NAME_LENGTH];

	/* Check for error log node, else skip logging */
	node = get_fw_config_node("/error-log", false);
	if (node < 0) {
		INFO("Skipping error logging in device tree\n");
		return;
	}

	/* Get current number or errors */
	err = get_fw_config_uint32("/error-log", "errors", &error_num, false);
	if (err != 0) {
		INFO("Unable to get error-log number of errors\n");
		return;
	}

	/* Get property name for this error */
	snprintf(name, MAX_NODE_NAME_LENGTH, "error-%d", error_num);

	/* Set property with error/warning message */
	err = set_fw_config_string("/error-log", name, input, false);
	if (err != 0) {
		INFO("Unable to log error to fw_config\n");
		return;
	}

	/* Set new number of errors */
	err = set_fw_config_uint32("/error-log", "errors", error_num + 1, false);
	if (err != 0)
		INFO("Unable to update log\n");
}

/*
 * Get current number of errors from device tree
 */
uint32_t plat_get_fw_config_error_num(void)
{
	int err = -1;
	uint32_t error_num = -1;

	/* Get number of errors in error-log */
	err = get_fw_config_uint32("/error-log", "errors", &error_num, false);
	if (err < 0)
		return err;

	return error_num;
}

bool plat_is_bootrom_bypass_enabled(void)
{
	return bootrom_bypass_enabled;
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

bool plat_is_hardware(void)
{
	uint8_t reg = mmio_read_8(PLATFORM_ID_REG);

	if (((reg & PLATFORM_ID_MASK) >> PLATFORM_ID_SHIFT) == 0x0U)
		return true;
	else
		return false;
}
