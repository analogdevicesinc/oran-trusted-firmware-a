/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_CLI_H
#define PLAT_CLI_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_COMMAND_NAME_LENGTH    64
#define MAX_COMMAND_COUNT          200

typedef struct {
	char cmd_name[MAX_COMMAND_NAME_LENGTH];
	int (*cmd_function)(uint8_t *command_buffer, bool help);
} cli_command_t;

extern cli_command_t plat_command_list[];

void plat_enter_cli(void);
uint8_t * parse_next_param(uint32_t base, uint8_t *input_buffer, uint64_t *data);

#endif /* PLAT_CLI_H */
