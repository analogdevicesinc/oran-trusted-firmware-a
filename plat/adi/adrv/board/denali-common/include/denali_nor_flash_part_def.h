/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DENALI_NOR_FLASH_PART_H
#define DENALI_NOR_FLASH_PART_H

#include <drivers/partition/partition.h>

#define NOR_BOOT_A_START        0x00000000
#define NOR_BOOT_B_START        0x00040000
#define NOR_BOOT_SIZE   0x00040000
#define NOR_BOOTCTRL_START      0x00080000
#define NOR_BOOTCTRL_SIZE       0x00010000
#define NOR_BOOTCFG_START       0x00090000
#define NOR_BOOTCFG_SIZE        0x00010000
#define NOR_FIP_A_START 0x000A0000
#define NOR_FIP_B_START 0x002A0000
#define NOR_FIP_SIZE    0x00200000
#define NOR_KERNEL_A_START      0x004A0000
#define NOR_KERNEL_B_START      0x034A0000
#define NOR_KERNEL_SIZE 0x03000000

#define NOR_PART_COUNT  8

#define BOOT_A  "boot_a"
#define BOOT_B  "boot_b"
#define BOOTCTRL        "bootctrl"
#define BOOTCFG "bootcfg"
#define FIP_A   "fip_a"
#define FIP_B   "fip_b"
#define KERNEL_A        "kernel_a"
#define KERNEL_B        "kernel_b"

extern partition_entry_t nor_part_info_list[NOR_PART_COUNT];

#endif
