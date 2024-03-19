/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>

#include <adrv906x_clkrst_def.h>
#include <platform.h>
#include <plat_status_reg.h>

int plat_warm_reset(void)
{
	/* Set a55_subsys_global_sw_reset bit to perform warm reset */
	mmio_write_32(A55_SYS_CFG + CLK_RST_CTRL_CFG, mmio_read_32(A55_SYS_CFG + CLK_RST_CTRL_CFG) | SUBSYS_GLOBAL_SW_RESET_MASK);

	while (1)
		;

	return PSCI_E_INTERN_FAIL;
}
