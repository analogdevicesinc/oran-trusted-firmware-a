/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_EL3_INT_HANDLERS_H
#define ADRV906X_EL3_INT_HANDLERS_H

void plat_assign_interrupt_handlers(void);
uint64_t primary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t secondary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t cache_l1l2_fault_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t cache_l3_fault_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t cache_l1l2_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t cache_l3_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t l4_error_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t l4_warning_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);

#endif /* ADRV906X_EL3_INT_HANDLERS_H */
