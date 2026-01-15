/*
 * Copyright (c) 2026, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>

#include <drivers/adi/adrv906x/ddr/ddr.h>
#include <drivers/delay_timer.h>

#include <lib/mmio.h>
#include <string.h>

#include "ddr_regmap.h"

static ddr_perf_run_details_t ddr_perf_run_details[DDR_PERF_EVENT_COUNTERS_TOTAL] = { 0 };
static bool ddr_perf_run_is_active = false;
static ddr_perf_callback_t *ddr_perf_callback = NULL;
static void *ddr_perf_userdata = NULL;

static bool ddr_perf_set_data_sel(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_data_sel_t sel, ddr_perf_data_qual_t qual)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = (sel << PERF_MON_DATA_SELECT_SHIFT) & PERF_MON_DATA_SELECT_MASK;
	if (sel < DDR_PERF_DATA_SEL_MULTIBIT_OFFSET) {
		cfg = (sel << PERF_MON_DATA_SELECT_SHIFT) & PERF_MON_DATA_SELECT_MASK;
	} else {
		cfg = (DDR_PERF_DATA_SEL_MULTIBIT_OFFSET << PERF_MON_DATA_SELECT_SHIFT) & PERF_MON_DATA_SELECT_MASK;
		cfg |= ((sel - DDR_PERF_DATA_SEL_MULTIBIT_OFFSET) << PERF_MON_MULTIBIT_DATA_SELECT_SHIFT) & PERF_MON_MULTIBIT_DATA_SELECT_MASK;
		cfg |= (qual << PERF_MON_DATA_QUALIFIER_SELECT_SHIFT) & PERF_MON_DATA_QUALIFIER_SELECT_MASK;
	}
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_PERF_MON_DATA_SELECT0 + (index * 4), cfg);

	return true;
}

static bool ddr_perf_get_data_select(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_data_sel_t *sel, ddr_perf_data_qual_t *qual)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_PERF_MON_DATA_SELECT0 + (index * 4));
	if (sel) {
		*sel = (cfg & PERF_MON_DATA_SELECT_MASK) >> PERF_MON_DATA_SELECT_SHIFT;
		if (*sel == DDR_PERF_DATA_SEL_MULTIBIT_OFFSET) {
			*sel = DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + ((cfg & PERF_MON_MULTIBIT_DATA_SELECT_MASK) >> PERF_MON_MULTIBIT_DATA_SELECT_SHIFT);
			if (qual)
				*qual = (cfg & PERF_MON_DATA_QUALIFIER_SELECT_MASK) >> PERF_MON_DATA_QUALIFIER_SELECT_SHIFT;
		}
	}

	return true;
}

static bool ddr_perf_set_bus_config(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_bus_sel_t bus, ddr_perf_compare_type_t compare_type, uint64_t compare_value, uint64_t enable_value)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = (compare_type << DFI_PERF_MON_BUS_COMPARE_TYPE_SHIFT) & DFI_PERF_MON_BUS_COMPARE_TYPE_MASK;
	cfg |= (bus << DFI_PERF_MON_BUS_SELECT_SHIFT) & DFI_PERF_MON_BUS_SELECT_MASK;
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_00 + (index * 4), cfg);
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_10 + (index * 4), compare_value & DFI_PERF_MON_BUS_COMPARE_VALUE_LOW_MASK);
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_20 + (index * 4), (compare_value >> 32) & DFI_PERF_MON_BUS_COMPARE_VALUE_HIGH_MASK);
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_30 + (index * 4), enable_value & DFI_PERF_MON_BUS_ENABLE_VALUE_LOW_MASK);
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_40 + (index * 4), (enable_value >> 32) & DFI_PERF_MON_BUS_ENABLE_VALUE_HIGH_MASK);

	return true;
}

static bool ddr_perf_get_bus_config(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_bus_sel_t *bus, ddr_perf_compare_type_t *compare_type, uint64_t *compare_value, uint64_t *enable_value)
{
	uint32_t cfg;
	uint32_t val_low;
	uint32_t val_high;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_00 + (index * 4));
	if (compare_type)
		*compare_type = (cfg & DFI_PERF_MON_BUS_COMPARE_TYPE_MASK) >> DFI_PERF_MON_BUS_COMPARE_TYPE_SHIFT;
	if (bus)
		*bus = (cfg & DFI_PERF_MON_BUS_SELECT_MASK) >> DFI_PERF_MON_BUS_SELECT_SHIFT;

	val_low = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_10 + (index * 4));
	val_high = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_20 + (index * 4));
	if (compare_value)
		*compare_value = ((uint64_t)val_high << 32) | val_low;

	val_low = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_30 + (index * 4));
	val_high = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_DFI_PERF_MON_BUS_CFG_40 + (index * 4));
	if (enable_value)
		*enable_value = ((uint64_t)val_high << 32) | val_low;

	return true;
}

static bool ddr_perf_set_multibit_config(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_compare_type_t compare_type, uint8_t compare_value, uint8_t enable_value)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = (compare_type << MULTIBIT_PERF_MON_COMPARE_TYPE_SHIFT) & MULTIBIT_PERF_MON_COMPARE_TYPE_MASK;
	cfg |= (enable_value << MULTIBIT_PERF_MON_ENABLE_VALUE_SHIFT) & MULTIBIT_PERF_MON_ENABLE_VALUE_MASK;
	cfg |= (compare_value << MULTIBIT_PERF_MON_COMPARE_VALUE_SHIFT) & MULTIBIT_PERF_MON_COMPARE_VALUE_MASK;
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_MULTIBIT_PERF_MON_CFG0 + (index * 4), cfg);

	return true;
}

static bool ddr_perf_get_multibit_config(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_compare_type_t *compare_type, uint8_t *compare_value, uint8_t *enable_value)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_MULTIBIT_PERF_MON_CFG0 + (index * 4));
	if (compare_type)
		*compare_type = (cfg & MULTIBIT_PERF_MON_COMPARE_TYPE_MASK) >> MULTIBIT_PERF_MON_COMPARE_TYPE_SHIFT;
	if (enable_value)
		*enable_value = (cfg & MULTIBIT_PERF_MON_ENABLE_VALUE_MASK) >> MULTIBIT_PERF_MON_ENABLE_VALUE_SHIFT;
	if (compare_value)
		*compare_value = (cfg & MULTIBIT_PERF_MON_COMPARE_VALUE_MASK) >> MULTIBIT_PERF_MON_COMPARE_VALUE_SHIFT;

	return true;
}

static bool ddr_perf_set_start_stop_mode(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_start_stop_mode_t start_mode, ddr_perf_start_stop_mode_t stop_mode)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = (start_mode << ENABLE_START_MODE_SHIFT) & ENABLE_START_MODE_MASK;
	cfg |= (stop_mode << ENABLE_STOP_MODE_SHIFT) & ENABLE_STOP_MODE_MASK;
	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_ENABLE_CONTROL_REG_10 + (index * 4), cfg);

	return true;
}

static bool ddr_perf_get_start_stop_mode(uintptr_t base_addr_ctrl, unsigned int index, ddr_perf_start_stop_mode_t *start_mode, ddr_perf_start_stop_mode_t *stop_mode)
{
	uint32_t cfg;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	cfg = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_ENABLE_CONTROL_REG_10 + (index * 4));
	if (start_mode)
		*start_mode = (cfg & ENABLE_START_MODE_MASK) >> ENABLE_START_MODE_SHIFT;
	if (stop_mode)
		*stop_mode = (cfg & ENABLE_STOP_MODE_MASK) >> ENABLE_STOP_MODE_SHIFT;

	return true;
}

static bool ddr_perf_manual_start(uintptr_t base_addr_ctrl, unsigned int index)
{
	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_ENABLE_CONTROL_REG_00 + (index * 4), SW_ENABLE_SET_MASK);

	return true;
}

static bool ddr_perf_manual_stop(uintptr_t base_addr_ctrl, unsigned int index)
{
	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_ENABLE_CONTROL_REG_00 + (index * 4), SW_ENABLE_CLR_MASK);

	return true;
}

static bool ddr_perf_get_event_count(uintptr_t base_addr_ctrl, unsigned int index, uint32_t *count)
{
	uint32_t val = 0;
	unsigned int i;

	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_EVENT_COUNTER_VALUE0 + (index * 4), EVENT_COUNT_RD_EN_MASK);
	for (i = 0; i < 100 && val == 0; i++) {
		udelay(1);
		val = mmio_read_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_EVENT_COUNTER_VALUE0 + (index * 4)) & EVENT_COUNT_MASK;
	}
	if (count)
		*count = val;

	return true;
}

bool ddr_perf_data_sel_is_multibit(ddr_perf_data_sel_t sel)
{
	return sel >= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET;
}

bool ddr_perf_is_running(void)
{
	return ddr_perf_run_is_active;
}

bool ddr_perf_set_configuration(unsigned int index, ddr_perf_run_details_t *details)
{
	if (details == NULL)
		return false;
	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;
	if (ddr_perf_data_sel_is_multibit(details->data_select))
		return false; /* not yet supported */
	if (ddr_perf_is_running())
		return false;

	memcpy(&ddr_perf_run_details[index], details, sizeof(ddr_perf_run_details_t));
	return true;
}

