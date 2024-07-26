/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_DEVICE_PROFILE_H
#define PLAT_DEVICE_PROFILE_H
#include <stddef.h>

#define ETH_LEN                           6     // Bytes

#define MAX_NODE_NAME_LENGTH             30

/* Declared in platform.h, but mentioned here for consistency.
 * unsigned int plat_get_syscnt_freq2(void);
 */

const char *plat_get_boot_slot(void);
void plat_set_boot_slot(const char *slot);
void plat_set_fw_config_rollback_ctr(void);
uint32_t plat_get_fw_config_rollback_ctr(void);
int plat_get_enforcement_counter(unsigned int *nv_ctr);
int  plat_get_mac_setting(uint32_t index, uint8_t **mac);
int plat_get_num_macs(void);
bool plat_is_sysc(void);
bool plat_is_bootrom_bypass_enabled(void);

size_t plat_get_dram_size(void);

#endif /* PLAT_DEVICE_PROFILE_H */
