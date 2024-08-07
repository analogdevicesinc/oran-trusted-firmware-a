/*
 * Copyright (c) 2015-2024, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_COMMON_DEF_H
#define PLAT_COMMON_DEF_H

#include <arch.h>
#include <common/tbbr/tbbr_img_def.h>
#include <plat/common/common_def.h>
#include <lib/utils_def.h>

/*
 * Generic platform constants
 */

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE             0x1000

#define FIRMWARE_WELCOME_STR            "Booting Trusted Firmware\n"

#define MAX_IO_DEVICES                  4
#define MAX_IO_HANDLES                  4
#define MAX_IO_BLOCK_DEVICES            U(2)

/* Boot partition related constants */
#define BOOTCTRL_PARTITION_NAME "bootctrl"
#define BOOTCFG_PARTITION_NAME "bootcfg"
#define FIP_PARTITION_NAME_SIZE 6               /* fip_a/fip_b + null */
#define FIP_PARTITION_BASE_NAME "fip_"

/*
 * Bootctrl related constants
 */
#define BOOTCTRL_BOOT_CNT_THRESHOLD     4       /* Maximum number of boot attempts per slot */
#define BOOTCTRL_ACTIVE_SLOT_START      'a'     /* Starting slot */
#define BOOTCTRL_ACTIVE_SLOT_LAST       'b'     /* Highest available slot */

/*
 * Console and UART related constants
 */
#define PLAT_CONSOLE_BAUDRATE           115200

/*
 * Platform memory map related constants
 */

/*
 * Shared memory regions
 */
#define SHARED_RAM_BASE                 (SRAM_BASE)                     /* Place shared memory at beginning of SRAM */
#define SHARED_RAM_SIZE                 UL(0x1000)                      /* 4KB shared memory region */

/*
 * TEE memory regions
 * Composed of two regions:
 * 1) A secure memory region for the TEE itself
 * 2) A non-secure shared region for use by the REE and the TEE
 */
#define TEE_DRAM_BASE                   (DRAM_BASE)
#define TEE_DRAM_SIZE                   UL(0x02000000)                  /* 32MB */
#define SECURE_DRAM_BASE                (TEE_DRAM_BASE)
#define SECURE_DRAM_SIZE                UL(0x01E00000)                  /* 30MB */
#define TEE_SHMEM_BASE                  (SECURE_DRAM_BASE + SECURE_DRAM_SIZE)
#define TEE_SHMEM_SIZE                  UL(0x00200000)                  /* 2MB */

/*
 * Non-secure memory regions
 */
#define NS_DRAM_BASE                    (TEE_DRAM_BASE + TEE_DRAM_SIZE)         /* Place NS DRAM directly after secure DRAM */
#define NS_DRAM_SIZE_MIN                (DRAM_SIZE_MIN - TEE_DRAM_SIZE)         /* Minimum NS DRAM size is the remaining DRAM after secure DRAM */

/*
 * BL generic defines.
 */
#define BL_RAM_SIZE                     UL(0x00100000)
#define BL_RAM_BASE                     (SRAM_BASE + SRAM_SIZE - BL_RAM_SIZE)     /* BL stages have last 1MB of SRAM available */

/*
 * FW_CONFIG defines.
 */
#ifndef FW_CONFIG_MAX_SIZE
#define FW_CONFIG_MAX_SIZE              UL(0x2000)                                      /* 8KB max for FW_CONFIG */
#endif
#define FW_CONFIG_BASE                  (BL_RAM_BASE + sizeof(meminfo_t))               /* Place FW_CONFIG at the beginning of BL_RAM, after BL2 meminfo */
#define FW_CONFIG_LIMIT                 (FW_CONFIG_BASE + FW_CONFIG_MAX_SIZE)

/*
 * Bootcfg defines
 */
#ifndef BOOTCFG_MAX_SIZE
#define BOOTCFG_MAX_SIZE                UL(0x2000)                              /* 8KB max for bootcfg */
#endif
#define BOOTCFG_BASE                    (FW_CONFIG_BASE + FW_CONFIG_MAX_SIZE)   /* Place bootcfg after FW_CONFIG */
#define BOOTCFG_LIMIT                   (BOOTCFG_BASE + BOOTCFG_MAX_SIZE)

/*
 * HW_CONFIG defines.
 */
