/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_ADRV906X_CLKRST_DEF_H__
#define __ADI_ADRV906X_CLKRST_DEF_H__

#define BOOT_INFO       0xA0020
#define BOOT_MODE_MASK  0x0000000F
#define CLK_RST_CTRL_CFG        0x01000
#define CLK_RST_WARM_RST_CTRL   0x01050
#define CLK_SEL_MASK    0x00000400
#define DEVCLK_GOOD_MASK        0x01000000
#define GPINT_WARM_RST_EN_MASK  0x00000001
#define HOST_BOOT_OFFSET        0x01048
#define HOST_BOOT_READY_MASK    0x00000001
#define SECONDARY_CTRL_OFFSET   0x0104C
#define SUBSYS_GLOBAL_SW_RESET_MASK     0x00800000
#define TEST_CONTROL_MASK       0x000000F0
#define TEST_CONTROL_SHIFT      4
#define TEST_ENABLE_MASK        0x00000100
#define TEST_ENABLE_SHIFT       8


#endif /* __ADI_ADRV906X_CLKRST_DEF_H__ */
