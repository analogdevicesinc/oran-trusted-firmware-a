/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdio.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <lib/fconf/fconf.h>
#include <lib/fconf/fconf_dyn_cfg_getter.h>
#include <lib/mmio.h>
#include <lib/utils.h>
#include <lib/xlat_tables/xlat_tables_compat.h>
#include <platform.h>

#include <platform_def.h>
#include <plat_boot.h>
#include <plat_bootcfg.h>
#include <plat_bootctrl.h>
#include <plat_console.h>
#include <plat_device_profile.h>
#include <plat_err.h>
#include <plat_io_storage.h>
#include <plat_mmap.h>
#include <plat_ras.h>
#include <plat_setup.h>
#include <plat_status_reg.h>
#include <plat_te.h>
#include <plat_wdt.h>

#ifdef RMA_CLI
#include <plat_cli.h>
#endif

/*
 * Memory region mappings
 */
#define MAP_BL1_TOTAL           MAP_REGION_FLAT(                        \
		bl1_tzram_layout.total_base,    \
		bl1_tzram_layout.total_size,    \
		MT_MEMORY | MT_RW | MT_SECURE)
/*
 * If SEPARATE_CODE_AND_RODATA=1 we define a region for each section
 * otherwise one region is defined containing both
 */
#if SEPARATE_CODE_AND_RODATA
#define MAP_BL1_RO              MAP_REGION_FLAT(                        \
		BL_CODE_BASE,                   \
		BL1_CODE_END - BL_CODE_BASE,    \
		MT_CODE | MT_SECURE),           \
	MAP_REGION_FLAT(                        \
		BL1_RO_DATA_BASE,               \
		BL1_RO_DATA_END                 \
		- BL_RO_DATA_BASE,      \
		MT_RO_DATA | MT_SECURE)
#else
#define MAP_BL1_RO              MAP_REGION_FLAT(                        \
		BL_CODE_BASE,                   \
		BL1_CODE_END - BL_CODE_BASE,    \
		MT_CODE | MT_SECURE)
#endif

/* Holds copy of non-secure and secure reset cause before it gets set to default */
static uint32_t reset_cause_copy;
static uint32_t reset_cause_ns_copy;

/* Data structure which holds the extents of the trusted SRAM for BL1 */
static meminfo_t bl1_tzram_layout;

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}

/*******************************************************************************
 * Load the FW_CONFIG file.
 * We implement this here in the platform layer since we are not using the
 * FCONF framework.
 ******************************************************************************/
static void plat_load_fw_config(void)
{
	image_info_t config_image_info = {
		.h.type		= (uint8_t)PARAM_IMAGE_BINARY,
		.h.version	= (uint8_t)VERSION_2,
		.h.size		= (uint16_t)sizeof(image_info_t),
		.h.attr		= 0,
		.image_base	= FW_CONFIG_BASE,
		.image_max_size = FW_CONFIG_MAX_SIZE
	};
	int err = -1;

	VERBOSE("Loading FW_CONFIG\n");
	err = load_auth_image(FW_CONFIG_ID, &config_image_info);
	if (err != 0) {
		plat_error_message("Loading of FW_CONFIG failed %d", err);
		plat_error_handler(err);
	}

	INFO("FW_CONFIG loaded at address = 0x%lx\n", config_image_info.image_base);
	fconf_populate("FW_CONFIG", FW_CONFIG_BASE);
	INFO("FW_CONFIG populated\n");
}

/*******************************************************************************
 * Load the bootcfg file.
 ******************************************************************************/
static void plat_load_bootcfg(void)
{
	int err = -1;

	err = plat_bootcfg_init();
	if (err != 0) {
		/* Clear mem and flush if loading of bootcfg failed */
		zero_normalmem((void *)BOOTCFG_BASE, BOOTCFG_MAX_SIZE);
		flush_dcache_range(BOOTCFG_BASE, BOOTCFG_MAX_SIZE);
	} else {
		INFO("Bootcfg loaded at address = 0x%lx\n", BOOTCFG_BASE);
	}
}

/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void bl1_early_platform_setup(void)
{
	/* Save reset cause register and set current
	 * reset cause to WDT timeout (default)
	 */
	reset_cause_copy = plat_rd_status_reg(RESET_CAUSE);
	reset_cause_ns_copy = plat_rd_status_reg(RESET_CAUSE_NS);
	plat_wr_status_reg(RESET_CAUSE, WATCHDOG_RESET);
	plat_wr_status_reg(RESET_CAUSE_NS, WATCHDOG_RESET);

	/* Perform early setup
	 * This contains the minimum setup necessary to
	 * enable the watchdog and console on the platform.
	 */
	plat_bl1_early_setup();

	/* Enable watchdog timer */
	plat_secure_wdt_start();

	/* Enable boot console */
	plat_console_boot_init();

	/* Enable cache ECC */
	plat_enable_cache_ecc();

	/* Setup BL1 RAM region */
	bl1_tzram_layout.total_base = BL_RAM_BASE;
	bl1_tzram_layout.total_size = BL_RAM_SIZE;
}

/******************************************************************************
 * Perform the very early platform specific architecture setup.  This only
 * does basic initialization. Later architectural setup (bl1_arch_setup())
 * does not do anything platform specific.
 *****************************************************************************/
void bl1_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_BL1_TOTAL,
		MAP_BL1_RO,
		{ 0 }
	};

	/* Setup and enable MMU */
	setup_page_tables(bl_regions, plat_get_mmap());
	enable_mmu_el3(0);
}

void bl1_platform_setup(void)
{
	char active_boot_slot[] = " ";

#ifdef RMA_CLI
	/* Enter CLI for secure debug access and RMA */
	plat_enter_cli();
#endif

	/* Enable system timer */
	generic_delay_timer_init();

	/* Do platform-specific setup */
	plat_bl1_setup();

	/* Setup storage abstraction layer, enable boot failure detection mechanism */
	plat_io_setup(true, reset_cause_copy, reset_cause_ns_copy);

	/* Load FW_CONFIG */
	plat_load_fw_config();

	/* Load bootcfg if not host boot */
	if (plat_get_boot_device() != PLAT_BOOT_DEVICE_HOST)
		plat_load_bootcfg();

	/* Read the active boot slot as determined by bootctrl and save it
	 * for downstream BL stages to use.
	 */
	active_boot_slot[0] = plat_bootctrl_get_active_slot();
	plat_set_boot_slot(active_boot_slot);

	/* Log previous reset cause and boot failure messages to device tree */
	plat_log_dt_boot_messages();

	/* Set nv counter in FW_CONFIG for BL31 stage to propagate to
	 * OP-TEE via HW_CONFIG
	 */
	plat_set_fw_config_rollback_ctr();

	/* Set TE anti-rollback counter in FW_CONFIG for BL31 stage to
	 * propagate to OP-TEE via HW_CONFIG
	 */
	plat_set_fw_config_te_rollback_ctr(APP_PACK_ANTI_ROLLBACK_VERSION);

	/* Initialize the tiny enclave mailbox */
	plat_enclave_mailbox_init();
}
