/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/adi/adi_te_interface.h>
#include <adrv906x_dual.h>
#include <adrv906x_mmap.h>

#include <plat_cli.h>
#include <plat_rma.h>
#include <platform_def.h>

static bool initialized = false;

static bool dual_tile_enabled = false;

static void print_data(uint8_t *buf, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++)
		printf("%02x ", buf[i]);
	printf("\n");
}

int plat_prepare_rma_mailboxes(bool secondary)
{
	/* Initialize the tiny enclave mailbox */
	adi_enclave_mailbox_init(TE_MAILBOX_BASE);

	if (secondary) {
		plat_setup_secondary_mmap(false);
		adrv906x_enable_secondary_tile();
		adrv906x_load_secondary_image();
		dual_tile_enabled = true;
	} else {
		dual_tile_enabled = false;
	}

	return 0;
}

int plat_rma_challenge_request(chal_type_e chal, bool secondary)
{
	int status = 0;
	uint8_t challenge_buf[16] = { 0 };
	uint32_t chal_buf_len = sizeof(challenge_buf);

	if (secondary) {
		if (!dual_tile_enabled) {
			printf("Secondary tile is not enabled, run the prepare command with secondary argument first\n");
			return -1;
		}
		status = adi_enclave_request_challenge(SEC_TE_MAILBOX_BASE, chal, challenge_buf, &chal_buf_len);
		if (status != 0) {
			printf("Status of the secondary challenge response API : %x\n", status);
		} else {
			printf("Secondary challenge response: ");
			print_data(challenge_buf, chal_buf_len);
		}
	} else {
		/* Call challenge request API */
		status = adi_enclave_request_challenge(TE_MAILBOX_BASE, chal, challenge_buf, &chal_buf_len);
		if (status != 0) {
			printf("Status of challenge response API : %x\n", status);
		} else {
			printf("Primary challenge response: ");
			print_data(challenge_buf, chal_buf_len);
		}
	}
	return status;
}

int plat_rma_secure_debug_access(uint8_t *response_buf, uint32_t size, bool secondary)
{
	int status = 0;

	if (secondary) {
		if (!dual_tile_enabled) {
			printf("Secondary tile is not enabled, run the prepare command with secondary argument first\n");
			return -1;
		}
		status = adi_enclave_priv_secure_debug_access(SEC_TE_MAILBOX_BASE, response_buf, size);
		printf("Status of the secondary secure debug access: %x\n", status);
	} else {
		/* Call secure debug access API */
		status = adi_enclave_priv_secure_debug_access(TE_MAILBOX_BASE, response_buf, size);
		printf("Status of secure debug access: %x\n", status);
	}

	return status;
}

int plat_rma_perform_rma(uint8_t *response_buf, uint32_t size, bool secondary)
{
	int status = 0;

	if (secondary) {
		if (!dual_tile_enabled) {
			printf("Secondary tile is not enabled, run the prepare command with secondary argument first\n");
			return -1;
		}
		status = adi_enclave_priv_set_rma(SEC_TE_MAILBOX_BASE, response_buf, size);
		printf("Status of the secondary RMA: %x\n", status);
	} else {
		/* Call RMA API */
		status = adi_enclave_priv_set_rma(TE_MAILBOX_BASE, response_buf, size);
		printf("Status of RMA: %x\n", status);
	}

	return status;
}

int plat_prepare_mailboxes_function(uint8_t *command_buffer, bool help)
{
	int status = 0;
	uint64_t secondary = 0;

	if (help) {
		printf("prepare <secondary>                ");
		printf("Prepares TE mailboxes for RMA functions.\n");
		printf("                                   ");
		printf("This command must be called before any other command.\n");
		printf("                                   ");
		printf("<secondary>,1=Secondary tile enabled, 0=secondary tile disabled\n");
	} else {
		printf("Preparing TE mailboxes\n");

		if (command_buffer == NULL) {
			printf("Missing secondary argument");
			return -1;
		}
		command_buffer = parse_next_param(10, command_buffer, &secondary);

		status = plat_prepare_rma_mailboxes(secondary);
		if (status == 0)
			initialized = true;
	}
	return status;
}

