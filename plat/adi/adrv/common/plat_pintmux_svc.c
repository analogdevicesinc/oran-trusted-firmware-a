/*
 * Copyright (c) 2023, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/smccc.h>

#include <plat_pintmux.h>
#include <plat_pintmux_svc.h>
#include <plat_pinctrl.h>

typedef enum {
	MAP	= 1,
	UNMAP	= 2,
} func_id_t;

#define LINUX_SPI_IRQ_BASE_OFFSET 32

/**
 * adapt_irq_base_to_requester - Adapts the IRQ base depending on the requester
 *
 *  OPTEE requires absolute INTIDs.
 *  For the GIC, Linux requires that these are SPI or PPI relative.
 *
 *  OPTEE is secure and U-Boot and Linux are non-secure.
 *  As U-Boot doesn´t enable interrupts, we can use the bool
 *  "is_secure" to identify the requester, OPTEE o Linux.
 */
static uint32_t adapt_irq_base_to_requester(bool is_secure, uint32_t irq)
{
	bool is_linux = !is_secure;

	if (is_linux)
		// Our GPIO kernel driver sets the IRQs as GIC_SPI, so we adapt the IRQ to the SPI_BASE
		irq = irq - LINUX_SPI_IRQ_BASE_OFFSET;

	return irq;
}

static int plat_pintmux_smc_map(unsigned int gpio, bool is_secure, bool pos_mask, uintptr_t base_addr)
{
	int lane;
	uint32_t irq;

	lane = plat_secure_pintmux_map(gpio, is_secure, pos_mask, base_addr);
	if (lane < 0) {
		ERROR("PINTMUX service: Failed to map GPIO_%s_%u - already mapped or no lanes available.\n", is_secure ? "S" : "NS", gpio);
		return lane;
	}
	irq = plat_pintmux_lane_to_irq(lane, base_addr);
	if (irq == 0) {
		ERROR("PINTMUX service: Failed to translate GPIO_%s_%u to IRQ.\n", is_secure ? "S" : "NS", gpio);
		return irq;
	}

	irq = adapt_irq_base_to_requester(is_secure, irq);

	return irq;
}

static int plat_pintmux_smc_unmap(unsigned int gpio, bool is_secure, uintptr_t base_addr)
{
	uint32_t irq;
	int lane;

	lane = plat_secure_pintmux_unmap(gpio, is_secure, base_addr);
	if (lane < 0) {
		ERROR("PINTMUX service: Failed to map GPIO_%s_%u - already mapped or no lanes available.\n", is_secure ? "S" : "NS", gpio);
		return lane;
	}
	irq = plat_pintmux_lane_to_irq(lane, base_addr);
	if (irq == 0) {
		ERROR("PINTMUX service: Failed to translate GPIO_%s_%u to IRQ.\n", is_secure ? "S" : "NS", gpio);
		return irq;
	}

	irq = adapt_irq_base_to_requester(is_secure, irq);

	return irq;
}

uintptr_t plat_pintmux_smc_handler(unsigned int smc_fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4, void *cookie, void *handle, u_register_t flags)
{
	func_id_t fid;
	unsigned int gpio;
	bool is_secure;
	bool pos_mask;
	uintptr_t base_addr;

	fid = (func_id_t)x1;
	gpio = (unsigned int)x2;
	pos_mask = (bool)((x3 == 1) ? true : false);
	base_addr = (uintptr_t)x4;
	is_secure = is_caller_secure(flags);

	switch (fid) {
	case MAP:
		SMC_RET2(handle, SMC_OK, plat_pintmux_smc_map(gpio, is_secure, pos_mask, base_addr));
	case UNMAP:
		SMC_RET2(handle, SMC_OK, plat_pintmux_smc_unmap(gpio, is_secure, base_addr));
	default:
		break;
	}
	WARN("PINTMUX service: Unexpected FID %d\n", fid);
	SMC_RET1(handle, SMC_UNK);
}
