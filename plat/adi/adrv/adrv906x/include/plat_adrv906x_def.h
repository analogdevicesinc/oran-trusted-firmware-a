/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_ADRV906X_DEF_H
#define PLAT_ADRV906X_DEF_H

#include "adrv906x_def.h"

#define PLATFORM_CACHE_LINE_SIZE        64

#define PLATFORM_CLUSTER_COUNT          U(1)
#define PLATFORM_CORE_COUNT_PER_CLUSTER U(4)
#define PLATFORM_CORE_COUNT             (PLATFORM_CLUSTER_COUNT * \
					 PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLAT_MAX_PWR_LVL                MPIDR_AFFLVL1
#define PLAT_NUM_PWR_DOMAINS            (PLATFORM_CORE_COUNT + \
					 PLATFORM_CLUSTER_COUNT + 1)

#define PLAT_MAX_RET_STATE              U(1)
#define PLAT_MAX_OFF_STATE              U(2)

#define PLAT_PRIMARY_CPU_ID             (0x0)

/* GIC addresses */
#define PLAT_GICD_BASE                  GIC_BASE
#define PLAT_GICR_BASE                  (GIC_BASE + 0x00040000)

/* Misc platform addresses */
#define PLAT_TSGEN_BASE                 TSGEN_BASE
#define ADRV906X_SYSC_EMMC_BASE           EMMC_0_BASE
#define ADRV906X_SYSC_SD_BASE             SD_0_BASE

/*
 * Console and UART related constants
 */
#ifndef PLAT_BOOT_UART_BASE
#define PLAT_BOOT_UART_BASE             PL011_0_BASE    /* UART0 */
#endif
#ifndef PLAT_RUN_UART_BASE
#define PLAT_RUN_UART_BASE              PL011_1_BASE            /* UART1 (secure UART) */
#endif
#define PLAT_CRASH_UART_BASE            PLAT_RUN_UART_BASE      /* Same as PLAT_RUN_UART_BASE */

/*
 * Physical memory regions
 */
#define SRAM_BASE                       UL(0x00000000)
#define SRAM_SIZE                       UL(0x00600000)                  /* 6 MB */
#define DRAM_BASE                       UL(0x40000000)
#define DRAM_SIZE_MIN                   UL(0x10000000)                  /* 256MB TODO: Define this */
#define SEC_SRAM_BASE                   UL(0x04000000)
#define SEC_SRAM_SIZE                   UL(0x00600000)                  /* 6 MB */
#define SEC_DRAM_SIZE_MIN               UL(0x08000000)                  /* 128MB TODO: Define this */
#define MAX_DDR_SIZE                    UL(0xC0000000)                  /* 3GB Max for DDR combined*/
#define MAX_SECONDARY_DDR_SIZE          UL(0x60000000)                  /* 1.5GB MAX for secondary tile DDR*/

/*
 * Non-secure memory regions
 */
#define NS_SRAM_BASE                       UL(0x00100000)
#define NS_SRAM_SIZE                       UL(0x00400000)                       /* 4 MB */
#define SEC_NS_SRAM_BASE                   UL(0x04100000)
#define SEC_NS_SRAM_SIZE                   UL(0x00400000)                       /* 4 MB */

/*
 * Device addresses
 */
#define DEVICE0_BASE                    UL(0x20000000)
#define DEVICE0_SIZE                    UL(0x04000000)
#define DEVICE1_BASE                    UL(0x24000000)
#define DEVICE1_SIZE                    UL(0x04000000)
#define DEVICE2_BASE                    UL(0x18000000)
#define DEVICE2_SIZE                    UL(0x02000000)
#define DEVICE3_BASE                    UL(0x28000000)
#define DEVICE3_SIZE                    UL(0x04000000)
#define SEC_DEVICE2_BASE                UL(0x1C000000)
#define SEC_DEVICE2_SIZE                UL(0x02000000)
#define SEC_DEVICE3_BASE                UL(0x2C000000)
#define SEC_DEVICE3_SIZE                UL(0x04000000)
#ifdef TEST_FRAMEWORK
// Used to create mem map for DDR as one device for use in test framework code
#define TEST_DRAM_BASE                  UL(0x40000000)
#define TEST_DRAM_SIZE                  UL(0x20000000)
#endif
/*
 * Platform specific page table and MMU setup constants
 */
#define PLAT_VIRT_ADDR_SPACE_SIZE       (1ULL << 32)
#define PLAT_PHY_ADDR_SPACE_SIZE        (1ULL << 32)

#define PLAT_XLAT_TABLES_DYNAMIC 1
#define MAX_XLAT_TABLES         8
#define MAX_MMAP_REGIONS                16

#define CACHE_WRITEBACK_SHIFT           6
#define CACHE_WRITEBACK_GRANULE         (1 << CACHE_WRITEBACK_SHIFT)

/*
 * Max MTD Devices
 */
#define MAX_IO_MTD_DEVICES              U(1)

/*
 * Default clock frequencies
 */
/* ROSC freq varies. Default to highest (worst-case) freq. */
#define ROSC_FREQ_DFLT 400000000U
#ifdef TEST_FRAMEWORK
#define DEVCLK_FREQ_DFLT 983040000U
#else
#define DEVCLK_FREQ_DFLT 245760000U
#endif

/*
 * SPI-Flash setup
 */
#define QSPI_FLASH_CHIP_SELECT       1
#define QSPI_CLK_FREQ_HZ           (50U * 1000U * 1000U)

/*
 * EMMC defines
 */
#define EMMC_0_CLK_RATE_HZ         (26U * 1000U * 1000U)
#define EMMC_0_BUS_WIDTH           MMC_BUS_WIDTH_8

/*
 * SD defines
 */
#define SD_0_CLK_RATE_HZ           (25U * 1000U * 1000U)
#define SD_0_BUS_WIDTH             MMC_BUS_WIDTH_4

#define ADI_SDEI_SGI_PRIVATE  (8)
#include "adrv906x_gicv3_def.h"

/* Include ADI-generic definitions */
/* MUST BE INCLUDED LAST */
#include <plat_common_def.h>

#endif /* PLAT_ADRV906X_DEF_H */
