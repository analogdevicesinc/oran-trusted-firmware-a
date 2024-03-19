/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <lib/mmio.h>

#include "adi_spu.h"

void adi_spu_disable_ssec(uintptr_t base, unsigned int peripheral_id)
{
	uintptr_t addr = SPU_SECUREP_REG(base, peripheral_id);
	uint32_t reg = mmio_read_32(addr);

	reg = SPU_SECUREP_REG_CLR_SSEC(reg);
	mmio_write_32(addr, reg);
}

void adi_spu_enable_ssec(uintptr_t base, unsigned int peripheral_id)
{
	uintptr_t addr = SPU_SECUREP_REG(base, peripheral_id);
	uint32_t reg = mmio_read_32(addr);

	reg = SPU_SECUREP_REG_SET_SSEC(reg);
	mmio_write_32(addr, reg);
}

void adi_spu_disable_msec(uintptr_t base, unsigned int peripheral_id)
{
	uintptr_t addr = SPU_SECUREP_REG(base, peripheral_id);
	uint32_t reg = mmio_read_32(addr);

	reg = SPU_SECUREP_REG_CLR_MSEC(reg);
	mmio_write_32(addr, reg);
}

void adi_spu_enable_msec(uintptr_t base, unsigned int peripheral_id)
{
	uintptr_t addr = SPU_SECUREP_REG(base, peripheral_id);
	uint32_t reg = mmio_read_32(addr);

	reg = SPU_SECUREP_REG_SET_MSEC(reg);
	mmio_write_32(addr, reg);
}

void adi_spu_enable_write_protect(uintptr_t base, unsigned int peripheral_id, uint32_t master_id)
{
	uintptr_t addr = SPU_WP_REG(base, peripheral_id);
	uint32_t reg = mmio_read_32(addr);

	reg |= (1 << master_id);
	mmio_write_32(addr, reg);
}

void adi_spu_disable_write_protect(uintptr_t base, unsigned int peripheral_id, uint32_t master_id)
{
	uintptr_t addr = SPU_WP_REG(base, peripheral_id);
	uint32_t reg = mmio_read_32(addr);

	reg &= ~(1 << master_id);
	mmio_write_32(addr, reg);
}

void adi_spu_init(uintptr_t base, unsigned int peripherals_count)
{
	unsigned int i;

	for (i = 0; i < peripherals_count; i++) {
		// disable msec, enable ssec
		mmio_write_32(SPU_SECUREP_REG(base, i), 1);
		// disable write-protect
		mmio_write_32(SPU_WP_REG(base, i), 0);
	}
}
