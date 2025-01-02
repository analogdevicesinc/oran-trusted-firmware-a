/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_compat.h>
#include <platform.h>

#include <platform_def.h>
#include <plat_boot.h>
#include <plat_bootctrl.h>
#include <plat_console.h>
#include <plat_device_profile.h>
#include <plat_err.h>
#include <plat_io_storage.h>
#include <plat_mmap.h>
#include <plat_security.h>
#include <plat_setup.h>
#include <plat_wdt.h>

/*
 * Memory region mappings
 */
#define MAP_BL2_TOTAL           MAP_REGION_FLAT(                        \
		bl2_tzram_layout.total_base,    \
		bl2_tzram_layout.total_size,    \
		MT_MEMORY | MT_RW | MT_SECURE)

/* Data structure which holds the extents of the trusted SRAM for BL2 */
static meminfo_t bl2_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

/*
 * Check that BL2_BASE is above FW_CONFIG_LIMIT. This reserved page is
 * for `meminfo_t` data structure and fw_configs passed from BL1.
 */
CASSERT(BL2_BASE >= FW_CONFIG_LIMIT, assert_bl2_base_overflows);

/* Check that BL_RAM and SHARED_RAM regions don't overlap by checking with SRAM size */
CASSERT(SRAM_SIZE >= BL_RAM_SIZE + SHARED_RAM_SIZE, assert_bl_and_shared_ram_overlap);

/* Disable optimiziations for this function to ensure the FPU enable happens first */
#pragma GCC push_options
#pragma GCC optimize ("O0")
void bl2_early_platform_setup2(u_register_t arg0, u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	/* Enable FPU support early on. This is enabled later in bl2_arch_setup(),
	 * but it appears to be needed earlier in boot, specifically by the NOTICE()
	 * calls in bl2_main(). Most likely this is because removing the -mgeneral-regs-only
	 * for BL2 allows the compiler to use FPU registers.
	 */
#ifdef __aarch64__
	write_cpacr(CPACR_EL1_FPEN(CPACR_EL1_FP_TRAP_NONE));
	isb();
#endif

	/* Perform early setup */
	plat_bl2_early_setup();

	/* Enable watchdog */
	plat_secure_wdt_start();

	/* Enable boot console */
	plat_console_boot_init();

	/* Setup BL2 RAM region
	 * BL2 sees the entire BL RAM region, minus BL1's RW and RO regions.
	 * This calculation assumes BL1_RW is placed before BL1_RO.
	 */
	CASSERT(BL1_RW_BASE > BL_RAM_BASE, bl1_ram_base_incorrect);
	CASSERT(BL1_RO_BASE > BL1_RW_BASE, bl1_rw_base_incorrect);
	bl2_tzram_layout.total_base = BL_RAM_BASE;
	bl2_tzram_layout.total_size = BL1_RW_BASE - BL_RAM_BASE;
}
#pragma GCC pop_options

void bl2_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_BL2_TOTAL,
		PLAT_MAP_BL_RO,
		{ 0 }
	};

	/* Setup and enable MMU */
	setup_page_tables(bl_regions, plat_get_mmap());
	enable_mmu_el1(0);
}

void bl2_platform_setup(void)
{
	size_t dram_size = 0;

	/* Enable system timer */
	generic_delay_timer_init();

	/* Do platform-specific setup */
	plat_bl2_setup();

	/* Setup storage abstraction layer, skip boot failure detection mechanism */
	plat_io_setup(false, 0, 0);

	/* Confirm DRAM size is sufficient before continuing */
	dram_size = plat_get_dram_size();
	if (dram_size < DRAM_SIZE_MIN) {
		plat_error_message("DRAM size 0x%lx is too small. Must be at least 0x%lx bytes.", dram_size, DRAM_SIZE_MIN);
		plat_error_handler(-ENOMEM);
	}

	/* Configure TZC */
	plat_security_setup();
}
