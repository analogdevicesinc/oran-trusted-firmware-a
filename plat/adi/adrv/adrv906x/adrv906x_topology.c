/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <lib/psci/psci.h>

#include <platform.h>
#include <platform_def.h>
#include <plat_helpers.h>

static const unsigned char plat_power_domain_tree_desc[] = { 1, 4 };

/*******************************************************************************
 * This function returns the default topology tree information.
 ******************************************************************************/
const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return plat_power_domain_tree_desc;
}

/*******************************************************************************
 * This function implements a part of the critical interface between the psci
 * generic layer and the platform that allows the former to query the platform
 * to convert an MPIDR to a unique linear index. An error code (-1) is returned
 * in case the MPIDR is invalid.
 *
 * NOTE: This MUST align with plat_calc_core_pos()
 ******************************************************************************/
int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	int core_pos;
	unsigned int cluster_id, cpu_id, pe_id;
	uint64_t valid_mask;

	valid_mask = ~(MPIDR_AFFLVL_MASK |
		       (MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT) |
		       (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT) |
		       (MPIDR_AFFLVL_MASK << MPIDR_AFF3_SHIFT));
	cluster_id = (mpidr >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK;
	cpu_id = (mpidr >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK;
	pe_id = (mpidr >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK;

	mpidr &= MPIDR_AFFINITY_MASK;
	if ((mpidr & valid_mask) != 0U)
		return -1;

	if (cluster_id >= PLATFORM_CLUSTER_COUNT)
		return -1;

	if (cpu_id >= PLATFORM_CORE_COUNT_PER_CLUSTER)
		return -1;

	/* Only one thread per CPU allowed */
	if (pe_id >= 1)
		return -1;

	core_pos = plat_calc_core_pos(mpidr);

	return core_pos;
}
