/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_ADRV906X_DEBUG_XBAR_DEFAULT_MAPS_H__
#define __ADI_ADRV906X_DEBUG_XBAR_DEFAULT_MAPS_H__

#include "debug_xbar_defs.h"

typedef enum {
	BRINGUP_MAPPING,
	MAX_DEFAULT_MAPPING_ID
} default_map_id_t;

extern debug_xbar_default_map_t debug_xbar_default_maps[];

#endif /* __ADI_ADRV906X_DEBUG_XBAR_DEFAULT_MAPS_H__ */
