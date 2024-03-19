/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdbool.h>

#include <common/debug.h>
#include <lib/mmio.h>

#include <adrv906x_def.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.h>

#include "debug_xbar_regs.h"

/**
 * adi_adrv906x_debug_xbar_set_output - Routes the selected source to the selected output in the mux
 */
bool adi_adrv906x_debug_xbar_set_output(uintptr_t base_addr, adi_adrv906x_debug_xbar_source_t source, adi_adrv906x_debug_xbar_output_t output)
{
	uintptr_t offset;

	/* Check base address */
	if ((base_addr != DEBUG_XBAR_SOURCE_CONTROL_BASE) && (base_addr != SEC_DEBUG_XBAR_SOURCE_CONTROL_BASE)) {
		ERROR("Debug Crossbar: Invalid base address 0x%lx\n", base_addr);
		return false;
	}

	/* Get register offset */
	switch (output) {
	case DEBUG_XBAR_OUT_0: offset = IDX_XBAR_SRC_CTRL_0; break;
	case DEBUG_XBAR_OUT_1: offset = IDX_XBAR_SRC_CTRL_1; break;
	case DEBUG_XBAR_OUT_2: offset = IDX_XBAR_SRC_CTRL_2; break;
	case DEBUG_XBAR_OUT_3: offset = IDX_XBAR_SRC_CTRL_3; break;
	case DEBUG_XBAR_OUT_4: offset = IDX_XBAR_SRC_CTRL_4; break;
	case DEBUG_XBAR_OUT_5: offset = IDX_XBAR_SRC_CTRL_5; break;
	case DEBUG_XBAR_OUT_6: offset = IDX_XBAR_SRC_CTRL_6; break;
	case DEBUG_XBAR_OUT_7: offset = IDX_XBAR_SRC_CTRL_7; break;
	default:
		ERROR("Debug Crossbar: Invalid output number %u\n", output);
		return false;
	}

	mmio_write_32(base_addr + offset, (source & BITM_XBAR_SRC_CTRL) << BITP_XBAR_SRC_CTRL);
	return true;
}

/**
 * adi_adrv906x_debug_xbar_set_map - Uses the provided map to configure the mux outputs
 */
bool adi_adrv906x_debug_xbar_set_map(uintptr_t base_addr, debug_xbar_map_t map)
{
	for (size_t i = 0; i < DEBUG_XBAR_OUT_COUNT; i++)
		if (!adi_adrv906x_debug_xbar_set_output(base_addr, map[i], i)) return false;

	return true;
}

/**
 * adi_adrv906x_debug_xbar_set_default_map - Uses a default map to configure the mux outputs (see debug_xbar_default_maps.h)
 */
bool adi_adrv906x_debug_xbar_set_default_map(uintptr_t base_addr, unsigned int default_map_id)
{
	unsigned int index;
	bool found = 0;

	for (size_t i = 0; i < MAX_DEFAULT_MAPPING_ID; i++) {
		if (debug_xbar_default_maps[i].id == default_map_id) {
			index = i;
			found = 1;
			break;
		}
	}

	if (!found) {
		ERROR("Debug Crossbar: Invalid default map id %u\n", default_map_id);
		return false;
	}

	return adi_adrv906x_debug_xbar_set_map(base_addr, debug_xbar_default_maps[index].map);
}

adi_adrv906x_debug_xbar_source_t adi_adrv906x_debug_xbar_get_source(uintptr_t base_addr, adi_adrv906x_debug_xbar_output_t output)
{
	uintptr_t offset;
	uint32_t value;
	adi_adrv906x_debug_xbar_source_t source;

	/* Check base address */
	if ((base_addr != DEBUG_XBAR_SOURCE_CONTROL_BASE) && (base_addr != SEC_DEBUG_XBAR_SOURCE_CONTROL_BASE)) {
		ERROR("Debug Crossbar: Invalid base address 0x%lx\n", base_addr);
		return false;
	}

	/* Get register offset */
	switch (output) {
	case DEBUG_XBAR_OUT_0: offset = IDX_XBAR_SRC_CTRL_0; break;
	case DEBUG_XBAR_OUT_1: offset = IDX_XBAR_SRC_CTRL_1; break;
	case DEBUG_XBAR_OUT_2: offset = IDX_XBAR_SRC_CTRL_2; break;
	case DEBUG_XBAR_OUT_3: offset = IDX_XBAR_SRC_CTRL_3; break;
	case DEBUG_XBAR_OUT_4: offset = IDX_XBAR_SRC_CTRL_4; break;
	case DEBUG_XBAR_OUT_5: offset = IDX_XBAR_SRC_CTRL_5; break;
	case DEBUG_XBAR_OUT_6: offset = IDX_XBAR_SRC_CTRL_6; break;
	case DEBUG_XBAR_OUT_7: offset = IDX_XBAR_SRC_CTRL_7; break;
	default:
		ERROR("Debug Crossbar: Invalid output number %u\n", output);
		return false;
	}

	value = mmio_read_32(base_addr + offset);
	source = (value >> BITP_XBAR_SRC_CTRL) & BITM_XBAR_SRC_CTRL;

	return source;
}
