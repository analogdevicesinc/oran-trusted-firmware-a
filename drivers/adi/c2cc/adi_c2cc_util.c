/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <drivers/adi/adi_c2cc.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include "adi_c2cc.h"

uintptr_t adi_c2cc_primary_addr_base = 0;
uintptr_t adi_c2cc_secondary_addr_base = 0;
bool adi_c2cc_loopback = false;

uint32_t adi_c2cc_read_bf32(uintptr_t addr, uint8_t position, uint32_t mask)
{
	return (mmio_read_32(addr) & mask) >> position;
}

void adi_c2cc_write_bf32(uintptr_t addr, uint8_t position, uint32_t mask, uint32_t val)
{
	uint32_t reg = (mmio_read_32(addr) & ~mask);

	mmio_write_32(addr, reg | ((val << position) & mask));
}

bool adi_c2cc_wait_transactions(uintptr_t addr_base)
{
	uint64_t timeout = timeout_init_us(ADI_C2C_MAX_AXI_WAIT);

	while (ADI_C2CC_IS_AXI_OUTSTANDING(addr_base))
		if (timeout_elapsed(timeout))
			return false;
	return true;
}

void adi_c2cc_intr_control(uintptr_t addr_base, bool enabled)
{
	uint32_t reg = 0x0;

	/* Enable/disable all individual interrupt ports */
	/* TODO: Determine if these should be enabled on a per-interrupt
	 * basis or if it is ok to do just turn everything on, as it is
	 * done here.
	 */
	for (reg = (addr_base + ADI_C2CC_REG_INTPORTEN0); reg <= (addr_base + ADI_C2CC_REG_INTPORTEN9); reg += sizeof(uint32_t))
		mmio_write_32(reg, enabled ? 0xFFFFFFFF : 0x0);

	/* Then configure the global interrupt enable */
	ADI_C2CC_WRITE_INT_EN(addr_base, enabled ? 1 : 0);
	dsb();
}

void adi_c2cc_axi_control(uintptr_t addr_base, bool enabled)
{
	ADI_C2CC_WRITE_AXI_INIT_EN(addr_base, enabled ? 1 : 0);
	dsb();
}
