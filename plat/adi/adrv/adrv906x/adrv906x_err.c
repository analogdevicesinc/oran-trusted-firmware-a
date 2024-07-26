/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>

#include <adrv906x_clkrst_def.h>
#include <drivers/arm/sp805.h>
#include <platform.h>
#include <plat_err.h>

#define RESET_DELAY_NS          10000000
#define CALC_DELAY_LOOPS        (RESET_DELAY_NS / 9)             /* Delay calculated based on 10ns/instruction and 9 instructions per loop iteration */
#define NOP()   asm ("nop")

/* Turn off optimization for loop timing to implement delay */
#pragma GCC push_options
#pragma GCC optimize ("O0")

int plat_warm_reset(void)
{
	int i;

	/* Setup WDT1 to timeout immediately.
	 * WDT1 timeout causes GPINT1 to fire to give the radio time
	 * to shutdown.
	 */
	sp805_refresh(WDOG_TIMER1_BASE, 0U);

	/* Wait 10ms before performing warm reset, to give the radio
	 * time to shutdown.
	 * Cannot use generic delay timer here due to possible reset
	 * early in boot before the generic delay timer is setup.
	 * Using cycle counting to implement a delay.
	 */
	for (i = 0; i < CALC_DELAY_LOOPS; i++)
		NOP();

	/* Set a55_subsys_global_sw_reset bit to perform warm reset */
	mmio_write_32(A55_SYS_CFG + CLK_RST_CTRL_CFG, mmio_read_32(A55_SYS_CFG + CLK_RST_CTRL_CFG) | SUBSYS_GLOBAL_SW_RESET_MASK);

	while (1)
		;

	return PSCI_E_INTERN_FAIL;
}

#pragma GCC pop_options
