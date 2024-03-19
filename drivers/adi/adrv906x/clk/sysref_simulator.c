/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <lib/mmio.h>

#include <drivers/adi/adrv906x/sysref_simulator.h>

#include "sysref_simulator_regmap.h"

/*--------------------------------------------------------
 * DEFINES
 *------------------------------------------------------*/
#define SIM_CTRL_SYSREF_ON_BIT   0x02 /* Note: Magicnumber because this is not defined in yoda */

/*--------------------------------------------------------
 * EXPORTED FUNCTIONS
 *------------------------------------------------------*/
void simulated_sysref_enable(uintptr_t baseaddr)
{
	uintptr_t addr = baseaddr + PLATFORM_CONTROL_OFFSET;
	uint8_t plat_ctrl;

	plat_ctrl = mmio_read_8(addr);
	plat_ctrl |= (SIM_CTRL_SYSREF_ON_BIT << SIMULATION_CTRL_POS);
	mmio_write_8(addr, plat_ctrl);
}

void simulated_sysref_disable(uintptr_t baseaddr)
{
	uintptr_t addr = baseaddr + PLATFORM_CONTROL_OFFSET;
	uint8_t plat_ctrl;

	plat_ctrl = mmio_read_8(addr);
	plat_ctrl &= ~(SIM_CTRL_SYSREF_ON_BIT << SIMULATION_CTRL_POS);
	mmio_write_8(addr, plat_ctrl);
}
