/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/aarch64/arch_helpers.h>
#include <assert.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>
#include <stddef.h>
#include <string.h>

#include <drivers/adi/adi_te_interface.h>

#include "adi_te_mailbox.h"

#define HOST_ERROR_INVALID_ARGS (0x01UL)        /* 0x01 - Host error invalid arguments */
#define HOST_ERROR_BUFFER       (0x02UL)
#define NUM_MAILBOX_DATA_REGS (10)
#define CHALLENGE_SIZE_MAX_BYTES (16)           /* 128-bit nonce */
#define RESPONSE_SIZE_BYTES (2 * 256 / 8)       /* R,S of Ed25519 */
#define KEYC_KEY_SIZE_16 (16u)
#define KEYC_KEY_SIZE_TAG (8u)
#define ADI_TE_RET_OK   (0)
#define TE_RESPONSE_TIMEOUT_US_8_S        (8000000U)
#define TE_INIT_WAITING_TIMEOUT_US_1_S        (1000000U)

uint32_t mb_regs_mdr[NUM_MAILBOX_DATA_REGS] = {
	MB_REGS_MDR0,
	MB_REGS_MDR1,
	MB_REGS_MDR2,
	MB_REGS_MDR3,
	MB_REGS_MDR4,
	MB_REGS_MDR5,
	MB_REGS_MDR6,
	MB_REGS_MDR7,
	MB_REGS_MDR8,
	MB_REGS_MDR9,
};

typedef enum {
	ADI_ENCLAVE_GET_ENCLAVE_VERSION		= 0x00,
	ADI_ENCLAVE_GET_MAILBOX_VERSION		= 0x01,
	ADI_ENCLAVE_GET_API_VERSION		= 0x02,
	ADI_ENCLAVE_ENABLE_FEATURE		= 0x06,
	ADI_ENCLAVE_GET_ENABLED_FEATURES	= 0x07,
	ADI_ENCLAVE_GET_DEVICE_IDENTITY		= 0x09,
	ADI_ENCLAVE_GET_SERIAL_NUMBER		= 0x0b,
	ADI_ENCLAVE_INCR_ANTIROLLBACK_VERSION	= 0x1c,
	ADI_ENCLAVE_GET_ANTIROLLBACK_VERSION	= 0x1d,
	ADI_ENCLAVE_GET_HUK			= 0x1e,
	ADI_ENCLAVE_REQUEST_CHALLENGE		= 0x80,
	ADI_ENCLAVE_PRIV_SECURE_DEBUG_ACCESS	= 0x8a,
	ADI_ENCLAVE_PRIV_SET_RMA		= 0x8b,
	ADI_ENCLAVE_PROV_HSTKEY			= 0x90,
	ADI_ENCLAVE_PROV_PREPARE_FINALIZE	= 0x91,
	ADI_ENCLAVE_PROV_FINALIZE		= 0x92,
	ADI_ENCLAVE_UNWRAP_CUST_KEY		= 0x182,
	ADI_ENCLAVE_WRAP_CUST_KEY		= 0x183,
	ADI_ENCLAVE_RANDOM			= 0x184,
} adi_enclave_api_id_t;

static uint8_t te_buf[1024] __attribute__((aligned(sizeof(uint64_t))));    /* Buffer to transfer data through TE mailbox */
static uintptr_t cur_ptr = (uintptr_t)te_buf;

const char *adi_enclave_get_lifecycle_state_str(uintptr_t base_addr)
{
	switch (adi_enclave_get_lifecycle_state(base_addr)) {
	case ADI_LIFECYCLE_UNTESTED:
		return "Untested";
	case ADI_LIFECYCLE_OPEN_SAMPLE:
		return "Open Sample";
	case ADI_LIFECYCLE_TESTED:
		return "Tested";
	case ADI_LIFECYCLE_ADI_PROV_ENC:
		return "ADI Provisioned";
	case ADI_LIFECYCLE_CUST1_PROV_HOST:
		return "Customer Provisioned";
	case ADI_LIFECYCLE_DEPLOYED:
		return "Deployed";
	case ADI_LIFECYCLE_CUST1_RETURN:
		return "Customer RMA";
	case ADI_LIFECYCLE_ADI_RETURN:
		return "ADI RMA";
	case ADI_LIFECYCLE_END_OF_LIFE:
		return "End of Life";
	default:
		return "Unknown";
	}
}

