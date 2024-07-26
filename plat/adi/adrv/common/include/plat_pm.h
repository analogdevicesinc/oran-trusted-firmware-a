/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLAT_PM_H__
#define __PLAT_PM_H__

#include <lib/psci/psci.h>

plat_psci_ops_t *plat_adrv_psci_override_pm_ops(plat_psci_ops_t *ops);

#endif /* __PLAT_PM_H__ */
