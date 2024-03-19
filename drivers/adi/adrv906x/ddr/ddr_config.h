/*
 * Copyright(c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DDR_CONFIG_H
#define DDR_CONFIG_H

#include <stdbool.h>
#include <drivers/adi/adrv906x/ddr/ddr.h>

typedef struct {
	ddr_error_t (*pre_reset_function)(uintptr_t base_addr_ctrl, bool ecc);
	ddr_error_t (*phy_function)(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
} ddr_function_config_t;

ddr_error_t ddr_4gb_2rank_x8_2gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_2rank_x8_2gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_4gb_2rank_x16_1gbx16_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_2rank_x16_1gbx16_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_4gb_2rank_x16_1gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_2rank_x16_1gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_4gb_1rank_x16_2gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_1rank_x16_2gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x16_1gbx16_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x16_1gbx16_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x8_2gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x8_2gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_2rank_x8_1gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_2rank_x8_1gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_multi_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_multi_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_2rank_x8_2gbx8_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_2rank_x8_2gbx8_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);

ddr_error_t ddr_4gb_2rank_x8_2gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_2rank_x8_2gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_4gb_2rank_x16_1gbx16_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_2rank_x16_1gbx16_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_4gb_2rank_x16_1gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_2rank_x16_1gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_4gb_1rank_x16_2gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_4gb_1rank_x16_2gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x16_1gbx16_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x16_1gbx16_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x8_2gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x8_2gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x8_1gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_2rank_x8_1gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_multi_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_1rank_x16_1gbx8_multi_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);
ddr_error_t ddr_2gb_2rank_x8_2gbx8_1600_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);
ddr_error_t ddr_2gb_2rank_x8_2gbx8_1600_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc);

extern ddr_function_config_t ddr_function_configurations[DDR_NUM_CONFIGURATIONS];

#endif /* DDR_CONFIG_H */
