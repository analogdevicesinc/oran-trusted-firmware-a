/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_C2CC_UTIL_H
#define ADI_C2CC_UTIL_H

#include "adi_c2cc.h"

extern uintptr_t adi_c2cc_primary_addr_base;
extern uintptr_t adi_c2cc_secondary_addr_base;
extern bool adi_c2cc_loopback;

uint32_t adi_c2cc_read32(uintptr_t addr);
void adi_c2cc_write32(uintptr_t addr, uint32_t val);
uint32_t adi_c2cc_read_bf32(uintptr_t addr, uint8_t position, uint32_t mask);
void adi_c2cc_write_bf32(uintptr_t addr, uint8_t position, uint32_t mask, uint32_t val);
bool adi_c2cc_wait_transactions(uintptr_t addr_base);
void adi_c2cc_intr_control(uintptr_t addr_base, bool enabled);
void adi_c2cc_axi_control(uintptr_t addr_base, bool enabled);

#endif /* ADI_C2CC_UTIL_H */
