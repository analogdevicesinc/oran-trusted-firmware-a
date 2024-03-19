/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_ADRV906X_DEBUG_XBAR_H__
#define __ADI_ADRV906X_DEBUG_XBAR_H__

#include "debug_xbar_defs.h"

bool adi_adrv906x_debug_xbar_set_output(uintptr_t base_addr, adi_adrv906x_debug_xbar_source_t source, adi_adrv906x_debug_xbar_output_t output);
bool adi_adrv906x_debug_xbar_set_map(uintptr_t base_addr, debug_xbar_map_t map);
bool adi_adrv906x_debug_xbar_set_default_map(uintptr_t base_addr, unsigned int default_map_id);

adi_adrv906x_debug_xbar_source_t adi_adrv906x_debug_xbar_get_source(uintptr_t base_addr, adi_adrv906x_debug_xbar_output_t output);

#endif /* __ADI_ADRV906X_DEBUG_XBAR_H__ */
