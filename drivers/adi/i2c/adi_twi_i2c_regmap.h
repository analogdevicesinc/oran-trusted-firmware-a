/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_TWI_I2C_REGMAP_INTERNAL_H__
#define __ADI_TWI_I2C_REGMAP_INTERNAL_H__

#define TWI_CLKDIV      0x00
#define TWI_CTL 0x04
#define TWI_CTL_EN      0x0080
#define TWI_FIFOCTL     0x28
#define TWI_FIFOCTL_RXFLUSH     0x0002
#define TWI_FIFOCTL_TXFLUSH     0x0001
#define TWI_FIFOSTAT    0x2C
#define TWI_IMSK        0x24
#define TWI_ISTAT       0x20
#define TWI_ISTAT_MCOMP 0x0010
#define TWI_ISTAT_MERR  0x0020
#define TWI_ISTAT_RXSERV        0x0080
#define TWI_ISTAT_TXSERV        0x0040
#define TWI_MSTRADDR    0x1C
#define TWI_MSTRCTL     0x14
#define TWI_MSTRCTL_DCNT_OFFSET 6
#define TWI_MSTRCTL_DIR 0x0004
#define TWI_MSTRCTL_EN  0x0001
#define TWI_MSTRCTL_FAST        0x0008
#define TWI_MSTRCTL_RSTART      0x0020
#define TWI_MSTRCTL_STOP        0x0010
#define TWI_MSTRSTAT    0x18
#define TWI_MSTRSTAT_BUSBUSY    0x0100
#define TWI_RXDATA16    0x8C
#define TWI_RXDATA8     0x88
#define TWI_SLVADDR     0x10
#define TWI_SLVCTL      0x08
#define TWI_SLVSTAT     0x0C
#define TWI_TXDATA16    0x84
#define TWI_TXDATA8     0x80


#endif /* __ADI_TWI_I2C_REGMAP_INTERNAL_H__ */
