/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_DDR_EDAC_H
#define PLAT_DDR_EDAC_H

#include <inttypes.h>
#include <stdbool.h>

enum error_type {
	DDR_ERROR_CORRECTABLE = 0,
	DDR_ERROR_UNCORRECTABLE,
};

struct ecc_error_info {
	uint32_t row;
	uint32_t col;
	uint32_t bank;
	uint32_t bitpos;
	uint32_t data;
	uint32_t bankgrpnr;
	uint32_t blknr;
};

struct ecc_poison_config {
	uint32_t rank;
	uint32_t col;
	uint32_t bank_grp;
	uint32_t bank;
	uint32_t row;
};

/* ECC */
void plat_ddr_edac_get_error_info(const uintptr_t baseaddr, enum error_type type, uint32_t *count, struct ecc_error_info *info);
void plat_ddr_edac_get_syndrome_mask(const uintptr_t baseaddr, uint32_t *syndrome);
uint32_t plat_ddr_edac_get_dtype(const uintptr_t baseaddr);
uint32_t plat_ddr_edac_get_mtype(const uintptr_t baseaddr);
uint32_t plat_ddr_edac_get_ecc_state(const uintptr_t baseaddr);
uint32_t plat_ddr_edac_get_address_map_offset(const uintptr_t baseaddr, unsigned int bank);
bool plat_ddr_edac_set_poison_config(const uintptr_t baseaddr, bool enable, enum error_type type);
bool plat_ddr_edac_get_poison_config(const uintptr_t baseaddr, bool *enable, enum error_type *type);
bool plat_ddr_edac_set_inject_poison(const uintptr_t baseaddr, struct ecc_poison_config *config);
bool plat_ddr_edac_get_inject_poison(const uintptr_t baseaddr, struct ecc_poison_config *config);

/* performance monitoring */
bool plat_ddr_edac_set_perf_mon_core_enabled(unsigned int index, bool enabled);
bool plat_ddr_edac_get_perf_mon_core_enabled(unsigned int index, bool *enabled);
bool plat_ddr_edac_set_perf_mon_core_data_sel(unsigned int index, unsigned int data_sel);
bool plat_ddr_edac_get_perf_mon_core_data_sel(unsigned int index, unsigned int *data_sel);
bool plat_ddr_edac_set_perf_mon_core_bus_select(unsigned int index, unsigned int bus_sel);
bool plat_ddr_edac_get_perf_mon_core_bus_select(unsigned int index, unsigned int *bus_sel);
bool plat_ddr_edac_set_perf_mon_core_bus_compare(unsigned int index, unsigned int compare_type, uint64_t compare_value);
bool plat_ddr_edac_get_perf_mon_core_bus_compare(unsigned int index, unsigned int *compare_type, uint64_t *compare_value);
bool plat_ddr_edac_set_perf_mon_core_bus_enable(unsigned int index, uint64_t enable_val);
bool plat_ddr_edac_get_perf_mon_core_bus_enable(unsigned int index, uint64_t *enable_val);
bool plat_ddr_edac_set_perf_mon_core_multibit_qualifier(unsigned int index, unsigned int qualifier);
bool plat_ddr_edac_get_perf_mon_core_multibit_qualifier(unsigned int index, unsigned int *qualifier);
bool plat_ddr_edac_set_perf_mon_core_multibit_compare(unsigned int index, unsigned int compare_type, uint64_t compare_value);
bool plat_ddr_edac_get_perf_mon_core_multibit_compare(unsigned int index, unsigned int *compare_type, uint64_t *compare_value);
bool plat_ddr_edac_set_perf_mon_core_multibit_enable(unsigned int index, uint64_t enable_val);
bool plat_ddr_edac_get_perf_mon_core_multibit_enable(unsigned int index, uint64_t *enable_val);
bool plat_ddr_edac_start_perf_mon(const uintptr_t baseaddr, unsigned int *counts);
bool plat_ddr_edac_stop_perf_mon(const uintptr_t baseaddr);

#endif /* PLAT_DDR_EDAC_H */
