/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <platform_def.h>
#include <plat_mailbox.h>

	.globl	plat_get_my_entrypoint
	.globl	plat_secondary_cold_boot_setup
	.globl	plat_panic_handler

	/* -----------------------------------------------------
	 * void plat_wait_for_warm_boot (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a CPU to be put into holding pen to wait
	 * for a warm boot request.
	 * The function will never return.
	 * -----------------------------------------------------
	 */
func plat_wait_for_warm_boot
	/*
	 * Calculate address of our hold entry.
	 * As the function will never return, there is no need to save LR.
	 */
	bl	plat_my_core_pos
	lsl	x0, x0, #3
	mov_imm	x2, PLAT_TM_HOLD_BASE
	add	x0, x0, x2
	/*
	 * This code runs way before requesting the warmboot of this core,
	 * so it is possible to clear the mailbox before getting a request
	 * to boot.
	 */
	mov	x1, PLAT_TM_HOLD_STATE_WAIT
	str	x1,[x0]

	/* Wait until we have a go */
poll_mailbox:
	wfe
	ldr	x1, [x0]
	cmp	x1, PLAT_TM_HOLD_STATE_GO
	bne	poll_mailbox

	/* Jump to the provided entrypoint */
	mov_imm	x0, PLAT_TM_ENTRYPOINT
	ldr	x1, [x0]
	br	x1
endfunc plat_wait_for_warm_boot
	
	/* ---------------------------------------------------------------------
	 * uintptr_t plat_get_my_entrypoint (void);
	 *
	 * Main job of this routine is to distinguish between a cold and warm
	 * boot.
	 *
	 * For a cold boot, return 0.
	 * For a warm boot, read the mailbox and return the address it contains.
	 * ---------------------------------------------------------------------
	 */
func plat_get_my_entrypoint
	/* TODO: Determine if warm reset needs to be supported */
	/* Cold reset */
	mov	x0, #0
	ret
endfunc plat_get_my_entrypoint

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset e.g
	 * mark the cpu's presence, mechanism to place it in a
	 * holding pen etc.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	b	plat_wait_for_warm_boot
endfunc plat_secondary_cold_boot_setup

	/* ---------------------------------------------
	 * void plat_panic_handler (void) __dead2;
	 *
	 * Call system reset function on panic.
	 * Set up an emergency stack to run C functions
	 *
	 * Call function to set reset cause
	 * Call function to perform warm reset
	 * Endless loop
	 * ---------------------------------------------
	 */
func plat_panic_handler
	msr spsel, #0
	bl plat_set_my_stack
	bl plat_panic_reset_cause
	b plat_warm_reset
	b .
endfunc plat_panic_handler