void adi_enclave_mailbox_init(uintptr_t base_addr)
{
	uint8_t output_buf[16] = { 0 };
	uint32_t output_buf_len = sizeof(output_buf);
	uint32_t print_timeout, reg;

	/* Wait for OpFw initialization to complete, will get a success response when it is running */
	NOTICE("Waiting for Tiny Enclave initialization at address %lx\n", base_addr);
	print_timeout = timeout_init_us(TE_INIT_WAITING_TIMEOUT_US_1_S);
	while (adi_enclave_get_enclave_version(base_addr, output_buf, &output_buf_len) != 0) {
		if (timeout_elapsed(print_timeout)) {
			NOTICE("Still waiting\n");
			reg = mmio_read_32(base_addr + MB_REGS_BOOT_FLOW1);
			if (reg != 0x0) {
				ERROR("TE @ %lx: OpFw failed with error code: %x\n", base_addr, reg);
				plat_error_handler(-ETIMEDOUT);
			}
			print_timeout = timeout_init_us(TE_INIT_WAITING_TIMEOUT_US_1_S);
		}
	}

	NOTICE("Lifecycle State from %lx: %s\n", base_addr, adi_enclave_get_lifecycle_state_str(base_addr));
	NOTICE("TE @ %lx: App Pack %s\n", base_addr, adi_enclave_get_active_boot_slot(base_addr));
}

adi_lifecycle_t adi_enclave_get_lifecycle_state(uintptr_t base_addr)
{
	return (mmio_read_32(base_addr + MB_REGS_LIFECYCLE_STATUS) & MB_REGS_LIFECYCLE_ENCODE_MASK) >> MB_REGS_LIFECYCLE_ENCODE_BITP;
}

const char * adi_enclave_get_active_boot_slot(uintptr_t base_addr)
{
	uint32_t addr1_first_attempt;
	const char *boot_slot;
	uint32_t error_code_0;

	/* Determine App Pack version that was booted
	 * If sbs_addr2_1st_attempt is set, error_code_0 not set means app_pack_b is active and error_code_0 set means app_pack_a is active
	 * If sbs_addr2_1st_attempt is not set, error_code_0 not set means app_pack_a is active and error_code_0 set means app_pack_b is active
	 */
	addr1_first_attempt = (mmio_read_32(base_addr + MB_REGS_BOOT_FLOW0) & MB_REGS_BOOT_FLOW0_SBS_ADDR1_1ST_ATTEMPT_BITM) >> MB_REGS_BOOT_FLOW0_SBS_ADDR1_1ST_ATTEMPT_BITP;
	error_code_0 = (mmio_read_32(base_addr + MB_REGS_BOOT_FLOW1) & MB_REGS_BOOT_FLOW1_ERROR_CODE_0_BITM) >> MB_REGS_BOOT_FLOW1_ERROR_CODE_0_BITP;

	if (addr1_first_attempt == 0) {
		if (error_code_0 == 0)
			boot_slot = "a";
		else
			boot_slot = "b";
	} else {
		if (error_code_0 == 0)
			boot_slot = "b";
		else
			boot_slot = "a";
	}

	return boot_slot;
}

static void ack_response(uintptr_t base_addr)
{
	mmio_write_32(base_addr + MB_REGS_H_STATUS, MB_REGS_ERESP_ACK);
}

static void signal_request_ready(uintptr_t base_addr)
{
	mmio_write_32(base_addr + MB_REGS_H_STATUS, MB_REGS_HREQ_RDY);
}

static int wait_for_response(uintptr_t base_addr)
{
	uint32_t status = 0;
	uint64_t timeout;

	timeout = timeout_init_us(TE_RESPONSE_TIMEOUT_US_8_S);
	while ((status & MB_REGS_ERESP_RDY) != MB_REGS_ERESP_RDY) {
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;
		status = mmio_read_32(base_addr + MB_REGS_E_STATUS);
	}

	return ADI_TE_RET_OK;
}

