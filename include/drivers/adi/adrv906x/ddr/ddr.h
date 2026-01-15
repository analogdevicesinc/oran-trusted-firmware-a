/*
 * Copyright(c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX - License - Identifier: BSD - 3 - Clause
 */
#ifndef DDR_H
#define DDR_H

#include <stdint.h>
#include <stdbool.h>

#define ADI_DDR_CTRL_TIMEOUT                      (20)
#define ADI_DDR_ECC_SCRUB_TIMEOUT                 (2000)
#define ADI_DDR_FREQ_DEFAULT_MHZ                  (1600)
#define DDR_MHZ_TO_HZ                             (1000000)
#define DDR_DFI_PAD_SEQUENCE_SIZE 28
#define DDR_PHY_PAD_SEQUENCE_SIZE 44
#define DDR_SIZE_0GB 0
#define DDR_SIZE_0_5GB 0x20000000
#define DDR_SIZE_1GB   0x40000000
#define DDR_SIZE_1_5GB 0x60000000
#define DDR_SIZE_2GB   0x80000000
#define DDR_SIZE_2_5GB 0xA0000000
#define DDR_SIZE_3GB   0xC0000000
#define DDR_HDTCTRL_MAX_VERBOSITY 0x04

#define DDR_PERF_EVENT_COUNTERS_TOTAL 6
#define DDR_PERF_DATA_SEL_MULTIBIT_OFFSET 52

typedef enum {
	DDR_INIT_FULL,
	DDR_PRE_RESET_INIT,
	DDR_REMAP_INIT,
	DDR_POST_RESET_INIT,
	DDR_CUSTOM_TRAINING
} ddr_init_stages_t;

typedef enum {
	DDR_PRIMARY_CONFIGURATION,
	DDR_SECONDARY_CONFIGURATION,
	DDR_NUM_CONFIGURATIONS
} ddr_config_t;

typedef enum {
	ERROR_DDR_NO_ERROR,
	ERROR_DDR_CTRL_INIT_FAILED,
	ERROR_DDR_PHY_INIT_FAILED,
	ERROR_DDR_PHY_MAILBOX_FAILED,
	ERROR_DDR_PHY_FW_FAILED,
	ERROR_DDR_BASIC_MEM_TEST_FAILED,
	ERROR_DDR_EXTENSIVE_MEM_TEST_FAILED,
	ERROR_DDR_ECC_SCRUB_FAILED
} ddr_error_t;

typedef enum {
	DEV_UNKNOWN = 0,
	DEV_X1,
	DEV_X2,
	DEV_X4,
	DEV_X8,
	DEV_X16,
	DEV_X32,
	DEV_X64
} ddr_dtype_t;

typedef enum {
	MEM_EMPTY = 0,
	MEM_RESERVED,
	MEM_UNKNOWN,
	MEM_FPM,
	MEM_EDO,
	MEM_BEDO,
	MEM_SDR,
	MEM_RDR,
	MEM_DDR,
	MEM_RDDR,
	MEM_RMBS,
	MEM_DDR2,
	MEM_FB_DDR2,
	MEM_RDDR2,
	MEM_XDR,
	MEM_DDR3,
	MEM_RDDR3,
	MEM_LRDDR3,
	MEM_LPDDR3,
	MEM_DDR4,
	MEM_RDDR4,
	MEM_LRDDR4,
	MEM_LPDDR4,
	MEM_DDR5,
	MEM_RDDR5,
	MEM_LRDDR5,
	MEM_NVDIMM,
	MEM_WIO2,
	MEM_HBM2,
	MEM_HBM3,
} ddr_mtype_t;

typedef struct {
	uint8_t rank;
	uint8_t row;
	uint8_t bank_group;
	uint8_t bank;
	uint16_t block;
	uint8_t corrected_bit_num;
	uint16_t error_count;
} ddr_ecc_error_data_t;

typedef enum {
	DDR_MASTER0,
	DDR_ANIB,
	DDR_DBYTE,
} ddr_mux_group_t;

typedef enum { /* Entries in this enumeration should not be rearranged, DDR_PSTATE0 always == 0, etc.*/
	DDR_PSTATE0
} ddr_pstate_t;

typedef struct {
	ddr_pstate_t pstate;
	int freq;
} ddr_pstate_data_t;

