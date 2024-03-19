/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_EL3_INT_HANDLERS_H
#define ADRV906X_EL3_INT_HANDLERS_H

uint64_t primary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);
uint64_t secondary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);

#endif /* ADRV906X_EL3_INT_HANDLERS_H */
