/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_OTP_H
#define ADRV906X_OTP_H

#include <stdint.h>

void adrv906x_otp_init_driver(void);
int adrv906x_otp_get_product_id(const uintptr_t mem_ctrl_base, uint8_t *id);
int adrv906x_otp_get_rollback_counter(const uintptr_t mem_ctrl_base, unsigned int *nv_ctr);
int adrv906x_otp_set_rollback_counter(const uintptr_t mem_ctrl_base, unsigned int nv_ctr);
int adrv906x_otp_set_mac_addr(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t *mac);
int adrv906x_otp_get_mac_addr(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t *mac);

#endif /* ADRV906X_OTP_H */