static int verify_buf_len_ptr(const void *buf, uint32_t *buflen, uint32_t minlen, uint32_t maxlen)
{
	if (buf == NULL || buflen == NULL)
		return HOST_ERROR_INVALID_ARGS;

	if (minlen <= maxlen && (*buflen < minlen || *buflen > maxlen))
		return HOST_ERROR_INVALID_ARGS;

	/* Check for overflow of te_buf */
	if ((cur_ptr + *buflen) > ((uintptr_t)te_buf + sizeof(te_buf)))
		return HOST_ERROR_BUFFER;

	return ADI_TE_RET_OK;
}

static int verify_buf_len(const void *buf, uint32_t buflen, uint32_t minlen, uint32_t maxlen)
{
	return verify_buf_len_ptr(buf, &buflen, minlen, maxlen);
}

/* Copy buffer to te_buf which is used to transfer data through the mailbox */
static uintptr_t reserve_buf(uintptr_t buf, uint32_t size)
{
	uintptr_t old_ptr = cur_ptr;

	memcpy((void *)cur_ptr, (void *)buf, size);

	cur_ptr += size;

	return old_ptr;
}

/* Set pointer back to start of te_buf and clear buffer */
static void buf_init(void)
{
	cur_ptr = (uintptr_t)te_buf;

	memset(te_buf, 0, sizeof(te_buf));
}

/* Data sent through TE mailbox must be copied to te_buf prior to calling this function to be able to flush/invalidate memory */
static int perform_enclave_transaction(uintptr_t base_addr, adi_enclave_api_id_t requestId, uint32_t args[], uint32_t numArgs)
{
	uint32_t i;
	int ret;

	if ((args == NULL && numArgs != 0) || (numArgs > NUM_MAILBOX_DATA_REGS))
		return HOST_ERROR_INVALID_ARGS;

	/* Flush cache */
	flush_dcache_range((uintptr_t)te_buf, sizeof(te_buf));

	mmio_write_32(base_addr + MB_REGS_HRC0, requestId);

	for (i = 0; i < numArgs; i++)
		mmio_write_32(base_addr + mb_regs_mdr[i], args[i]);

	signal_request_ready(base_addr);

	ret = wait_for_response(base_addr);
	if (ret != ADI_TE_RET_OK) {
		ERROR("Timed out waiting for Enclave mailbox response\n");
		plat_error_handler(-ETIMEDOUT);
		return ret;
	}

	ack_response(base_addr);

	/* Invalidate cache */
	inv_dcache_range((uintptr_t)te_buf, sizeof(te_buf));

	for (i = 0; i < numArgs; i++)
		args[i] = mmio_read_32(base_addr + mb_regs_mdr[i]);

	return mmio_read_32(base_addr + MB_REGS_ERC1);
}

/* Tiny Enclave version */
int adi_enclave_get_enclave_version(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[1] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_ENCLAVE_VERSION, args, 2);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[0], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[1], sizeof(*o_buff_len));
	}

	return status;
}

/* The version of the mailbox HW block as provided in RTL and memory mapped */
int adi_enclave_get_mailbox_version(uintptr_t base_addr)
{
	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_MAILBOX_VERSION, NULL, 0);
}

/* Get the device serial number provisioned in OTP */
int adi_enclave_get_serial_number(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[1] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_SERIAL_NUMBER, args, 2);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[0], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[1], sizeof(*o_buff_len));
	}

	return status;
}

/* Only responds while in CUST1_PROV_HOST lifecycle
 * Sets the device lifecycle to DEPLOYED
 */
int adi_enclave_provision_finalize(uintptr_t base_addr)
{
	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_PROV_FINALIZE, NULL, 0);
}

/* Initiate the challenge-response protocol by asking the enclave for the challenge */
int adi_enclave_request_challenge(uintptr_t base_addr, chal_type_e chal_type, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[3];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = chal_type;
	args[1] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[2] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_REQUEST_CHALLENGE, args, 3);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[1], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[2], sizeof(*o_buff_len));
	}

	return status;
}

/* Sets the lifecycle of the part to CUST or ADI RMA depending on the type of RMA challenge requested
 * Calling this API without first calling adi_enclave_request_challenge will result in an error.
 */
int adi_enclave_priv_set_rma(uintptr_t base_addr, const uint8_t *cr_input_buffer, uint32_t input_buff_len)
{
	int ret;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len(cr_input_buffer, input_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)cr_input_buffer, input_buff_len);
	args[1] = input_buff_len;

	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_PRIV_SET_RMA, args, 2);
}

