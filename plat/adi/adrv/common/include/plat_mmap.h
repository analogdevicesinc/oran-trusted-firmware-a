/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_MMAP_H
#define PLAT_MMAP_H

#include <lib/xlat_tables/xlat_tables_compat.h>
#include <platform_def.h>

#define MAP_SHARED_RAM  MAP_REGION_FLAT(                \
		SHARED_RAM_BASE,    \
		SHARED_RAM_SIZE,    \
		MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_SEC_DRAM    MAP_REGION_FLAT(SECURE_DRAM_BASE, \
					SECURE_DRAM_SIZE, \
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_TEE_SHMEM_DRAM    MAP_REGION_FLAT(TEE_SHMEM_BASE, \
					      TEE_SHMEM_SIZE, \
					      MT_MEMORY | MT_RW | MT_NS)

#define MAP_NS_DRAM     MAP_REGION_FLAT(NS_DRAM_BASE, \
					NS_DRAM_SIZE_MIN, \
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_HW_CONFIG   MAP_REGION_FLAT(HW_CONFIG_BASE, \
					HW_CONFIG_LIMIT - HW_CONFIG_BASE, \
					MT_MEMORY | MT_RW | MT_NS)


const mmap_region_t *plat_get_mmap(void);

#endif /* PLAT_MMAP_H */
