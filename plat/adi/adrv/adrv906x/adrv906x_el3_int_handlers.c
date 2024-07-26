/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <errno.h>
#if EL3_EXCEPTION_HANDLING
#include <services/sdei.h>
#endif

#include <plat/common/platform.h>
#include <lib/extensions/ras.h>

#include <plat_errno.h>
#include <plat_interrupts.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_el3_int_handlers.h>
#include <adrv906x_gpint.h>
#include <adrv906x_irq_def.h>
#include <adrv906x_sram.h>
#include <adrv906x_sdei_events.h>

void plat_assign_interrupt_handlers(void)
{
	/* Set up handlers for each of the GPINTs */
	plat_request_intr_type_el3(IRQ_GP_INTERRUPT_SYNC_0, primary_gpint_handler);
	plat_request_intr_type_el3(IRQ_GP_INTERRUPT_SYNC_1, primary_gpint_handler);

	/* Handlers for cache ECC warnings and errors */
	plat_request_intr_type_el3(IRQ_NERRIRQ_0, cache_l3_error_handler);
	plat_request_intr_type_el3(IRQ_NERRIRQ_1, cache_l1l2_error_handler);
	plat_request_intr_type_el3(IRQ_NERRIRQ_2, cache_l1l2_error_handler);
	plat_request_intr_type_el3(IRQ_NERRIRQ_3, cache_l1l2_error_handler);
	plat_request_intr_type_el3(IRQ_NERRIRQ_4, cache_l1l2_error_handler);

	plat_request_intr_type_el3(IRQ_NFAULTIRQ_0, cache_l3_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_1, cache_l1l2_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_2, cache_l1l2_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_3, cache_l1l2_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_4, cache_l1l2_fault_handler);

	/* Handlers for L4 cache warnings and errors */
	plat_request_intr_type_el3(IRQ_L4_ECC_ERR_INTR_0, l4_error_handler);
	plat_request_intr_type_el3(IRQ_L4_ECC_ERR_INTR_1, l4_error_handler);
	plat_request_intr_type_el3(IRQ_L4_ECC_ERR_INTR_2, l4_error_handler);

	plat_request_intr_type_el3(IRQ_L4_ECC_WRN_INTR_0, l4_warning_handler);
	plat_request_intr_type_el3(IRQ_L4_ECC_WRN_INTR_1, l4_warning_handler);
	plat_request_intr_type_el3(IRQ_L4_ECC_WRN_INTR_2, l4_warning_handler);


	if (plat_get_dual_tile_enabled()) {
		/* Set up handlers for each of the GPINTs received on the Primary from the Secondary */
		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_197, secondary_gpint_handler);
		plat_request_intr_type_el3(IRQ_GPINT_INTERRUPT_SECONDARY_TO_PRIMARY, secondary_gpint_handler);

		/* Handlers for L4 warnings and errors on the secondary */
		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_18, l4_error_handler);
		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_21, l4_error_handler);
		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_24, l4_error_handler);

		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_17, l4_warning_handler);
		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_20, l4_warning_handler);
		plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_23, l4_warning_handler);
	}
}

static uint64_t gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie, uintptr_t gpint_addr)
{
#if EL3_EXCEPTION_HANDLING
	int32_t status = 0;
#endif
	uint64_t ret = 0;
	uint64_t mask = 1;
	struct gpint_settings settings;

	adrv906x_gpint_get_status(gpint_addr, &settings);
	for (int i = 0; i < GPINT_INTS_PER_WORD; i++) {
		if ((settings.upper_word & mask) >> i) { /*If GPINT event is active*/
			if (adrv906x_gpint_is_nonsecure(true, mask)) {
#if EL3_EXCEPTION_HANDLING
				if (gpint_addr == DIG_CORE_BASE)
					status = sdei_dispatch_event(GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD);
				else
					status = sdei_dispatch_event(SEC_GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD);

				if (status != 0) {
					if (gpint_addr == DIG_CORE_BASE)
						ERROR("sdei_dispatch_event for event %d returned %d\n", GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD, status);
					else
						ERROR("sdei_dispatch_event for event %d returned %d\n", SEC_GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD, status);
					ret = 1;
				}
#else
				ERROR("Unhandled GPINT interrupt: %d\n", i);
#endif
			} else {
				ERROR("Unhandled GPINT interrupt: %d\n", i + GPINT_INTS_PER_WORD);
			}
		}

		if ((settings.lower_word & mask) >> i) { /*If GPINT event is active*/
			if (adrv906x_gpint_is_nonsecure(false, mask)) {
#if EL3_EXCEPTION_HANDLING
				if (gpint_addr == DIG_CORE_BASE)
					status = sdei_dispatch_event(GPINT_DEFAULT_SDEI_EVENT + i);
				else
					status = sdei_dispatch_event(SEC_GPINT_DEFAULT_SDEI_EVENT + i);

				if (status != 0) {
					if (gpint_addr == DIG_CORE_BASE)
						ERROR("sdei_dispatch_event for event %d returned %d\n", GPINT_DEFAULT_SDEI_EVENT + i, status);
					else
						ERROR("sdei_dispatch_event for event %d returned %d\n", SEC_GPINT_DEFAULT_SDEI_EVENT + i, status);
					ret = 1;
				}
#else
				ERROR("Unhandled GPINT interrupt: %d\n", i);
#endif
			} else {
				ERROR("Unhandled GPINT interrupt: %d\n", i);
			}
		}

		mask = mask << 1;
	}

	return ret;
}

