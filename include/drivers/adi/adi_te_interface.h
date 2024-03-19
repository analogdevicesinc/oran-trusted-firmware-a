/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_TE_INTERFACE_H
#define ADI_TE_INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct __attribute__((packed, aligned(sizeof(uint64_t)))){
	uint8_t hst_key_id;
	uint8_t key_len;
	uint8_t *key;
} host_keys_t;

typedef enum {
	ADI_ENCLAVE_CHAL_GET_VALUE		= 0xC565B77CUL,
	ADI_ENCLAVE_CHAL_SECURE_DEBUG_ACCESS	= 0xD75F3B50UL,
	ADI_ENCLAVE_CHAL_SET_CUST_RMA		= 0x75A9E161UL,
	ADI_ENCLAVE_CHAL_SET_ADI_RMA		= 0xCC9CC56BUL,
} chal_type_e;

typedef enum {
	ADI_LIFECYCLE_UNTESTED,
	ADI_LIFECYCLE_OPEN_SAMPLE,
	ADI_LIFECYCLE_TESTED,
	ADI_LIFECYCLE_ADI_PROV_ENC,
	ADI_LIFECYCLE_CUST1_PROV_HOST,
	ADI_LIFECYCLE_DEPLOYED,
	ADI_LIFECYCLE_CUST1_RETURN,
	ADI_LIFECYCLE_ADI_RETURN,
	ADI_LIFECYCLE_END_OF_LIFE,
} adi_lifecycle_t;

typedef enum {
	HST_SEC_DEBUG,
	HST_SEC_BOOT,
	HST_PLLSA,
	HST_IPK
} HST_KEY_ID;

void adi_enclave_mailbox_init(uintptr_t base_addr);
adi_lifecycle_t adi_enclave_get_lifecycle_state(uintptr_t base_addr);
const char * adi_enclave_get_lifecycle_state_str(uintptr_t base_addr);
const char * adi_enclave_get_active_boot_slot(uintptr_t base_addr);
int adi_enclave_get_mailbox_version(uintptr_t base_addr);
int adi_enclave_get_enclave_version(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_get_api_version(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_provision_host_keys(uintptr_t base_addr, const uintptr_t hst_keys, uint32_t hst_keys_len, uint32_t hst_key_size);
int adi_enclave_provision_prepare_finalize(uintptr_t base_addr);
int adi_enclave_provision_finalize(uintptr_t base_addr);
int adi_enclave_enable_feature(uintptr_t base_addr, const uint8_t *input_buffer_fcer, uint32_t fcer_len);
int adi_enclave_get_enabled_features(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_get_device_identity(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_get_serial_number(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_request_challenge(uintptr_t base_addr, chal_type_e chal_type, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_priv_set_rma(uintptr_t base_addr, const uint8_t *cr_input_buffer, uint32_t input_buff_len);
int adi_enclave_unwrap_cust_key(uintptr_t base_addr, const void *wrapped_key, uint32_t wk_len, void *unwrapped_key, uint32_t *uwk_len);
int adi_enclave_update_otp_app_anti_rollback(uintptr_t base_addr, uint32_t *appSecVer);
int adi_enclave_get_huk(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len);
int adi_enclave_random_bytes(uintptr_t base_addr, void *output_buffer, uint32_t o_buff_len);
int adi_enclave_priv_secure_debug_access(uintptr_t base_addr, const uint8_t *cr_input_buffer, uint32_t input_buff_len);
bool adi_enclave_is_host_boot_ready(uintptr_t base_addr);

#endif /* ADI_TE_INTERFACE_H */
