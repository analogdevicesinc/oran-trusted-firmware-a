/*
 * Copyright (c) 2022, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_BOOTCTRL_H
#define PLAT_BOOTCTRL_H

void plat_bootctrl_init(uintptr_t dev_handle, uintptr_t spec, uint32_t reset_cause, uint32_t reset_cause_ns);
char plat_bootctrl_get_active_slot(void);

#endif /* PLAT_BOOTCTRL_H */
