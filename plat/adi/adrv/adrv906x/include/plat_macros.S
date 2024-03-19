/*
 * Copyright (c) 2014-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PLAT_MACROS_S
#define PLAT_MACROS_S

#include <platform_def.h>

	/* ---------------------------------------------
	 * The below required platform porting macro
	 * prints out relevant GIC registers whenever an
	 * unhandled exception is taken in BL31.
	 * ---------------------------------------------
	 */
	.macro plat_crash_print_regs
	/* TODO: Review plat/arm/fvp implementation */
	.endm

#endif /* PLAT_MACROS_S */
