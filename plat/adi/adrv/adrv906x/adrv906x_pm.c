/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat_pm.h>

/*******************************************************************************
 * ADRV906x handlers and setup functions
 ******************************************************************************/
plat_psci_ops_t *plat_adrv_psci_override_pm_ops(plat_psci_ops_t *ops)
{
	ops->pwr_domain_off = NULL;             // ADRV906x does not support switching off CPUs.
	ops->pwr_domain_pwr_down_wfi = NULL;    // ADRV906x does not support switching off CPUs.
	ops->cpu_standby = NULL;                // Standby is managed directly by the kernel, using calls to the WFI instruction, without any dependency on the PSCI interface.
	ops->validate_power_state = NULL;       // This is used by the PSCI CPU_SUSPEND call. As the kernel does not uses the PSCI interface for CPU_SUSPEND, this function is not required.
	return ops;
}
