/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <errno.h>

#include <drivers/adi/adi_te_interface.h>
#include <drivers/delay_timer.h>
#include <plat_cli.h>
#include <plat_err.h>
#include <plat_rma.h>
#include <platform_def.h>

static int plat_end_function(uint8_t *command_buffer, bool help)
{
	return 0;
}

static void print_data(uint8_t *buf, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++)
		printf("%02x ", buf[i]);
	printf("\n");
}

#pragma weak plat_prepare_mailboxes_function
int plat_prepare_mailboxes_function(uint8_t *command_buffer, bool help)
{
	int status = 0;

	if (help) {
		printf("prepare                            ");
		printf("Prepares TE mailbox for RMA functions.\n");
		printf("                                   ");
		printf("This command must be called before any other command.\n");
	} else {
		printf("Preparing TE mailbox\n");

		adi_enclave_mailbox_init(TE_MAILBOX_BASE);
	}
	return status;
}

#pragma weak plat_challenge_request_function
int plat_challenge_request_function(uint8_t *command_buffer, bool help)
{
	uint8_t challenge_buf[16] = { 0 };
	uint32_t chal_buf_len = sizeof(challenge_buf);
	int status = 0;
	uint64_t type = 0;
	chal_type_e chal;

	if (help) {
		printf("request <type>                     ");
		printf("Performs challenge response request TE mailbox API.\n");
		printf("                                   ");
		printf("<type>, 0=SECURE_DEBUG_ACCESS, 1=SET_CUST_RMA, 2=SET_ADI_RMA\n");
	} else {
		printf("Challenge response request\n");

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
			plat_error_message("Invalid challenge type: %ld", type);
			return -1;
		}

		/* Call challenge request API */
		status = adi_enclave_request_challenge(TE_MAILBOX_BASE, chal, challenge_buf, &chal_buf_len);
		if (status != 0)
			printf("Status of challenge response API : %x\n", status);
		else
			print_data(challenge_buf, chal_buf_len);
	}
	return status;
}

#pragma weak plat_secure_debug_access_function
int plat_secure_debug_access_function(uint8_t *command_buffer, bool help)
{
	int status = 0;
	uint8_t response_buf[64] = { 0 };
	uint64_t data = 0;

	if (help) {
		printf("debug                              ");
		printf("Performs secure debug access TE mailbox API.\n");
	} else {
		printf("Secure debug access\n");

		/* Get signed response buffer */
		for (int i = 0; i < 64; i++) {
			if (command_buffer == NULL) {
				plat_error_message("Missing signed response");
				return -1;
			}
			command_buffer = parse_next_param(16, command_buffer, &data);
			response_buf[i] = (uint8_t)data;
		}

		/* Call secure debug access API */
		status = adi_enclave_priv_secure_debug_access(TE_MAILBOX_BASE, response_buf, sizeof(response_buf));

		printf("Status of secure debug access: %x\n", status);
	}
	return status;
}

#pragma weak plat_rma_function
int plat_rma_function(uint8_t *command_buffer, bool help)
{
	int status = 0;
	uint8_t response_buf[64] = { 0 };
	uint64_t data = 0;

	if (help) {
		printf("rma                                ");
		printf("Performs RMA TE mailbox API.\n");
	} else {
		printf("RMA\n");

		/* Get signed response buffer */
		for (int i = 0; i < 64; i++) {
			command_buffer = parse_next_param(16, command_buffer, &data);
			if (command_buffer == NULL) {
				plat_error_message("Missing signed response");
				return -1;
			}
			response_buf[i] = (uint8_t)data;
		}

		/* Call secure debug access API */
		status = adi_enclave_priv_set_rma(TE_MAILBOX_BASE, response_buf, sizeof(response_buf));

		printf("Status of RMA: %x\n", status);
	}
	return status;
}

cli_command_t plat_command_list[] = {
	{ "debug",   plat_secure_debug_access_function },
	{ "prepare", plat_prepare_mailboxes_function   },
	{ "request", plat_challenge_request_function   },
	{ "rma",     plat_rma_function		       },
	{ "end",     plat_end_function		       }
};