int plat_challenge_request_function(uint8_t *command_buffer, bool help)
{
	int status = 0;
	uint64_t type = 0;
	uint64_t secondary = 0;
	chal_type_e chal;

	if (help) {
		printf("request <secondary> <type>         ");
		printf("Performs challenge response request TE mailbox API.\n");
		printf("                                   ");
		printf("<secondary>, 1=Secondary tile, 0=Primary tile\n");
		printf("                                   ");
		printf("<type>, 0=SECURE_DEBUG_ACCESS, 1=SET_CUST_RMA, 2=SET_ADI_RMA\n");
	} else {
		printf("Challenge response request\n");

		if (initialized == false) {
			printf("Mailboxes not initialized. Call prepare command first");
			return -1;
		}

		command_buffer = parse_next_param(10, command_buffer, &secondary);
		if (secondary != 0 && secondary != 1) {
			printf("Invalid secondary argument");
			return -1;
		}

		/* Get challenge type */
		command_buffer = parse_next_param(10, command_buffer, &type);
		if (command_buffer == NULL) {
			printf("No challenge type provided\n");
			return -1;
		}

		switch (type) {
		case 0:
			chal = ADI_ENCLAVE_CHAL_SECURE_DEBUG_ACCESS;
			break;
		case 1:
			chal = ADI_ENCLAVE_CHAL_SET_CUST_RMA;
			break;
		case 2:
			chal = ADI_ENCLAVE_CHAL_SET_ADI_RMA;
			break;
		default:
			printf("Invalid challenge type: %ld", type);
			return -1;
		}
		printf("Challenge type: %ld\n", chal);

		status = plat_rma_challenge_request(chal, (bool)secondary);
	}
	return status;
}


int plat_secure_debug_access_function(uint8_t *command_buffer, bool help)
{
	int status = 0;
	uint8_t response_buf[64] = { 0 };
	uint64_t data = 0;
	uint64_t secondary = 0;

	if (help) {
		printf("debug <secondary>                  ");
		printf("Performs secure debug access TE mailbox API.\n");
		printf("                                   ");
		printf("<secondary>, 1=Secondary tile, 0=Primary tile\n");
	} else {
		printf("Secure debug access\n");

		if (initialized == false) {
			printf("Mailboxes not initialized. Call prepare command first");
			return -1;
		}

		command_buffer = parse_next_param(10, command_buffer, &secondary);
		if (secondary != 0 && secondary != 1) {
			printf("Invalid secondary argument");
			return -1;
		}

		/* Get signed response buffer */
		for (int i = 0; i < 64; i++) {
			if (command_buffer == NULL) {
				printf("Missing signed response");
				return -1;
			}
			command_buffer = parse_next_param(16, command_buffer, &data);
			response_buf[i] = (uint8_t)data;
		}

		status = plat_rma_secure_debug_access(response_buf, sizeof(response_buf), (bool)secondary);
	}
	return status;
}

int plat_rma_function(uint8_t *command_buffer, bool help)
{
	int status = 0;
	uint8_t response_buf[64] = { 0 };
	uint64_t data = 0;
	uint64_t secondary = 0;

	if (help) {
		printf("rma <secondary>                    ");
		printf("Performs RMA TE mailbox API.\n");
		printf("                                   ");
		printf("<secondary>, 1=Secondary tile, 0=Primary tile\n");
	} else {
		printf("RMA\n");

		if (initialized == false) {
			printf("Mailboxes not initialized. Call prepare command first");
			return -1;
		}

		command_buffer = parse_next_param(10, command_buffer, &secondary);
		if (secondary != 0 && secondary != 1) {
			printf("Invalid secondary argument");
			return -1;
		}

		/* Get signed response buffer */
		for (int i = 0; i < 64; i++) {
			command_buffer = parse_next_param(16, command_buffer, &data);
			if (command_buffer == NULL) {
				printf("Missing signed response");
				return -1;
			}
			response_buf[i] = (uint8_t)data;
		}

		status = plat_rma_perform_rma(response_buf, sizeof(response_buf), (bool)secondary);
	}
	return status;
}
