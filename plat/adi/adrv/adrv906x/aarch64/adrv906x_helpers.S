/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <platform_def.h>
#include <cortex_a55.h>

	.globl	platform_mem_init
	.globl	plat_is_my_cpu_primary
	.globl	plat_my_core_pos
	.globl	plat_calc_core_pos

	/* -----------------------------------------------------
	 *  unsigned int plat_my_core_pos(void)
	 *
	 *  This function uses the plat_calc_core_pos()
	 *  definition to get the index of the calling CPU.
	 * -----------------------------------------------------
	 */
func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_calc_core_pos
endfunc plat_my_core_pos

	/* -----------------------------------------------------
	 *  unsigned int plat_calc_core_pos(u_register_t mpidr);
	 *
	 *  NOTE: This implementation MUST align with plat_core_pos_by_mpidr()
	 *  CorePos = (ClusterId * 4) + CoreId
	 *  This assumes AFF0 (thread ID) is 0.
	 * -----------------------------------------------------
	 */
func plat_calc_core_pos
	/* Extract individual affinity fields from MPIDR */
	ubfx	x1, x0, #MPIDR_AFF1_SHIFT, #MPIDR_AFFINITY_BITS
	ubfx	x0, x0, #MPIDR_AFF2_SHIFT, #MPIDR_AFFINITY_BITS
	add	x0, x1, x0, LSL #2
	ret
endfunc plat_calc_core_pos

	/* -----------------------------------------------------
	 * unsigned int plat_is_my_cpu_primary (void);
	 *
	 * Find out whether the current cpu is the primary
	 * cpu.
	 * -----------------------------------------------------
	 */
func plat_is_my_cpu_primary
	mrs	x0, mpidr_el1
	and	x0, x0, #((MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT) | (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT))
	cmp	x0, #PLAT_PRIMARY_CPU_ID
	cset	w0, eq
	ret
endfunc plat_is_my_cpu_primary

	/* ---------------------------------------------
	 * void platform_mem_init (void);
	 *
	 * Not needed. SRAM init is handled by Tiny Enclave.
	 * ---------------------------------------------
	 */
func platform_mem_init
	ret
endfunc platform_mem_init