uint64_t primary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	return gpint_handler(id, flags, handle, cookie, DIG_CORE_BASE);
}

uint64_t secondary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	return gpint_handler(id, flags, handle, cookie, SEC_DIG_CORE_BASE);
}

/* Handler for ECC errors, correctable and uncorrectable, in the L3 cache.
 *  Uncorrectable errors will trigger the error handler as well. */
uint64_t cache_l1l2_fault_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	int core = 0;

	switch (id) {
	case IRQ_NFAULTIRQ_1:
		core = 0;
		break;
	case IRQ_NFAULTIRQ_2:
		core = 1;
		break;
	case IRQ_NFAULTIRQ_3:
		core = 2;
		break;
	case IRQ_NFAULTIRQ_4:
		core = 3;
		break;
	default:
		ERROR("Invalid int id for handler.\n");
		return -1;
	}

	WARN("ECC error detected in L1/L2 cache on core %d.\n", core);

	return 0;
}

/* Handler for ECC errors, correctable and uncorrectable, in the L3 cache.
 *  Uncorrectable errors will trigger the error handler as well. */
uint64_t cache_l3_fault_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	NOTICE("ECC error detected in L3 cache\n");

	return 0;
}

/* Handler for uncorrectable errors in the L1/L2 caches */
uint64_t cache_l1l2_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	int core = 0;

	/* NFAULTIRQ_0 is reserved for L3 interrupt, so IRQ_NFAULTIRQ_n is a fault in the L1/L2 cache of core [n-1] */
	switch (id) {
	case IRQ_NFAULTIRQ_1:
		core = 0;
		break;
	case IRQ_NFAULTIRQ_2:
		core = 1;
		break;
	case IRQ_NFAULTIRQ_3:
		core = 2;
		break;
	case IRQ_NFAULTIRQ_4:
		core = 3;
		break;
	default:
		ERROR("Invalid int id for handler.\n");
		return -1;
	}
	ERROR("Uncorrectable error detected in L1/L2 cache on core %d, resetting the board\n", core);

	/* Reset the board after logging error */
	plat_error_handler(-EECC);

	return 0;
}

/* Handler for uncorrectable ECC errors in the L3 cache */
uint64_t cache_l3_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	ERROR("Uncorrectable error detected in L3 cache, resetting the board\n");

	/* Reset the board after logging error */
	plat_error_handler(-EECC);

	return 0;
}

/* Handler for correctable ECC errors in the L4 memory */
uint64_t l4_warning_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	uintptr_t base_addr;

	switch (id) {
	case IRQ_L4_ECC_WRN_INTR_0:
		base_addr = L4CTL_CFG0_BASE;
		break;
	case IRQ_L4_ECC_WRN_INTR_1:
		base_addr = L4CTL_CFG1_BASE;
		break;
	case IRQ_L4_ECC_WRN_INTR_2:
		base_addr = L4CTL_CFG2_BASE;
		break;
	case IRQ_C2C_OUT_HW_INTERRUPT_17:
		base_addr = SEC_L4CTL_CFG0_BASE;
		break;
	case IRQ_C2C_OUT_HW_INTERRUPT_20:
		base_addr = SEC_L4CTL_CFG1_BASE;
		break;
	case IRQ_C2C_OUT_HW_INTERRUPT_23:
		base_addr = SEC_L4CTL_CFG2_BASE;
		break;
	default:
		ERROR("Can't match int id to L4 base address.\n");
		return -1;
	}

	l4_warning_info(base_addr);

	return 0;
}

/* Handler for uncorrectable errors in the L4 memory */
uint64_t l4_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	uintptr_t base_addr;

	switch (id) {
	case IRQ_L4_ECC_ERR_INTR_0:
		base_addr = L4CTL_CFG0_BASE;
		break;
	case IRQ_L4_ECC_ERR_INTR_1:
		base_addr = L4CTL_CFG1_BASE;
		break;
	case IRQ_L4_ECC_ERR_INTR_2:
		base_addr = L4CTL_CFG2_BASE;
		break;
	case IRQ_C2C_OUT_HW_INTERRUPT_18:
		base_addr = SEC_L4CTL_CFG0_BASE;
		break;
	case IRQ_C2C_OUT_HW_INTERRUPT_21:
		base_addr = SEC_L4CTL_CFG1_BASE;
		break;
	case IRQ_C2C_OUT_HW_INTERRUPT_24:
		base_addr = SEC_L4CTL_CFG2_BASE;
		break;
	default:
		ERROR("Can't match int id to L4 base address.\n");
		return -1;
	}

	l4_error_info(base_addr);
	plat_error_handler(-EECC);

	return 0;
}
