/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADRV906X_DDR_H__
#define __ADRV906X_DDR_H__

#include <drivers/adi/adrv906x/ddr/ddr.h>

int adrv906x_ddr_init(void);
int adrv906x_ddr_ate_test(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk);

/* Debug-only functions */
int adrv906x_ddr_extensive_mem_test(uintptr_t base_addr_ddr, uint32_t size);
int adrv906x_ddr_mem_test(uintptr_t base_addr_ddr, uint32_t size, bool restore);
int adrv906x_ddr_custom_training_test(uintptr_t base_addr_phy, uint16_t sequence_ctrl, int train_2d);
void adrv906x_ddr_set_custom_parameters(ddr_custom_values_t *values);
void adrv906x_ddr_mux_set_output(uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint8_t group, uint8_t instance, uint8_t source);
int adrv906x_ddr_iterative_init_pre_reset(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, ddr_config_t configuration);
int adrv906x_ddr_iterative_init_post_reset(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, ddr_config_t configuration);
int adrv906x_ddr_iterative_init_remapping(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uintptr_t base_addr_ddr, ddr_config_t configuration);

bool adrv906x_ddr_check_ecc_errors(uintptr_t base_addr_ctrl);
void adrv906x_ddr_clear_ap_error(uintptr_t base_addr_ctrl);
bool adrv906x_ddr_log_correctable_error(uintptr_t base_addr_ctrl);
bool adrv906x_ddr_log_uncorrectable_error(uintptr_t base_addr_ctrl);

#endif /* __ADRV906X_DDR_H__ */
