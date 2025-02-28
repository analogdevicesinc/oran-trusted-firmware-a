/*
 * Copyright (c) 2015-2024, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_DEVICE_PROFILE_H
#define PLAT_DEVICE_PROFILE_H
#include <stddef.h>

#define ETH_LEN                           6     /* Bytes */

#define MAX_NODE_NAME_LENGTH             30
#define MAX_NODE_STRING_LENGTH             200

/* Declared in platform.h, but mentioned here for consistency.
 * unsigned int plat_get_syscnt_freq2(void);
 */

const char *plat_get_boot_slot(void);
void plat_set_boot_slot(const char *slot);
void plat_set_fw_config_rollback_ctr(void);
uint32_t plat_get_fw_config_rollback_ctr(void);
void plat_set_fw_config_te_rollback_ctr(uint32_t ctr);
uint32_t plat_get_fw_config_te_rollback_ctr(void);
int plat_get_enforcement_counter(unsigned int *nv_ctr);
int  plat_get_mac_setting(uint32_t index, uint8_t **mac);
int plat_get_num_macs(void);
bool plat_is_sysc(void);
size_t plat_get_dram_size(void);
void plat_set_fw_config_reset_cause(uint32_t reset_cause, uint32_t reset_cause_ns);
uint32_t plat_get_fw_config_reset_cause(void);

/* Device tree error logging functions */
void plat_set_fw_config_error_log(char *input);
int plat_get_fw_config_error_num(void);
int get_fw_config_num_node_properties(const char *node_name);
const void *get_fw_config_error_log_prop(int *property_offset, char **name, int *length);

#endif /* PLAT_DEVICE_PROFILE_H */
