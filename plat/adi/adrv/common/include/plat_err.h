/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_ERR_H
#define PLAT_ERR_H

#include "plat_errno.h"

typedef enum {
	RESET_CAUSE_MESSAGE,
	BOOT_COUNT_MESSAGE,
	BOOT_EXHAUSTION_MESSAGE
} boot_message_t;

void plat_panic_reset_cause(void);
int plat_warm_reset(void);
void plat_error_message(char *fmt, ...);
void plat_warn_message(char *fmt, ...);
void plat_runtime_error_message(char *fmt, ...);
void plat_runtime_warn_message(char *fmt, ...);
void plat_record_boot_log(uint8_t type, char *fmt, ...);
void plat_log_dt_boot_messages(void);

#endif
