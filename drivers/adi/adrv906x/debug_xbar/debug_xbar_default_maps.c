/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.h>

debug_xbar_default_map_t debug_xbar_default_maps[] = {
	{
		.id = BRINGUP_MAPPING,
		.name = "Bringup map",
		.map ={
			DEBUG_XBAR_SRC_I_ROSC_DIV_CLK,          // output 0
			DEBUG_XBAR_SRC_I_DEVICE_REF_CLK,        // output 1
			DEBUG_XBAR_SRC_DBG_BUS_OUT_MCS_CLK_IND  // output 2
			// ...
		}
	}
};