typedef struct {
	uint32_t ard_ptr_init_val;              /* Address/Command FIFO RdPtr Initial Value */
	uint32_t data_tx_impedance_ctrl;        /* Data TX Impedance */
	uint32_t data_tx_odt_drive_strength;    /* Data TX ODT Driver Strength Control */
	uint32_t master_cal_rate;               /* Impedance Calibration Rate Control */
} ddr_custom_values_t;

typedef enum {
	WAQ_POP_0				= 0,
	WAQ_PUSH_0				= 1,
	WAQ_SPLIT_0				= 2,
	RAQ_POP_0				= 3,
	RAQ_PUSH_0				= 4,
	RAQ_SPLIT_0				= 5,
	WAQ_POP_1				= 6,
	WAQ_PUSH_1				= 7,
	WAQ_SPLIT_1				= 8,
	RAQ_POP_1				= 9,
	RAQ_PUSH_1				= 10,
	RAQ_SPLIT_1				= 11,
	PERF_HIF_RD_OR_WR			= 12,
	PERF_HIF_WR				= 13,
	PERF_HIF_RD				= 14,
	PERF_HIF_RMW				= 15,
	PERF_HIF_HI_PRI_RD			= 16,
	PERF_DFI_WR_DATA_CYCLES			= 17,
	PERF_DFI_RD_DATA_CYCLES			= 18,
	PERF_HPR_XACT_WHEN_CRITICAL		= 19,
	PERF_LPR_XACT_WHEN_CRITICAL		= 20,
	PERF_WR_XACT_WHEN_CRITICAL		= 21,
	PERF_OP_IS_ACTIVATE			= 22,
	PERF_OP_IS_RD_OR_WR			= 23,
	PERF_OP_IS_RD_ACTIVATE			= 24,
	PERF_OP_IS_RD				= 25,
	PERF_OP_IS_WR				= 26,
	PERF_OP_IS_PRECHARGE			= 27,
	PERF_PRECHARGE_FOR_RDWR			= 28,
	PERF_PRECHARGE_FOR_OTHER		= 29,
	PERF_RDWR_TRANSITIONS			= 30,
	PERF_WRITE_COMBINE			= 31,
	PERF_WRITE_COMBINE_NOECC		= 32,
	PERF_WRITE_COMBINE_WRECC		= 33,
	PERF_WAR_HAZARD				= 34,
	PERF_RAW_HAZARD				= 35,
	PERF_WAW_HAZARD				= 36,
	PERF_IE_BLK_HAZARD			= 37,
	PERF_OP_IS_ENTER_SELFREF		= 38,
	PERF_OP_IS_ENTER_POWERDOWN		= 39,
	PERF_OP_IS_ENTER_MPSM			= 40,
	PERF_SELFREF_MODE			= 41,
	PERF_OP_IS_REFRESH			= 42,
	PERF_OP_IS_CRIT_REF			= 43,
	PERF_OP_IS_SPEC_REF			= 44,
	PERF_OP_IS_LOAD_MODE			= 45,
	PERF_OP_IS_ZQCL				= 46,
	PERF_OP_IS_ZQCS				= 47,
	PERF_HPR_REQ_WITH_NOCREDIT		= 48,
	PERF_LPR_REQ_WITH_NOCREDIT		= 49,
	PERF_VISIBLE_WINDOW_LIMIT_REACHED_RD	= 50,
	PERF_VISIBLE_WINDOW_LIMIT_REACHED_WR	= 51,
	WAQ_WCOUNT_0				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 0,
	RAQ_WCOUNT_0				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 1,
	WAQ_WCOUNT_1				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 2,
	RAQ_WCOUNT_1				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 3,
	PERF_RANK				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 4,
	PERF_BANK				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 5,
	PERF_BG					= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 6,
	DBG_DFI_IE_WR_CMD_TYPE			= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 7,
	DBG_DFI_IE_RD_CMD_TYPE			= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 8,
	LPR_CREDIT_CNT				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 9,
	HPR_CREDIT_CNT				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 10,
	WR_CREDIT_CNT				= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 11,
	WRECC_CREDIT_CNT			= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET + 12,
} ddr_perf_data_sel_t;

typedef enum {
	PERFMON_BUS	= 0,
	DFI_1_BUS	= 1,
	DFI_2_BUS	= 2,
} ddr_perf_bus_sel_t;

typedef enum {
	QUAL_NONE	= 0,
	OP_IS_RD_OR_WR	= 1,
	OP_IS_RD	= 2,
	OP_IS_WR	= 3,
} ddr_perf_data_qual_t;

