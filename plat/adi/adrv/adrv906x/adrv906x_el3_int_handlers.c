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

#include <plat_err.h>
#include <plat_errno.h>
#include <plat_interrupts.h>

#include <adrv906x_ddr.h>
#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_el3_int_handlers.h>
#include <adrv906x_gpint.h>
#include <adrv906x_irq_def.h>
#include <adrv906x_sram.h>
#include <adrv906x_sdei_events.h>

static interrupt_type_handler_t primary_gpint_interrupt_table[TOTAL_GPINTS] = { NULL };
static interrupt_type_handler_t secondary_gpint_interrupt_table[TOTAL_GPINTS] = { NULL };

static void __unused plat_request_gpint_intr(uint32_t id, interrupt_type_handler_t handler, bool secondary){
	/* Validate 'handler' and 'id' parameters */
	if (id >= TOTAL_GPINTS) {
		plat_warn_message("Requested GPINT event handler number outside allowed range.");
		return;
	}

	if (!handler) {
		plat_warn_message("No handler given.");
		return;
	}

	if (secondary) {
		/* Check if a handler has already been registered */
		if (secondary_gpint_interrupt_table[id]) {
			plat_warn_message("Handler already exists for this secondary GPINT event.");
			return;
		}

		secondary_gpint_interrupt_table[id] = handler;
	} else {
		/* Check if a handler has already been registered */
		if (primary_gpint_interrupt_table[id]) {
			plat_warn_message("Handler already exists for this primary GPINT event.");
			return;
		}

		primary_gpint_interrupt_table[id] = handler;
	}
}

static uint64_t gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie, uintptr_t gpint_addr)
{
#if EL3_EXCEPTION_HANDLING
	int32_t status = 0;
#endif
	uint64_t ret = 0;
	uint64_t mask = 1;
	uint32_t gpint = 0;
	struct gpint_settings settings;
	interrupt_type_handler_t handler;

	if (id == IRQ_GP_INTERRUPT_SYNC_0 || IRQ_GPINT_INTERRUPT_SECONDARY_TO_PRIMARY)
		gpint = GPINT0;
	else if (id == IRQ_GP_INTERRUPT_SYNC_1 || IRQ_C2C_OUT_HW_INTERRUPT_197)
		gpint = GPINT1;

	adrv906x_gpint_get_masked_status(gpint_addr, &settings, gpint);
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
						plat_error_message("sdei_dispatch_event for event %d returned %d", GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD, status);
					else
						plat_error_message("sdei_dispatch_event for event %d returned %d", SEC_GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD, status);
					ret = 1;
				}
#else
				plat_error_message("Unhandled GPINT interrupt: %d", i);
#endif
			} else {
				if (gpint_addr == DIG_CORE_BASE)
					handler = primary_gpint_interrupt_table[i + GPINT_INTS_PER_WORD];
				else
					handler = secondary_gpint_interrupt_table[i + GPINT_INTS_PER_WORD];
				if (handler == NULL)
					plat_error_message("No handler for GPINT event %d", i);
				else
					ret = handler(i + GPINT_INTS_PER_WORD, flags, handle, cookie);
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
						plat_error_message("sdei_dispatch_event for event %d returned %d", GPINT_DEFAULT_SDEI_EVENT + i, status);
					else
						plat_error_message("sdei_dispatch_event for event %d returned %d", SEC_GPINT_DEFAULT_SDEI_EVENT + i, status);
					ret = 1;
				}
#else
				plat_error_message("Unhandled GPINT interrupt: %d", i);
#endif
			} else {
				if (gpint_addr == DIG_CORE_BASE)
					handler = primary_gpint_interrupt_table[i];
				else
					handler = secondary_gpint_interrupt_table[i];

				if (handler == NULL)
					plat_error_message("No handler for GPINT event %d", i);
				else
					ret = handler(i, flags, handle, cookie);
			}
		}

		mask = mask << 1;
	}

	return ret;
}

static uint64_t primary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	return gpint_handler(id, flags, handle, cookie, DIG_CORE_BASE);
}

static uint64_t secondary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	return gpint_handler(id, flags, handle, cookie, SEC_DIG_CORE_BASE);
}

static uint64_t ddr_ecc_uncorrected_err_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	adrv906x_ddr_log_uncorrectable_error(DDR_CTL_BASE);
	plat_error_handler(-EDECC);
	/* We should never reach here, so if we do return bad status code so interrupt driver will report it */
	return -1;
}

static uint64_t ddr_ap_err_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	plat_warn_message("DDR Address Protection errors have exceeded threshold");
	adrv906x_ddr_clear_ap_error(DDR_CTL_BASE);
	return 0;
}

static uint64_t ddr_ecc_corrected_err_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	adrv906x_ddr_log_correctable_error(DDR_CTL_BASE);
	return 0;
}

