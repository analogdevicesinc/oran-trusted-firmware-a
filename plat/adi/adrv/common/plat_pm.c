/*
 * Copyright (c) 2015-2024, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <platform.h>

#include <plat_board.h>
#include <plat_err.h>
#include <plat_helpers.h>
#include <plat_int_gicv3.h>
#include <plat_mailbox.h>
#include <plat_pm.h>
#include <plat_ras.h>
#include <plat_status_reg.h>

/*******************************************************************************
 * Platform handler called when a power domain is about to be turned on. The
 * mpidr determines the CPU to be turned on.
 ******************************************************************************/
static int plat_pwr_domain_on(u_register_t mpidr)
{
	int cpu_id = plat_core_pos_by_mpidr(mpidr);
	uintptr_t hold_base = PLAT_TM_HOLD_BASE;
	unsigned int pos = PLATFORM_CORE_COUNT;

	if (cpu_id < 0)
		return PSCI_E_INTERN_FAIL;

	pos = (unsigned int)cpu_id;

	/* Find the requested CPU's HOLD_STATE register, then set it to 'GO' */
	hold_base += pos * PLAT_TM_HOLD_ENTRY_SIZE;
	mmio_write_64(hold_base, PLAT_TM_HOLD_STATE_GO);

	/* No cache maintenance here, as hold_base is mapped as device memory. */

	/* Make sure that the write has completed */
	dsb();
	isb();

	/* Wake up the core */
	sev();

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Platform handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
static void plat_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	/* Program GIC per-cpu distributor or re-distributor interface */
	plat_gic_pcpu_init();

	/* Enable GIC CPU interface */
	plat_gic_cpuif_enable();

	/* Enable ECC on CPU caches */
	plat_enable_cache_ecc();
}

/*******************************************************************************
 * Platform handler called to check the validity of the non secure
 * entrypoint. Returns PSCI_E_SUCCESS if the entrypoint is valid, or
 * PSCI_E_INVALID_ADDRESS otherwise.
 ******************************************************************************/
int plat_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	if ((entrypoint >= NS_DRAM_BASE) && (entrypoint <
					     (NS_DRAM_BASE + NS_DRAM_SIZE_MIN)))
		return PSCI_E_SUCCESS;

	return PSCI_E_INVALID_ADDRESS;
}

/*******************************************************************************
 * Platform handler called to perform a soft reset
 ******************************************************************************/
static int plat_system_reset2(int is_vendor, int reset_type, u_register_t cookie)
{
	/* Clear the reset cause registers on a "normal" shutdown */
	if (plat_rd_status_reg(RESET_CAUSE) == WATCHDOG_RESET)
		plat_wr_status_reg(RESET_CAUSE, WARM_RESET);
	if (plat_rd_status_reg(RESET_CAUSE_NS) == WATCHDOG_RESET)
		plat_wr_status_reg(RESET_CAUSE_NS, WARM_RESET);

	NOTICE("Warm reset command received\n");

	console_flush();
	return plat_warm_reset();
}

/*******************************************************************************
 * Platform handlers and setup function.
 ******************************************************************************/
static plat_psci_ops_t plat_psci_pm_ops = {
	.pwr_domain_on		= plat_pwr_domain_on,
	.pwr_domain_on_finish	= plat_pwr_domain_on_finish,
	.validate_ns_entrypoint = plat_validate_ns_entrypoint,
	.system_reset2		= plat_system_reset2,
};

/* Allow boards to override psci operations */
#pragma weak plat_board_psci_override_pm_ops
plat_psci_ops_t *plat_board_psci_override_pm_ops(plat_psci_ops_t *ops)
{
	return ops;
}

/* Allow adrv device to override psci operations */
#pragma weak plat_adrv_psci_override_pm_ops
plat_psci_ops_t *plat_adrv_psci_override_pm_ops(plat_psci_ops_t *ops)
{
	return ops;
}

/* Board level reset  */
#pragma weak plat_board_system_reset
void __dead2 plat_board_system_reset(void)
{
	plat_error_message("TODO implement System Reset");
	console_flush();
	panic();
}

/*******************************************************************************
 * Setup PSCI ops.
 * NOTE: This is declared as "non-optimized" because if PLAT_TM_ENTRYPOINT is
 * 0 (mailbox address is 0), gcc will translate this to a brk #0x3e8 instruction.
 * This is a trap for a potential divide-by-zero, which is definitely a false
 * positive here.
 ******************************************************************************/
int __init __attribute__((optimize("O0"))) plat_setup_psci_ops(uintptr_t sec_entrypoint,
							       const plat_psci_ops_t **psci_ops)
{
	uintptr_t *entrypoint = (void *)PLAT_TM_ENTRYPOINT;
	plat_psci_ops_t *aux_ops;

	*entrypoint = sec_entrypoint;
	aux_ops = plat_adrv_psci_override_pm_ops(&plat_psci_pm_ops);
	*psci_ops = plat_board_psci_override_pm_ops(aux_ops);

	return 0;
}
