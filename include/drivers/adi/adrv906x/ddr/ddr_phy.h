/*
 * Copyright(c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX - License - Identifier: BSD - 3 - Clause
 */
#ifndef DDR_PHY_H
#define DDR_PHY_H

#include "drivers/adi/adrv906x/ddr/ddr.h"

ddr_error_t ddr_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_config_t configuration);

#endif /* DDR_PHY_H */
