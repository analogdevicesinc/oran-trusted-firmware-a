/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_SETUP_H
#define PLAT_SETUP_H

/*
 * Common translation table mappings for BL RO data
 *
 * If SEPARATE_CODE_AND_RODATA=1 we define a region for each section
 * otherwise one region is defined containing both.
 */
#if SEPARATE_CODE_AND_RODATA
#define PLAT_MAP_BL_RO                  MAP_REGION_FLAT(                        \
		BL_CODE_BASE,                   \
		BL_CODE_END - BL_CODE_BASE,     \
		MT_CODE | MT_SECURE),           \
	MAP_REGION_FLAT(                        \
		BL_RO_DATA_BASE,                \
		BL_RO_DATA_END                  \
		- BL_RO_DATA_BASE,      \
		MT_RO_DATA | MT_SECURE)
#else
#define PLAT_MAP_BL_RO                  MAP_REGION_FLAT(                        \
		BL_CODE_BASE,                   \
		BL_CODE_END - BL_CODE_BASE,     \
		MT_CODE | MT_SECURE)
#endif
void plat_bl1_early_setup(void);
void plat_bl2_early_setup(void);
void plat_bl31_early_setup(void);

void plat_bl1_setup(void);
void plat_bl2_setup(void);
void plat_bl31_setup(void);

#endif /* PLAT_SETUP_H */