#ifndef HW_CONFIG_MAX_SIZE
#define HW_CONFIG_MAX_SIZE              UL(0x2000)                                      /* 8KB max for HW_CONFIG */
#endif
#define HW_CONFIG_BASE                  (NS_DRAM_BASE)                                  /* Place HW_CONFIG at the beginning of NS_DRAM */
#define HW_CONFIG_LIMIT                 (HW_CONFIG_BASE + HW_CONFIG_MAX_SIZE)

/*
 * BL1 specific defines.
 */
#ifndef BL1_RO_MAX_SIZE
#define BL1_RO_MAX_SIZE         UL(0x21000)                                             /* 128KB max for BL1 RO */
#endif
#ifndef BL1_RW_MAX_SIZE
#define BL1_RW_MAX_SIZE         UL(0x18000)                                             /* 96KB max for BL1 RW */
#endif
#define BL1_RO_BASE                     (BL_RAM_BASE + BL_RAM_SIZE - BL1_RO_MAX_SIZE)   /* Place BL1 RO at the end of BL_RAM */
#define BL1_RO_LIMIT                    (BL1_RO_BASE + BL1_RO_MAX_SIZE)
#define BL1_RW_BASE                     (BL1_RO_BASE - BL1_RW_MAX_SIZE)                 /* Place BL1 RW directly before BL1 RO */
#define BL1_RW_LIMIT                    (BL1_RW_BASE + BL1_RW_MAX_SIZE)

/*
 * BL2 specific defines.
 */
#ifndef BL2_MAX_SIZE
#define BL2_MAX_SIZE                    UL(0x71000)                             /* 452 KB max for BL2 */
#endif
#define BL2_BASE                        (BL1_RW_BASE - BL2_MAX_SIZE)            /* Place BL2 directly before BL1 RW */
#define BL2_LIMIT                       (BL2_BASE + BL2_MAX_SIZE)

/*
 * BL31 specific defines.
 */
/* TODO: Enable BL31 to overwrite BL2
 * Also need to update plat_get_next_bl_params() to copy bl_params out to shared RAM, like the Arm platform does.
 */
#ifdef ADI_BL31_OVERWRITES_BL2
#ifndef BL31_MAX_SIZE
#define BL31_MAX_SIZE                   (BL1_RW_MAX_SIZE + BL2_MAX_SIZE)        /* BL31 can overwrite BL2 and BL1_RW */
#endif
#define BL31_BASE                       (BL1_RW_BASE - BL31_MAX_SIZE)
#define BL31_LIMIT                      (BL31_BASE + BL1_RW_MAX_SIZE + BL2_MAX_SIZE)
#else
#ifndef BL31_MAX_SIZE
#define BL31_MAX_SIZE                   UL(0x30000)                               /* 192 KB max for BL31 */
#endif
#define BL31_BASE                       (BL2_BASE - BL31_MAX_SIZE)
#define BL31_LIMIT                      (BL31_BASE + BL31_MAX_SIZE)
#endif

/*
 * BL32 specific defines.
 */
#define BL32_MAX_SIZE                   (SECURE_DRAM_SIZE)                      /* 32MB max for BL32 */
#define BL32_BASE                       (SECURE_DRAM_BASE)                      /* Place BL32 at the start of secure DRAM */
#define BL32_LIMIT                      (BL32_BASE + BL32_MAX_SIZE)

/*
 * BL33 specific defines.
 */
#define BL33_BASE                       (DRAM_BASE + UL(0x8000000))
#define BL33_LIMIT                      (DRAM_BASE + DRAM_SIZE_MIN)
#define BL33_MAX_SIZE                   (BL33_LIMIT - BL33_BASE)

/*
 * Platform macros to support interrupt handling
 */
#if EL3_EXCEPTION_HANDLING
#define PLAT_PRI_BITS                   U(3)
#define PLAT_SDEI_CRITICAL_PRI          U(0x20)
#define PLAT_SDEI_NORMAL_PRI            U(0x30)
#endif
#define PLAT_RAS_PRI                    U(0x10)
#define PLAT_IRQ_NORMAL_PRIORITY        U(0x40)

#if EL3_EXCEPTION_HANDLING
#define PLAT_EHF_DESC                   EHF_PRI_DESC(PLAT_PRI_BITS, \
						     PLAT_IRQ_NORMAL_PRIORITY)
#endif

/*
 * Trusted watchdog timer setup
 */
#define PLAT_WDT_TIMEOUT_DEFAULT_SEC U(60)
#define PLAT_WDT_TIMEOUT_MIN_SEC U(2)
#define PLAT_WDT_TIMEOUT_MAX_SEC U(120)

#endif /* PLAT_COMMON_DEF_H */
