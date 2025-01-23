/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <lib/xlat_tables/xlat_tables_compat.h>

#include <adrv906x_mmap.h>
#include <adrv906x_secondary_image.h>
#include <platform_def.h>
#include <plat_mmap.h>

#define MAP_DEVICE0     MAP_REGION_FLAT(DEVICE0_BASE,                   \
					DEVICE0_SIZE,                   \
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_DEVICE1     MAP_REGION_FLAT(DEVICE1_BASE,                   \
					DEVICE1_SIZE,                   \
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_DEVICE2     MAP_REGION_FLAT(DEVICE2_BASE,                   \
					DEVICE2_SIZE,                   \
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_DEVICE3     MAP_REGION_FLAT(DEVICE3_BASE,                   \
					DEVICE3_SIZE,                   \
					MT_DEVICE | MT_RW | MT_SECURE)

#ifdef TEST_FRAMEWORK
#define MAP_TEST_DRAM   MAP_REGION_FLAT(TEST_DRAM_BASE,                 \
					TEST_DRAM_SIZE,                 \
					MT_DEVICE | MT_RW | MT_SECURE)
#endif

/*
 * Table of memory regions for various BL stages to map using the MMU.
 * This doesn't include Trusted SRAM as setup_page_tables() already takes care
 * of mapping it.
 */

#ifdef IMAGE_BL1
static const mmap_region_t plat_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
	MAP_DEVICE1,
	MAP_DEVICE2,
#ifdef TEST_FRAMEWORK
	MAP_DEVICE3,
	MAP_TEST_DRAM,
#endif
	{ 0 }
};
#endif

#ifdef IMAGE_BL2
static const mmap_region_t plat_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
	MAP_DEVICE1,
	MAP_DEVICE2,
	MAP_DEVICE3,
	MAP_TEE_SHMEM_DRAM,
	MAP_SEC_DRAM,
	MAP_NS_DRAM,
	{ 0 }
};
#endif

#ifdef IMAGE_BL31
const mmap_region_t plat_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
	MAP_DEVICE1,
	MAP_DEVICE2,
	MAP_HW_CONFIG,
	MAP_SEC_DRAM,
	{ 0 }
};
#endif

#ifdef IMAGE_BL32
static const mmap_region_t plat_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
	MAP_DEVICE1,
	MAP_SEC_DRAM,
	MAP_NS_DRAM,
	{ 0 }
};
#endif

static int add_mmap_region(uintptr_t base, size_t size, unsigned int attr)
{
	uintptr_t base_aligned;
	int rc;
	size_t size_aligned;

	/* Align base addr and size on page boundaries */
	base_aligned = page_align(base, DOWN);
	size_aligned = page_align(size, UP);
	rc = mmap_add_dynamic_region((unsigned long long)base_aligned, base_aligned, size_aligned, attr);

	return rc;
}

/*
 * Returns platform-specific memory regions
 */
const mmap_region_t *plat_get_mmap(void)
{
	return plat_mmap;
}

int plat_setup_secondary_mmap(bool device_region_only)
{
	int rc;

	/* Setup secondary device regions */
	rc = add_mmap_region(SEC_DEVICE2_BASE, SEC_DEVICE2_SIZE, MT_DEVICE | MT_RW | MT_SECURE);
	if (rc == 0)
		rc = add_mmap_region(SEC_DEVICE3_BASE, SEC_DEVICE3_SIZE, MT_DEVICE | MT_RW | MT_SECURE);

	if (device_region_only)
		return rc;

	/* Setup region for secondary image */
	if (rc == 0)
		rc = add_mmap_region(PLAT_SEC_IMAGE_DST_ADDR, PLAT_SEC_IMAGE_SIZE, MT_MEMORY | MT_RW | MT_SECURE);

	/* Setup region for secondary boot config */
	if (rc == 0)
		rc = add_mmap_region(PLAT_SEC_BOOT_CFG_ADDR, sizeof(plat_sec_boot_cfg_t), MT_MEMORY | MT_RW | MT_SECURE);

	return rc;
}

int plat_setup_ns_sram_mmap(void)
{
	int rc;

	/* Setup memory region for NS SRAM */
	rc = add_mmap_region(NS_SRAM_BASE, NS_SRAM_SIZE, MT_MEMORY | MT_RW | MT_SECURE);

	return rc;
}
