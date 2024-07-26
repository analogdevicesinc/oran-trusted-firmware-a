/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLAT_PINTMUX_H__
#define __PLAT_PINTMUX_H__

#include <stdint.h>
#include <stdbool.h>

#define ERR_LOOKUP_FAIL -1
#define ERR_MAP_FAIL -2
#define ERR_NOT_MAPPED -3
#define ERR_SECURITY -4

int plat_pintmux_get_lane(unsigned int pin, uintptr_t base_addr);
bool plat_pintmux_is_lane_secure(unsigned int lane, uintptr_t base_addr);
bool plat_pintmux_is_lane_mapped(unsigned int lane, uintptr_t base_addr);
uint32_t plat_pintmux_lane_to_irq(unsigned int lane, uintptr_t base_addr);
int plat_secure_pintmux_map(unsigned int gpio, bool is_secure, bool pos_mask, uintptr_t base_addr);
int plat_secure_pintmux_unmap(unsigned int lane, bool is_secure, uintptr_t base_addr);
void plat_pintmux_init(void);

#endif /* __PLAT_PINTMUX_H__ */
