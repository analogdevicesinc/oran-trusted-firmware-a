/*
 * Copyright (c) 2014-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/arm/tzc400.h>
#include <drivers/adi/adi_spu.h>

#include <plat_common_def.h>
#include <plat_security.h>

/* Initialize the TrustZone Controller for ADI platforms */
void plat_tzc400_setup(uintptr_t tzc_base, const plat_tzc_regions_info_t *tzc_regions)
{
	unsigned int region_index = 1U;

	tzc400_init(tzc_base);

	tzc400_disable_filters();

	/* Region 0 set to no access by default */
	tzc400_configure_region0(TZC_REGION_S_NONE, 0);

	/* Configure each specified region according to tzc_regions array until empty region {0} is found */
	for (; tzc_regions->base != 0ULL || tzc_regions->end != 0ULL; tzc_regions++) {
		tzc400_configure_region(TZC_400_REGION_ATTR_FILTER_BIT_ALL, region_index,
					tzc_regions->base, tzc_regions->end, tzc_regions->sec_attr, tzc_regions->nsaid_permissions);
		region_index++;
	}

	/*
	 * Configure exception when an access violation occurs
	 * TZC_ACTION_ERR is the only TZC action that is supported and has been tested
	 */
	tzc400_set_action(TZC_ACTION_ERR);

	tzc400_enable_filters();
}

/* Initialize the System Protection Unit for ADI platforms */
void plat_spu_setup(uintptr_t spu_base,
		    const plat_spu_peripherals_info_t *spu_peripherals,
		    size_t spu_peripherals_count)
{
	unsigned int index = 0;

	adi_spu_init(spu_base, spu_peripherals_count);

	for (index = 0; index < spu_peripherals_count; index++) {
		unsigned int master_idx = 0;
		const plat_spu_peripherals_info_t *peripheral = &spu_peripherals[index];

		if ((peripheral->flags & ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC) != 0)
			adi_spu_disable_ssec(spu_base, index);
		if ((peripheral->flags & ADI_SPU_PERIPHERAL_FLAGS_MSEC) != 0)
			adi_spu_enable_msec(spu_base, index);

		for (master_idx = 0; master_idx < ADI_SPU_MAX_MASTERS; master_idx++)
			if (((peripheral->write_protect >> master_idx) & 1) == 1)
				adi_spu_enable_write_protect(spu_base, index, master_idx);
	}
}
