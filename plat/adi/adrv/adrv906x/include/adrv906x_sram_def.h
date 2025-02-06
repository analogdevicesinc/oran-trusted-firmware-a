/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_ADRV906X_SRAM_DEF_H__
#define __ADI_ADRV906X_SRAM_DEF_H__

#define L4_SRAM_NUMBER_OF_BANKS 9
#define L4_SRAM_BANK_CLEAR_MASK 0x1FF

#define L4CTL_CFG_EADDR0        0x0084
#define L4CTL_CFG_EADDR1        0x008C
#define L4CTL_CFG_ERRADDR0      0x0040
#define L4CTL_CFG_ET0   0x0080
#define L4CTL_CFG_ET1   0x0088
#define L4CTL_CFG_EWADDR0       0x00A0
#define L4CTL_CFG_SADR  0x00F0
#define L4CTL_CFG_SCNT  0x00F4
#define L4CTL_CFG_SCTL  0x00EC
#define L4CTL_CFG_SCTL_SCRUB_ENABLE_MASK        0x40000000
#define L4CTL_CFG_STAT  0x0010
#define L4CTL_CFG_STAT_ECCERR0_SHIFT    8
#define L4CTL_CFG_STAT_ECCWRN0_SHIFT    17
#define L4CTL_CFG_STAT_ERR0_MASK        0x00000001
#define L4CTL_CFG_STAT_ERR1_MASK        0x00000002

#endif /* __ADI_ADRV906X_SRAM_DEF_H__ */
