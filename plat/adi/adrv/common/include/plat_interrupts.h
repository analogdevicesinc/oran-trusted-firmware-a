/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_INTERRUPTS_H
#define PLAT_INTERRUPTS_H
#include <errno.h>

#include <bl31/interrupt_mgmt.h>

#include <platform.h>

#define MAX_INTR_EL3    1024

int plat_request_intr_type_el3(uint32_t id, interrupt_type_handler_t handler);
#if EL3_EXCEPTION_HANDLING
int plat_interrupt_handler(uint32_t intr_raw, uint32_t flags, void *handle, void *cookie);
#else
uint64_t plat_interrupt_handler(uint32_t intr_raw, uint32_t flags, void *handle, void *cookie);
#endif

#endif /* PLAT_INTERRUPTS_H */
