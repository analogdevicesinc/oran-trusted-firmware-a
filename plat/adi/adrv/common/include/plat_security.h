/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_SECURITY_H
#define PLAT_SECURITY_H

typedef struct plat_tzc_regions_info {
	unsigned long long base;
	unsigned long long end;
	unsigned int sec_attr;
	unsigned int nsaid_permissions;
} plat_tzc_regions_info_t;

typedef struct plat_spu_peripherals_info {
	uint8_t flags;
	uint32_t write_protect;
} plat_spu_peripherals_info_t;

#define ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC 0x01
#define ADI_SPU_PERIPHERAL_FLAGS_MSEC 0x02

void plat_security_setup(void);
void plat_tzc400_setup(uintptr_t tzc_base, const plat_tzc_regions_info_t *tzc_regions);
void plat_spu_setup(uintptr_t spu_base, const plat_spu_peripherals_info_t *spu_peripherals, size_t spu_peripherals_count);

#endif