int adi_enclave_priv_secure_debug_access(uintptr_t base_addr, const uint8_t *cr_input_buffer, uint32_t input_buff_len)
{
	int ret;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len(cr_input_buffer, input_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)cr_input_buffer, input_buff_len);
	args[1] = input_buff_len;

	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_PRIV_SECURE_DEBUG_ACCESS, args, 2);
}

int adi_enclave_get_api_version(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[1] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_API_VERSION, args, 2);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[0], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[1], sizeof(*o_buff_len));
	}

	return status;
}

/* Request the enclave to enable a feature/features of the system by issuing a Feature Certificate (FCER) */
int adi_enclave_enable_feature(uintptr_t base_addr, const uint8_t *input_buffer_fcer, uint32_t fcer_len)
{
	int ret;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len(input_buffer_fcer, fcer_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)input_buffer_fcer, fcer_len);
	args[1] = fcer_len;

	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_ENABLE_FEATURE, args, 2);
}

/* Get what's currently enabled in the system */
int adi_enclave_get_enabled_features(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[1] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_ENABLED_FEATURES, args, 2);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[0], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[1], sizeof(*o_buff_len));
	}

	return status;
}

int adi_enclave_get_device_identity(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[1] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_DEVICE_IDENTITY, args, 2);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[0], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[1], sizeof(*o_buff_len));
	}

	return status;
}


/* Only responds while in ADI_PROV_ENC lifecycle and must be called prior to adi_enclave_provisionPrepareFinalize() */
int adi_enclave_provision_host_keys(uintptr_t base_addr, const uintptr_t hst_keys, uint32_t hst_keys_len, uint32_t hst_keys_size)
{
	int ret;
	unsigned int key_num;
	uint32_t args[2];
	host_keys_t *tmp_hst_keys;

	buf_init();

	ret = verify_buf_len((const void *)hst_keys, hst_keys_size, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	tmp_hst_keys = (host_keys_t *)reserve_buf((uintptr_t)hst_keys, hst_keys_size);

	for (key_num = 0; key_num < hst_keys_len; key_num++) {
		ret = verify_buf_len(tmp_hst_keys[key_num].key, tmp_hst_keys[key_num].key_len, 1, (uint32_t)SIZE_MAX);
		if (ret != ADI_TE_RET_OK)
			return ret;

		tmp_hst_keys[key_num].key = (uint8_t *)reserve_buf((uintptr_t)tmp_hst_keys[key_num].key, tmp_hst_keys[key_num].key_len);
	}

	args[0] = (uintptr_t)tmp_hst_keys;
	args[1] = hst_keys_len;

	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_PROV_HSTKEY, args, 2);
}

/* Only responds while in ADI_PROV_ENC lifecycle
 * This mailbox API will partially complete the remaining items that could not occur during the ADI
 * provisioning in PRFW execution (calculate CRC, set lockout bits, sets device lifecycle to CUST_PROVISIONED)
 * The API adi_enclave_provisionFinalize() is supposed to be called subsequent to this function
 */
int adi_enclave_provision_prepare_finalize(uintptr_t base_addr)
{
	return perform_enclave_transaction(base_addr, ADI_ENCLAVE_PROV_PREPARE_FINALIZE, NULL, 0);
}

/* Use host IPK (c1) in OTP (wrapped by RIPK) to unwrap host c2 key */
int adi_enclave_unwrap_cust_key(uintptr_t base_addr, const void *wrapped_key, uint32_t wk_len,
				void *unwrapped_key, uint32_t *uwk_len)
{
	int ret, status;
	uint32_t args[4];

	buf_init();

	ret = verify_buf_len(wrapped_key, wk_len, KEYC_KEY_SIZE_16 + KEYC_KEY_SIZE_TAG, KEYC_KEY_SIZE_16 + KEYC_KEY_SIZE_TAG);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)wrapped_key, wk_len);
	args[1] = wk_len;

	ret = verify_buf_len(unwrapped_key, *uwk_len, KEYC_KEY_SIZE_16, KEYC_KEY_SIZE_16);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[2] = (uint32_t)reserve_buf((uintptr_t)unwrapped_key, *uwk_len);

	ret = verify_buf_len(uwk_len, *uwk_len, KEYC_KEY_SIZE_16, KEYC_KEY_SIZE_16);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[3] = (uint32_t)reserve_buf((uintptr_t)uwk_len, sizeof(*uwk_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_UNWRAP_CUST_KEY, args, 4);
	if (status == 0) {
		memcpy(unwrapped_key, (void *)(uintptr_t)args[2], *uwk_len);
		memcpy(uwk_len, (void *)(uintptr_t)args[3], sizeof(*uwk_len));
	}

	return status;
}