static uint64_t ddr_phy_err_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	switch (id) {
	case IRQ_O_DFI_INTERNAL_ERR_INTR:
		plat_error_message("DFI internal error detected, resetting the board");
		break;
	case IRQ_O_DFI_PHYUPD_ERR_INTR:
		plat_error_message("DFI PHY update error detected, resetting the board");
		break;
	case IRQ_O_DFI_ALERT_ERR_INTR:
		plat_error_message("DFI alert error detected, resetting the board");
		break;
	case IRQ_O_DWC_DDRPHY_INT_N:
		plat_error_message("DWC DDRPHY interrupt detected, resetting the board");
		break;
	}

	plat_error_handler(-EIO);
	/* We should never reach here, so if we do return bad status code so interrupt driver will report it */
	return -1;
}

/* Handler for ECC errors, correctable and uncorrectable, in the L3 cache.
 *  Uncorrectable errors will trigger the error handler as well. */
static uint64_t cache_l1l2_fault_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
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
		plat_error_message("Invalid int id for handler.");
		return -1;
	}

	plat_warn_message("ECC error detected in L1/L2 cache on core %d.", core);

	return 0;
}

/* Handler for ECC errors, correctable and uncorrectable, in the L3 cache.
 *  Uncorrectable errors will trigger the error handler as well. */
static uint64_t cache_l3_fault_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	NOTICE("ECC error detected in L3 cache\n");

	return 0;
}

/* Handler for uncorrectable errors in the L1/L2 caches */
static uint64_t __unused cache_l1l2_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie){
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
		plat_error_message("Invalid int id for handler.");
		return -1;
	}
	plat_error_message("Uncorrectable error detected in L1/L2 cache on core %d, resetting the board", core);

	/* Reset the board after logging error */
	plat_error_handler(-ECECC);

	return 0;
}

/* Handler for uncorrectable ECC errors in the L3 cache */
static uint64_t __unused cache_l3_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie){
	plat_error_message("Uncorrectable error detected in L3 cache, resetting the board");

	/* Reset the board after logging error */
	plat_error_handler(-ECECC);

	return 0;
}

/* Handler for correctable ECC errors in the L4 memory */
static uint64_t l4_warning_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
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
		plat_error_message("Can't match int id to L4 base address.");
		return -1;
	}

	l4_warning_info(base_addr);

	return 0;
}

/* Handler for uncorrectable errors in the L4 memory */
static uint64_t __unused l4_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie){
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
		plat_error_message("Can't match int id to L4 base address.");
		return -1;
	}

	l4_error_info(base_addr);
	plat_error_handler(-ECECC);

	return 0;
}

void plat_assign_interrupt_handlers(void)
{
	/* Set up handlers for GPINT0 */
	plat_request_intr_type_el3(IRQ_GP_INTERRUPT_SYNC_0, primary_gpint_handler);

	/* Handlers for cache ECC warnings and errors */
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_0, cache_l3_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_1, cache_l1l2_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_2, cache_l1l2_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_3, cache_l1l2_fault_handler);
	plat_request_intr_type_el3(IRQ_NFAULTIRQ_4, cache_l1l2_fault_handler);

	/* Handlers for L4 cache warnings and errors */
	plat_request_intr_type_el3(IRQ_L4_ECC_WRN_INTR_0, l4_warning_handler);
	plat_request_intr_type_el3(IRQ_L4_ECC_WRN_INTR_1, l4_warning_handler);
	plat_request_intr_type_el3(IRQ_L4_ECC_WRN_INTR_2, l4_warning_handler);

	/* Handlers for DDR error and warning events */
	plat_request_intr_type_el3(IRQ_ECC_CORRECTED_ERR_INTR, ddr_ecc_corrected_err_handler);
	plat_request_intr_type_el3(IRQ_ECC_CORRECTED_ERR_INTR_FAULT, ddr_ecc_corrected_err_handler);
	plat_request_intr_type_el3(IRQ_O_ECC_UNCORRECTED_ERR_INTR, ddr_ecc_uncorrected_err_handler);
	plat_request_intr_type_el3(IRQ_O_ECC_UNCORRECTED_ERR_INTR_FAULT, ddr_ecc_uncorrected_err_handler);
	plat_request_intr_type_el3(IRQ_O_ECC_AP_ERR_INTR, ddr_ap_err_handler);
	plat_request_intr_type_el3(IRQ_O_ECC_AP_ERR_INTR_FAULT, ddr_ap_err_handler);
	plat_request_intr_type_el3(IRQ_O_DFI_INTERNAL_ERR_INTR, ddr_phy_err_handler);
	plat_request_intr_type_el3(IRQ_O_DFI_PHYUPD_ERR_INTR, ddr_phy_err_handler);
	plat_request_intr_type_el3(IRQ_O_DFI_ALERT_ERR_INTR, ddr_phy_err_handler);
	plat_request_intr_type_el3(IRQ_O_DWC_DDRPHY_INT_N, ddr_phy_err_handler);


	if (plat_get_dual_tile_enabled()) {
		/* Set up handlers for GPINT0 received on the Primary from the Secondary */
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
