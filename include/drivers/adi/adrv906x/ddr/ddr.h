/*
 * Copyright(c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX - License - Identifier: BSD - 3 - Clause
 */
#ifndef DDR_H
#define DDR_H

#include <stdint.h>

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

typedef enum {
	DDR_INIT_FULL,
	DDR_PRE_RESET_INIT,
	DDR_REMAP_INIT,
	DDR_POST_RESET_INIT
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

ddr_error_t ddr_basic_mem_test(uintptr_t base_addr_ddr, int size, bool restore);
ddr_error_t ddr_extensive_mem_test(uintptr_t base_addr_ddr, int size);
ddr_error_t ddr_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, uint32_t ddr_size, uint32_t ddr_remap_size, uint8_t ddr_dfi_pad_sequence[], uint8_t ddr_phy_pad_sequence[], ddr_init_stages_t stage, ddr_config_t configuration, bool ecc);
ddr_error_t ddr_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_post_reset_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_ate_test(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t ate_fw_addr, uintptr_t ate_msg_blk_addr, uint32_t ate_fw_size, uint32_t ate_msg_blk_size);
ddr_error_t ddr_custom_training_test(uintptr_t base_addr_phy, uint8_t hdt_ctrl, uint16_t sequence_ctrl, int train_2d);

/* Debug-only functions */
void ddr_mux_set_output(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint8_t group, uint8_t instance, uint8_t source);

#endif /* DDR_H */