/* Increment the Security version of APP in OTP by 1 on every successive call */
int adi_enclave_update_otp_app_anti_rollback(uintptr_t base_addr, uint32_t *appSecVer)
{
	int ret, status;
	uint32_t args[1];

	buf_init();

	ret = verify_buf_len(appSecVer, sizeof(*appSecVer), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)appSecVer, sizeof(*appSecVer));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_INCR_ANTIROLLBACK_VERSION, args, 1);
	if (status == 0)
		memcpy(appSecVer, (void *)(uintptr_t)args[0], sizeof(*appSecVer));

	return status;
}

/* Get the Security version of APP in OTP */
int adi_enclave_get_otp_app_anti_rollback(uintptr_t base_addr, uint32_t *appSecVer)
{
	int ret, status;
	uint32_t args[1];

	buf_init();

	ret = verify_buf_len(appSecVer, sizeof(*appSecVer), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)appSecVer, sizeof(*appSecVer));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_ANTIROLLBACK_VERSION, args, 1);
	if (status == 0)
		memcpy(appSecVer, (void *)(uintptr_t)args[0], sizeof(*appSecVer));

	return status;
}

int adi_enclave_get_huk(uintptr_t base_addr, uint8_t *output_buffer, uint32_t *o_buff_len)
{
	int ret, status;
	uint32_t args[2];

	buf_init();

	ret = verify_buf_len_ptr(output_buffer, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf((uintptr_t)output_buffer, *o_buff_len);

	ret = verify_buf_len(o_buff_len, sizeof(*o_buff_len), 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[1] = (uint32_t)reserve_buf((uintptr_t)o_buff_len, sizeof(*o_buff_len));

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_GET_HUK, args, 2);
	if (status == 0) {
		memcpy(output_buffer, (void *)(uintptr_t)args[0], *o_buff_len);
		memcpy(o_buff_len, (void *)(uintptr_t)args[1], sizeof(*o_buff_len));
	}

	return status;
}


int adi_enclave_random_bytes(uintptr_t base_addr, void *output_buffer, uint32_t o_buff_len)
{
	uintptr_t buf = (uintptr_t)output_buffer;
	uint32_t args[2];
	int ret, status;

	buf_init();

	ret = verify_buf_len((const void *)buf, o_buff_len, 1, (uint32_t)SIZE_MAX);
	if (ret != ADI_TE_RET_OK)
		return ret;

	args[0] = (uint32_t)reserve_buf(buf, o_buff_len);
	args[1] = (uint32_t)o_buff_len;

	status = perform_enclave_transaction(base_addr, ADI_ENCLAVE_RANDOM, args, 2);
	if (status == 0)
		memcpy(output_buffer, (void *)(uintptr_t)args[0], o_buff_len);

	return status;
}

bool adi_enclave_is_host_boot_ready(uintptr_t base_addr)
{
	uint32_t reg;

	reg = mmio_read_32(base_addr + MB_REGS_BOOT_FLOW0);

	if ((reg & MB_REGS_BOOT_FLOW0_BOOT_FAILED_BITM) == MB_REGS_BOOT_FLOW0_BOOT_FAILED_BITM) {
		ERROR("TE @ %lx: Boot failed with error code: %x\n", base_addr, mmio_read_32(base_addr + MB_REGS_BOOT_FLOW1));
		return false;
	} else {
		NOTICE("TE @ %lx: Boot status: %x\n", base_addr, reg);
	}

	/* TE is ready for host boot when its boot status indicates LOAD_AND_UNWRAP_KEYS has been performed */
	if ((reg & MB_REGS_BOOT_FLOW0_LOAD_AND_UNWRAP_KEYS_BITM) == MB_REGS_BOOT_FLOW0_LOAD_AND_UNWRAP_KEYS_BITM)
		return true;
	return false;
}