typedef enum {
	MANUAL_MODE,
	TIMER_MODE,
	TRU_MODE,
	SATURATION_MODE, /* stop only */
} ddr_perf_start_stop_mode_t;

typedef enum {
	COMPARE_MATCH		= 0,
	COMPARE_GREATER_THAN	= 1,
} ddr_perf_compare_type_t;

typedef struct {
	bool enabled;
	ddr_perf_data_sel_t data_select;
	ddr_perf_data_qual_t multibit_qualifier;        /* only used if data_select >= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET */
	ddr_perf_compare_type_t multibit_compare_type;  /* only used if data_select >= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET */
	uint8_t multibit_compare_value;                 /* only used if data_select >= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET */
	uint8_t multibit_enable_value;                  /* only used if data_select >= DDR_PERF_DATA_SEL_MULTIBIT_OFFSET */
	ddr_perf_bus_sel_t bus_select;
	ddr_perf_compare_type_t bus_compare_type;
	uint64_t bus_compare_value;
	uint64_t bus_enable_value;
} ddr_perf_run_details_t;

typedef void ddr_perf_callback_t(unsigned int index, ddr_perf_run_details_t *details, uint32_t count, void *userdata);

ddr_error_t ddr_basic_mem_test(uintptr_t base_addr_ddr, uint32_t size, bool restore);
ddr_error_t ddr_extensive_mem_test(uintptr_t base_addr_ddr, uint32_t size);
ddr_error_t ddr_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, uint32_t ddr_size, uint32_t ddr_remap_size, uint8_t ddr_dfi_pad_sequence[], uint8_t ddr_phy_pad_sequence[], ddr_init_stages_t stage, ddr_config_t configuration, bool ecc);
ddr_error_t ddr_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_post_reset_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_init_stages_t stage, ddr_config_t configuration);
ddr_error_t ddr_ate_test(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t ate_fw_addr, uintptr_t ate_msg_blk_addr, uint32_t ate_fw_size, uint32_t ate_msg_blk_size);
ddr_error_t ddr_custom_training_test(uintptr_t base_addr_phy, uint8_t hdt_ctrl, uint16_t sequence_ctrl, int train_2d);

/* DDR ECC reporting functions */
bool ddr_get_ecc_enabled_state(uintptr_t base_addr_ctrl);
uint32_t ddr_get_address_map_offset(uintptr_t base_addr_ctrl, unsigned int bank);
ddr_dtype_t ddr_get_dtype(uintptr_t base_addr_ctrl);
ddr_mtype_t ddr_get_mtype(uintptr_t base_addr_ctrl);
bool ddr_get_ecc_error_info(uintptr_t base_addr_ctrl, bool correctable, ddr_ecc_error_data_t *data);
bool ddr_get_ecc_syndrome_mask(uintptr_t base_addr_ctrl, uint32_t *syndrome);
void ddr_clear_ap_error(uintptr_t base_addr_ctrl);
void ddr_set_data_inject_poison(uintptr_t base_addr_ctrl, unsigned int rank, unsigned int col, unsigned int bank_grp, unsigned int bank, unsigned int row);
void ddr_get_data_inject_poison(uintptr_t base_addr_ctrl, unsigned int *rank, unsigned int *col, unsigned int *bank_grp, unsigned int *bank, unsigned int *row);
void ddr_set_data_poison_config(uintptr_t base_addr_ctrl, bool enable, bool double_error);
void ddr_get_data_poison_config(uintptr_t base_addr_ctrl, bool *enable, bool *double_error);

/* DDR Performance Monitors */
bool ddr_perf_data_sel_is_multibit(ddr_perf_data_sel_t sel);
bool ddr_perf_is_running(void);
bool ddr_perf_set_configuration(unsigned int index, ddr_perf_run_details_t *details);
bool ddr_perf_get_configuration(unsigned int index, ddr_perf_run_details_t *details);
bool ddr_perf_start(uintptr_t base_addr_ctrl, ddr_perf_callback_t *callback, void *userdata);
bool ddr_perf_stop(uintptr_t base_addr_ctrl);

/* Debug-only functions */
void ddr_mux_set_output(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint8_t group, uint8_t instance, uint8_t source);

/* Optional functions used for updating training parameters with board specific values
 *  Functions expected to be defined in board specific files if utilized */
bool ddr_check_for_custom_parameters(void);
void ddr_set_custom_parameters(ddr_custom_values_t *values);
void ddr_board_custom_pre_training(uintptr_t base_addr_phy);

#endif /* DDR_H */
