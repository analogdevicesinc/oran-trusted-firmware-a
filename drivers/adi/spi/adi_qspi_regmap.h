/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_QSPI_REGMAP_INTERNAL_H__
#define __ADI_QSPI_REGMAP_INTERNAL_H__

#define SPI_CLK 0x10
#define SPI_CTL 0x04
#define SPI_CTL_ASSEL   0x00000040
#define SPI_CTL_CPHA    0x00000010
#define SPI_CTL_CPOL    0x00000020
#define SPI_CTL_EN      0x00000001
#define SPI_CTL_FMODE   0x00040000
#define SPI_CTL_LSBF    0x00001000
#define SPI_CTL_MIOM_MASK       0x00300000
#define SPI_CTL_MIOM_MIO_DIS    0x00000000
#define SPI_CTL_MIOM_MIO_DUAL   0x00000001
#define SPI_CTL_MIOM_MIO_QUAD   0x00000002
#define SPI_CTL_MIOM_OFFSET     20
#define SPI_CTL_MSTR    0x00000002
#define SPI_CTL_SELST   0x00000080
#define SPI_CTL_SIZE_MASK       0x00000600
#define SPI_CTL_SIZE_OFFSET     9
#define SPI_CTL_SIZE_SIZE32     0x00000002
#define SPI_CTL_SOSI    0x00400000
#define SPI_DLY 0x14
#define SPI_ILAT        0x44
#define SPI_ILAT_CLR    0x48
#define SPI_IMSK        0x30
#define SPI_IMSK_CLR    0x34
#define SPI_IMSK_SET    0x38
#define SPI_RFIFO       0x50
#define SPI_RWC 0x1C
#define SPI_RWCR        0x20
#define SPI_RWC_MASK    0x0000FFFF
#define SPI_RXCTL       0x08
#define SPI_RXCTL_RDR_MASK      0x00000070
#define SPI_RXCTL_RDR_NE        0x00000001
#define SPI_RXCTL_RDR_OFFSET    4
#define SPI_RXCTL_REN   0x00000001
#define SPI_RXCTL_RTI   0x00000004
#define SPI_RXCTL_RWCEN 0x00000008
#define SPI_SLVSEL      0x18
#define SPI_SLVSEL_SSEL_OFFSET  9
#define SPI_SLVSEL_SSE_OFFSET   1
#define SPI_STAT        0x40
#define SPI_STAT_RF     0x00000400
#define SPI_STAT_RFE    0x00400000
#define SPI_STAT_RFS_RFS_MASK   0x00007000
#define SPI_STAT_TF     0x00000800
#define SPI_STAT_TFF    0x00800000
#define SPI_STAT_TFS_MASK       0x00070000
#define SPI_TFIFO       0x58
#define SPI_TWC 0x24
#define SPI_TWCR        0x28
#define SPI_TWC_MASK    0x0000FFFF
#define SPI_TXCTL       0x0C
#define SPI_TXCTL_TDR_MASK      0x00000070
#define SPI_TXCTL_TDR_NF        0x00000001
#define SPI_TXCTL_TDR_OFFSET    4
#define SPI_TXCTL_TEN   0x00000001
#define SPI_TXCTL_TTI   0x00000004
#define SPI_TXCTL_TWCEN 0x00000008


#endif /* __ADI_QSPI_REGMAP_INTERNAL_H__ */
