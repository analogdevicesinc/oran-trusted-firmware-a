/*
 * Copyright(c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX - License - Identifier: BSD - 3 - Clause
 */
#ifndef DDR_PHY_HELPERS_H
#define DDR_PHY_HELPERS_H

#include "ddr_regmap.h"
#include "drivers/adi/adrv906x/ddr/ddr.h"
#include "drivers/adi/adrv906x/clk.h"

#define DDR_PHY_ENABLE_CLOCKS           (3)
#define DDR_PHY_MAILBOX_ACK             (1)

#define DDR_PHY_MAILBOX_STREAMING_MESSAGE (2)
#define DDR_PHY_MAILBOX_TRAINING_RUNNING (1)
#define DDR_PHY_MAILBOX_TRAINING_DONE   (0)
#define DDR_PHY_MAILBOX_MAILBOX_EMPTY   (-1)
#define DDR_PHY_MAILBOX_ACK_FAILED      (-2)
#define DDR_PHY_MAILBOX_NO_STREAMING_MESSAGE (-3)
#define DDR_PHY_MAILBOX_TOO_MANY_ARGUMENTS (-4)
#define DDR_PHY_MAILBOX_TRAINING_FAILED   (-5)

#define DDR_PHY_TRAINING_RUNNING        (0x00)
#define DDR_PHY_TRAINING_DONE           (0x07)
#define DDR_PHY_STREAMING_MESSAGE       (0x08)
#define DDR_PHY_TRAINING_FAILED  (0xFF)
#define DDR_PHY_STREAMING_MESSAGE_MASK  (0xFFFF)
#define DDR_PHY_STREAMING_MESSAGE_MAX_ARGUMENTS (32)

#define DDR_PHY_UCTWRITEPROTSHADOWMASK  (0x00000001)
#define DDR_RESETTOMICRO_MASK           (0x8)
#define DDR_STALLTOMICRO_MASK           (0x1)
#define DDR_DFIFREQRATIO                  (1)
#define DDR_DFI_PIPELINE_EMPTY    (0x30000000)
#define CDD_WR_START_ADDR 0x1500B8
#define CDD_RW_START_ADDR 0x150098
#define CDD_RR_START_ADDR 0x150060
#define CDD_WW_START_ADDR 0x150078
#define FINEDELAYMASK 0x1F
#define COARSEDELAYMASK 0x3C0

#define DDR_PHY_MAILBOX_TIMEOUT_US  3000
#define DDR_PHY_TRAINING_TIMEOUT_US 6000000

ddr_error_t update_umctl2_timing_values(uintptr_t base_addr_ctrl, ddr_pstate_t pstate);
ddr_error_t phy_override_user_input(void);
ddr_error_t phy_enable_power_and_clocks(uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint64_t freq);
ddr_error_t phy_run_pre_training(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uint64_t freq);
ddr_error_t phy_load_imem(int train_2d, ddr_pstate_t pstate, uintptr_t base_addr_phy);
ddr_error_t phy_set_dfi_clock(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_clk, ddr_pstate_data_t pstate_data);
ddr_error_t phy_load_dmem(int train_2d, ddr_pstate_t pstate, uintptr_t base_addr_phy, ddr_config_t configuration);
void        phy_enable_micro_ctrl(uintptr_t base_addr_phy);
ddr_error_t phy_wait_for_done(uintptr_t base_addr_phy, int train_2d);
ddr_error_t phy_read_msg_block(uintptr_t base_addr_phy, int train_2d, int ranks, ddr_pstate_t pstate);
ddr_error_t phy_run_post_training(void);
ddr_error_t phy_enter_mission_mode(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_clk, int train_2d, ddr_pstate_data_t last_trained, ddr_pstate_data_t default_pstate);
int phy_get_mailbox_message(uintptr_t base_addr_phy, int train_2d);

#endif /* DDR_PHY_HELPERS_H */
