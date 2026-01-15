/*
 * Copyright (c) 2025, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/runtime_svc.h>
#include <common/debug.h>
#include <lib/smccc.h>

#include <plat_ddr_edac.h>
#include <plat_ddr_edac_svc.h>
#include <plat_sip_svc.h>
#include <plat_err.h>
#include <platform_def.h>

#define SMC_DDR_EDAC_ERR 1

typedef enum {
	GET_CORRECTABLE_ERROR_INFO = 0,
	GET_UNCORRECTABLE_ERROR_INFO,
	GET_SYNDROME_MASK,
	GET_DTYPE,
	GET_MTYPE,
	GET_ECC_STATE,
	GET_ADDRESS_MAP_OFFSET,
	GET_POISON_CONFIG,
	SET_POISON_CONFIG,
	SET_INJECT_POISON,
	GET_INJECT_POISON,
	SET_PERF_MON_CORE_ENABLED,
	GET_PERF_MON_CORE_ENABLED,
	START_PERF_MON,
	STOP_PERF_MON,
	SET_PERF_MON_CORE_DATA_SEL,
	GET_PERF_MON_CORE_DATA_SEL,
	SET_PERF_MON_CORE_BUS_SELECT,
	GET_PERF_MON_CORE_BUS_SELECT,
	SET_PERF_MON_CORE_BUS_COMPARE,
	GET_PERF_MON_CORE_BUS_COMPARE,
	SET_PERF_MON_CORE_BUS_ENABLE,
	GET_PERF_MON_CORE_BUS_ENABLE,
	SET_PERF_MON_CORE_MULTIBIT_QUALIFIER,
	GET_PERF_MON_CORE_MULTIBIT_QUALIFIER,
	SET_PERF_MON_CORE_MULTIBIT_COMPARE,
	GET_PERF_MON_CORE_MULTIBIT_COMPARE,
	SET_PERF_MON_CORE_MULTIBIT_ENABLE,
	GET_PERF_MON_CORE_MULTIBIT_ENABLE,
} func_id_t;

/*
 * DDR EDAC service SMC handler
 */
