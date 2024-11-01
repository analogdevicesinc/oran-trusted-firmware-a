/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_QSPI_DDE_REGMAP_INTERNAL_H__
#define __ADI_QSPI_DDE_REGMAP_INTERNAL_H__

#define DDE_ADDRSTART   0x004
#define DDE_ADDR_CUR    0x02C
#define DDE_BWLCNT      0x040
#define DDE_BWLCNT_CUR  0x044
#define DDE_BWMCNT      0x048
#define DDE_BWMCNT_CUR  0x04C
#define DDE_CFG 0x008
#define DDE_CFG_CADDR   0x00000008
#define DDE_CFG_DESCIDCPY       0x02000000
#define DDE_CFG_EN      0x00000001
#define DDE_CFG_FLOW    0x00007000
#define DDE_CFG_FLOW_OFFSET     12
#define DDE_CFG_INT     0x00300000
#define DDE_CFG_INT_OFFSET      20
#define DDE_CFG_MSIZE   0x00000700
#define DDE_CFG_MSIZE_OFFSET    8
#define DDE_CFG_NDSIZE  0x00070000
#define DDE_CFG_NDSIZE_OFFSET   16
#define DDE_CFG_PDRF    0x10000000
#define DDE_CFG_PSIZE   0x00000070
#define DDE_CFG_PSIZE_OFFSET    4
#define DDE_CFG_SYNC    0x00000004
#define DDE_CFG_TOVEN   0x01000000
#define DDE_CFG_TRIG    0x00C00000
#define DDE_CFG_TRIG_OFFSET     22
#define DDE_CFG_TWAIT   0x00008000
#define DDE_CFG_TWOD    0x04000000
#define DDE_CFG_WNR     0x00000002
#define DDE_DSCPTR_CUR  0x024
#define DDE_DSCPTR_NXT  0x000
#define DDE_DSCPTR_PRV  0x028
#define DDE_DSCPTR_PRV_PDPO     0x00000001
#define DDE_STAT        0x030
#define DDE_STAT_ERRC   0x00000070
#define DDE_STAT_FIFOFILL       0x00070000
#define DDE_STAT_IRQDONE        0x00000001
#define DDE_STAT_IRQERR 0x00000002
#define DDE_STAT_MBWID  0x0000C000
#define DDE_STAT_PBWID  0x00003000
#define DDE_STAT_PIRQ   0x00000004
#define DDE_STAT_RUN    0x00000700
#define DDE_STAT_RUN_OFFSET     8
#define DDE_STAT_RUN_STOPPED    0x00000000
#define DDE_STAT_TWAIT  0x00100000
#define DDE_XCNT        0x00C
#define DDE_XCNT_CUR    0x034
#define DDE_XMOD        0x010
#define DDE_YCNT        0x014
#define DDE_YCNT_CUR    0x038
#define DDE_YMOD        0x018


#endif /* __ADI_QSPI_DDE_REGMAP_INTERNAL_H__ */