bool ddr_perf_get_configuration(unsigned int index, ddr_perf_run_details_t *details)
{
	if (details == NULL)
		return false;
	if (index >= DDR_PERF_EVENT_COUNTERS_TOTAL)
		return false;

	memcpy(details, &ddr_perf_run_details[index], sizeof(ddr_perf_run_details_t));
	return true;
}

bool ddr_perf_start(uintptr_t base_addr_ctrl, ddr_perf_callback_t *callback, void *userdata)
{
	unsigned int i;

	if (ddr_perf_is_running())
		return false;

	for (i = 0; i < DDR_PERF_EVENT_COUNTERS_TOTAL; i++) {
		ddr_perf_run_details_t *details = &ddr_perf_run_details[i];
		if (!details->enabled)
			continue;
		if (!ddr_perf_set_data_sel(base_addr_ctrl, i, details->data_select, QUAL_NONE))
			return false;
		if (!ddr_perf_set_multibit_config(base_addr_ctrl, i, details->multibit_compare_type, details->multibit_compare_value, details->multibit_enable_value))
			return false;
		if (!ddr_perf_set_bus_config(base_addr_ctrl, i, details->bus_select, details->bus_compare_type, details->bus_compare_value, details->bus_enable_value))
			return false;
	}

	for (i = 0; i < DDR_PERF_EVENT_COUNTERS_TOTAL; i++)
		if (!ddr_perf_set_start_stop_mode(base_addr_ctrl, i, MANUAL_MODE, MANUAL_MODE))
			return false;

	/* TODO: additional modes */

	mmio_write_32(base_addr_ctrl + DDR_PERFORMANCE_MONITOR_COUNT_RESET, EVENT_COUNT_RESET_N_MASK);

	for (i = 0; i < DDR_PERF_EVENT_COUNTERS_TOTAL; i++) {
		ddr_perf_run_details_t *details = &ddr_perf_run_details[i];
		if (!details->enabled)
			continue;
		if (!ddr_perf_manual_start(base_addr_ctrl, i))
			return false;
	}

	ddr_perf_run_is_active = true;
	ddr_perf_callback = callback;
	ddr_perf_userdata = userdata;
	return true;
}

bool ddr_perf_stop(uintptr_t base_addr_ctrl)
{
	unsigned int i;
	bool ret = true;

	if (!ddr_perf_is_running())
		return false;

	for (i = 0; i < DDR_PERF_EVENT_COUNTERS_TOTAL; i++) {
		ddr_perf_run_details_t *details = &ddr_perf_run_details[i];
		if (!details->enabled)
			continue;
		if (!ddr_perf_manual_stop(base_addr_ctrl, i))
			return false;
	}

	for (i = 0; i < DDR_PERF_EVENT_COUNTERS_TOTAL; i++) {
		uint32_t count = 0;

		ddr_perf_run_details_t *details = &ddr_perf_run_details[i];
		if (!details->enabled)
			continue;
		/* TODO: other output signals (besides count) */
		if (!ddr_perf_get_event_count(base_addr_ctrl, i, &count))
			ret = false;
		ddr_perf_callback(i, &ddr_perf_run_details[i], count, ddr_perf_userdata);
	}

	ddr_perf_run_is_active = false;
	return ret;
}