uintptr_t plat_ddr_edac_smc_handler(unsigned int smc_fid,
				    u_register_t x1,
				    u_register_t x2,
				    u_register_t x3,
				    u_register_t x4,
				    void *cookie,
				    void *handle,
				    u_register_t flags)
{
	func_id_t id;
	uint32_t count;
	uint32_t syndrome[3] = { 0 };
	struct ecc_error_info info;
	struct ecc_poison_config poison_cfg;
	bool enabled;
	enum error_type error_type;
	unsigned int perf_val;
	uint64_t perf_val64;
	static unsigned int perf_counts[6];

	id = (func_id_t)x1;
	switch (id) {
	case GET_CORRECTABLE_ERROR_INFO:
		plat_ddr_edac_get_error_info(DDR_CTL_BASE, DDR_ERROR_CORRECTABLE, &count, &info);
		SMC_RET5(handle, SMC_OK, count, ((info.bankgrpnr & 0x3) << 30) | ((info.bank & 0x3) << 28) | ((info.blknr & 0x3FF) << 18) | (info.row & 0x3FFFF), info.bitpos, info.data);
		break;
	case GET_UNCORRECTABLE_ERROR_INFO:
		plat_ddr_edac_get_error_info(DDR_CTL_BASE, DDR_ERROR_UNCORRECTABLE, &count, &info);
		SMC_RET5(handle, SMC_OK, count, ((info.bankgrpnr & 0x3) << 30) | ((info.bank & 0x3) << 28) | ((info.blknr & 0x3FF) << 18) | (info.row & 0x3FFFF), info.bitpos, info.data);
		break;
	case GET_SYNDROME_MASK:
		plat_ddr_edac_get_syndrome_mask(DDR_CTL_BASE, syndrome);
		SMC_RET4(handle, SMC_OK, syndrome[0], syndrome[1], syndrome[2]);
		break;
	case GET_DTYPE:
		SMC_RET2(handle, SMC_OK, plat_ddr_edac_get_dtype(DDR_CTL_BASE));
		break;
	case GET_MTYPE:
		SMC_RET2(handle, SMC_OK, plat_ddr_edac_get_mtype(DDR_CTL_BASE));
		break;
	case GET_ECC_STATE:
		SMC_RET2(handle, SMC_OK, plat_ddr_edac_get_ecc_state(DDR_CTL_BASE));
		break;
#if DEBUG == 1
	case GET_ADDRESS_MAP_OFFSET:
		SMC_RET2(handle, SMC_OK, plat_ddr_edac_get_address_map_offset(DDR_CTL_BASE, (unsigned int)x2));
		break;
	case GET_POISON_CONFIG:
		if (!plat_ddr_edac_get_poison_config(DDR_CTL_BASE, &enabled, &error_type))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET3(handle, SMC_OK, enabled, error_type);
		break;
	case SET_POISON_CONFIG:
		if (!plat_ddr_edac_set_poison_config(DDR_CTL_BASE, (bool)x2, (enum error_type)x3))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case SET_INJECT_POISON:
		poison_cfg.rank = (uint32_t)(x2 >> 16) & 0xFFFF;
		poison_cfg.col = (uint32_t)x2 & 0xFFFF;
		poison_cfg.bank_grp = (uint32_t)(x3 >> 16) & 0xFFFF;
		poison_cfg.bank = (uint32_t)x3 & 0xFFFF;
		poison_cfg.row = (uint32_t)x4;
		if (!plat_ddr_edac_set_inject_poison(DDR_CTL_BASE, &poison_cfg))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_INJECT_POISON:
		if (!plat_ddr_edac_get_inject_poison(DDR_CTL_BASE, &poison_cfg))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET4(handle, SMC_OK, poison_cfg.rank << 16 | (poison_cfg.col & 0xFFFF), poison_cfg.bank_grp << 16 | (poison_cfg.bank & 0xFFFF), poison_cfg.row);
		break;
	case SET_PERF_MON_CORE_ENABLED:
		if (!plat_ddr_edac_set_perf_mon_core_enabled((unsigned int)x2, (bool)x3))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_ENABLED:
		if (!plat_ddr_edac_get_perf_mon_core_enabled((unsigned int)x2, &enabled))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET2(handle, SMC_OK, enabled);
		break;
	case START_PERF_MON:
		memset(perf_counts, 0, sizeof(perf_counts));
		if (!plat_ddr_edac_start_perf_mon(DDR_PERF_MON_BASE, perf_counts))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case STOP_PERF_MON:
		if (!plat_ddr_edac_stop_perf_mon(DDR_PERF_MON_BASE))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET7(handle, SMC_OK, perf_counts[0], perf_counts[1], perf_counts[2], perf_counts[3], perf_counts[4], perf_counts[5]);
		break;
	case SET_PERF_MON_CORE_DATA_SEL:
		if (!plat_ddr_edac_set_perf_mon_core_data_sel((unsigned int)x2, (unsigned int)x3))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_DATA_SEL:
		if (!plat_ddr_edac_get_perf_mon_core_data_sel((unsigned int)x2, &perf_val))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET2(handle, SMC_OK, perf_val);
		break;
	case SET_PERF_MON_CORE_BUS_SELECT:
		if (!plat_ddr_edac_set_perf_mon_core_bus_select((unsigned int)x2, (unsigned int)x3))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_BUS_SELECT:
		if (!plat_ddr_edac_get_perf_mon_core_bus_select((unsigned int)x2, &perf_val))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET2(handle, SMC_OK, perf_val);
		break;
	case SET_PERF_MON_CORE_BUS_COMPARE:
		if (!plat_ddr_edac_set_perf_mon_core_bus_compare((unsigned int)x2, (unsigned int)(x3 >> 16), ((uint64_t)(x3 & 0xFFFF) << 32) | (uint64_t)x4))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_BUS_COMPARE:
		if (!plat_ddr_edac_get_perf_mon_core_bus_compare((unsigned int)x2, &perf_val, &perf_val64))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET3(handle, SMC_OK, (uint32_t)((perf_val << 16) | (perf_val64 >> 32)), (uint32_t)perf_val64);
		break;
	case SET_PERF_MON_CORE_BUS_ENABLE:
		if (!plat_ddr_edac_set_perf_mon_core_bus_enable((unsigned int)x2, ((uint64_t)(x3) << 32) | (uint64_t)x4))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_BUS_ENABLE:
		if (!plat_ddr_edac_get_perf_mon_core_bus_enable((unsigned int)x2, &perf_val64))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET3(handle, SMC_OK, (uint32_t)(perf_val64 >> 32), (uint32_t)perf_val64);
		break;
	case SET_PERF_MON_CORE_MULTIBIT_QUALIFIER:
		if (!plat_ddr_edac_set_perf_mon_core_multibit_qualifier((unsigned int)x2, (unsigned int)x3))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_MULTIBIT_QUALIFIER:
		if (!plat_ddr_edac_get_perf_mon_core_multibit_qualifier((unsigned int)x2, &perf_val))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET2(handle, SMC_OK, perf_val);
		break;
	case SET_PERF_MON_CORE_MULTIBIT_COMPARE:
		if (!plat_ddr_edac_set_perf_mon_core_multibit_compare((unsigned int)x2, (unsigned int)(x3 >> 16), ((uint64_t)(x3 & 0xFFFF) << 32) | (uint64_t)x4))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_MULTIBIT_COMPARE:
		if (!plat_ddr_edac_get_perf_mon_core_multibit_compare((unsigned int)x2, &perf_val, &perf_val64))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET3(handle, SMC_OK, (uint32_t)((perf_val << 16) | (perf_val64 >> 32)), (uint32_t)perf_val64);
		break;
	case SET_PERF_MON_CORE_MULTIBIT_ENABLE:
		if (!plat_ddr_edac_set_perf_mon_core_multibit_enable((unsigned int)x2, ((uint64_t)(x3) << 32) | (uint64_t)x4))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET1(handle, SMC_OK);
		break;
	case GET_PERF_MON_CORE_MULTIBIT_ENABLE:
		if (!plat_ddr_edac_get_perf_mon_core_multibit_enable((unsigned int)x2, &perf_val64))
			SMC_RET1(handle, SMC_DDR_EDAC_ERR);
		SMC_RET3(handle, SMC_OK, (uint32_t)(perf_val64 >> 32), (uint32_t)perf_val64);
		break;
#endif
	default:
		break;
	}

	plat_runtime_warn_message("DDR EDAC service: Unexpected command");
	SMC_RET1(handle, SMC_UNK);
}
