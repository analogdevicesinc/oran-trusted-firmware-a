/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <platform_def.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include "utils.h"


/**
 *******************************************************************************
 * Function: utils_floorLog2U32
 *
 * @brief       compute floor(log2(x)), where 'x' is uint32_t.
 *
 * @details
 *
 * Parameters:
 * @param [in/out]  x  -  Value to be converted
 *
 * @return          floor_log2_u32(x)
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
uint32_t utils_floorLog2U32(uint32_t x)
{
	uint32_t temp = 0;

	temp = __builtin_clz(x);
	return temp;
}
