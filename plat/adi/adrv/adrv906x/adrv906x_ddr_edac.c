/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <plat_ddr_edac.h>
#include <drivers/adi/adrv906x/ddr/ddr.h>

#include <lib/mmio.h>
#include <lib/utils_def.h>

void plat_ddr_edac_get_error_info(const uintptr_t baseaddr, enum error_type type, uint32_t *count, struct ecc_error_info *info)
{
	ddr_ecc_error_data_t ecc_data = { 0 };

	ddr_get_ecc_error_info(baseaddr, (type == DDR_ERROR_CORRECTABLE) ? true : false, &ecc_data);
	if (count)
		*count = ecc_data.error_count;
	if (info) {
		info->row = ecc_data.row;
		info->col = 0;
		info->bank = ecc_data.bank;
		info->bitpos = ecc_data.corrected_bit_num;
		info->data = 0; /* TODO */
		info->bankgrpnr = ecc_data.bank_group;
		info->blknr = ecc_data.block;
	}
}

void plat_ddr_edac_get_syndrome_mask(const uintptr_t baseaddr, uint32_t *syndrome)
{
	ddr_get_ecc_syndrome_mask(baseaddr, syndrome);
}

uint32_t plat_ddr_edac_get_dtype(const uintptr_t baseaddr)
{
	return (uint32_t)ddr_get_dtype(baseaddr);
}

uint32_t plat_ddr_edac_get_mtype(const uintptr_t baseaddr)
{
	return (uint32_t)ddr_get_mtype(baseaddr);
}

uint32_t plat_ddr_edac_get_ecc_state(const uintptr_t baseaddr)
{
	return ddr_get_ecc_enabled_state(baseaddr) ? 1 : 0;
}

uint32_t plat_ddr_edac_get_address_map_offset(const uintptr_t baseaddr, unsigned int bank)
{
	return ddr_get_address_map_offset(baseaddr, bank);
}

bool plat_ddr_edac_set_poison_config(const uintptr_t baseaddr, bool enable, enum error_type type)
{
	ddr_set_data_poison_config(baseaddr, enable, (type == DDR_ERROR_UNCORRECTABLE) ? true : false);
	return true;
}

bool plat_ddr_edac_get_poison_config(const uintptr_t baseaddr, bool *enable, enum error_type *type)
{
	bool double_err;

	ddr_get_data_poison_config(baseaddr, enable, &double_err);
	if (type)
		*type = double_err ? DDR_ERROR_UNCORRECTABLE : DDR_ERROR_CORRECTABLE;
	return true;
}

bool plat_ddr_edac_set_inject_poison(const uintptr_t baseaddr, struct ecc_poison_config *config)
{
	ddr_set_data_inject_poison(baseaddr, config->rank, config->col, config->bank_grp, config->bank, config->row);
	return true;
}

bool plat_ddr_edac_get_inject_poison(const uintptr_t baseaddr, struct ecc_poison_config *config)
{
	ddr_get_data_inject_poison(baseaddr, &config->rank, &config->col, &config->bank_grp, &config->bank, &config->row);
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_enabled(unsigned int index, bool enabled)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.enabled = enabled;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_enabled(unsigned int index, bool *enabled)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (enabled)
		*enabled = details.enabled;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_data_sel(unsigned int index, unsigned int data_sel)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.data_select = (ddr_perf_data_sel_t)data_sel;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_data_sel(unsigned int index, unsigned int *data_sel)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (data_sel)
		*data_sel = (unsigned int)details.data_select;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_bus_select(unsigned int index, unsigned int bus_sel)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.bus_select = (ddr_perf_bus_sel_t)bus_sel;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_bus_select(unsigned int index, unsigned int *bus_sel)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (bus_sel)
		*bus_sel = (unsigned int)details.bus_select;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_bus_compare(unsigned int index, unsigned int compare_type, uint64_t compare_value)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.bus_compare_type = (ddr_perf_compare_type_t)compare_type;
	details.bus_compare_value = compare_value;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_bus_compare(unsigned int index, unsigned int *compare_type, uint64_t *compare_value)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (compare_type)
		*compare_type = (unsigned int)details.bus_compare_type;
	if (compare_value)
		*compare_value = details.bus_compare_value;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_bus_enable(unsigned int index, uint64_t enable_val)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.bus_enable_value = enable_val;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_bus_enable(unsigned int index, uint64_t *enable_val)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (enable_val)
		*enable_val = details.bus_enable_value;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_multibit_qualifier(unsigned int index, unsigned int qualifier)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.multibit_qualifier = (ddr_perf_data_qual_t)qualifier;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_multibit_qualifier(unsigned int index, unsigned int *qualifier)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (qualifier)
		*qualifier = (unsigned int)details.multibit_qualifier;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_multibit_compare(unsigned int index, unsigned int compare_type, uint64_t compare_value)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.multibit_compare_type = (ddr_perf_compare_type_t)compare_type;
	details.multibit_compare_value = compare_value;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_multibit_compare(unsigned int index, unsigned int *compare_type, uint64_t *compare_value)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (compare_type)
		*compare_type = (unsigned int)details.multibit_compare_type;
	if (compare_value)
		*compare_value = details.multibit_compare_value;
	return true;
}

bool plat_ddr_edac_set_perf_mon_core_multibit_enable(unsigned int index, uint64_t enable_val)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	details.multibit_enable_value = enable_val;
	return ddr_perf_set_configuration(index, &details);
}

bool plat_ddr_edac_get_perf_mon_core_multibit_enable(unsigned int index, uint64_t *enable_val)
{
	ddr_perf_run_details_t details;

	if (!ddr_perf_get_configuration(index, &details))
		return false;
	if (enable_val)
		*enable_val = details.multibit_enable_value;
	return true;
}

static void plat_ddr_edac_perf_callback(unsigned int index, ddr_perf_run_details_t *details, uint32_t count, void *userdata)
{
	unsigned int *counts = (unsigned int *)userdata;

	if (counts && index < DDR_PERF_EVENT_COUNTERS_TOTAL)
		counts[index] = count;
}

bool plat_ddr_edac_start_perf_mon(const uintptr_t baseaddr, unsigned int *counts)
{
	return ddr_perf_start(baseaddr, plat_ddr_edac_perf_callback, (void *)counts);
}

bool plat_ddr_edac_stop_perf_mon(const uintptr_t baseaddr)
{
	return ddr_perf_stop(baseaddr);
}
