/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdarg.h>

#include <common/debug.h>
#include <drivers/console.h>
#include <errno.h>
#include <lib/mmio.h>
#include <platform.h>

#include <plat_device_profile.h>
#include <plat_err.h>
#include <plat_runtime_log.h>
#include <plat_status_reg.h>
#include <plat_wdt.h>
#include <platform_def.h>

#define DT_LOG_MESSAGE_MAX              256

/* Holds reset cause, boot exhaustion, and boot count messages */
static char reset_cause_log[MAX_NODE_STRING_LENGTH] = { 0 };
static char boot_exhaustion_log[MAX_NODE_STRING_LENGTH] = { 0 };
static char boot_count_log[MAX_NODE_STRING_LENGTH] = { 0 };

void plat_error_handler(int err)
{
	/*
	 * Set the RESET_CAUSE boot register with the appropriate value
	 * Errno error's mapped to reset causes
	 * If errno is not mapped, reset cause defaults to OTHER_RESET_CAUSE
	 */

	switch (-err) {
	case ECECC:
		plat_wr_status_reg(RESET_CAUSE, CACHE_ECC_ERROR);
		break;
	case EDECC:
		plat_wr_status_reg(RESET_CAUSE, DRAM_ECC_ERROR);
		break;
	case EAUTH:
		plat_wr_status_reg(RESET_CAUSE, IMG_VERIFY_FAIL);
		break;
	case EDINIT:
		plat_wr_status_reg(RESET_CAUSE, DRAM_INIT_ERROR);
		break;
	case EMCS:
		plat_wr_status_reg(RESET_CAUSE, MCS_FAIL);
		break;
	case EMBCAL:
		plat_wr_status_reg(RESET_CAUSE, MBIAS_CAL_FAIL);
		break;
	default:
		plat_wr_status_reg(RESET_CAUSE, OTHER_RESET_CAUSE);
	}

	console_flush();
	plat_warm_reset();

	while (1)
		;
}

/* Log message in device tree */
static void plat_log_dt_message(char *label, char *message)
{
	char log[MAX_NODE_STRING_LENGTH + 7];

	if (plat_get_fw_config_error_num() >= DT_LOG_MESSAGE_MAX) {
		INFO("Unable to log message to device tree, maximum exceeded\n");
		return;
	}

	/* Add label to beginning of message */
	memcpy(log, label, strlen(label));
	memcpy(log + strlen(label), message, strlen(message) + 1);

	/* Log to device tree */
	plat_set_fw_config_error_log(log);
}

/* Record warning message in device tree and to UART */
void plat_warn_message(char *fmt, ...)
{
	char message[MAX_NODE_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsnprintf(message, MAX_NODE_STRING_LENGTH, fmt, args);
	va_end(args);

	plat_log_dt_message("WARN: ", message);
	WARN("%s\n", message);
}

/* Record error message in device tree and to UART */
void plat_error_message(char *fmt, ...)
{
	char message[MAX_NODE_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsnprintf(message, MAX_NODE_STRING_LENGTH, fmt, args);
	va_end(args);

	plat_log_dt_message("ERROR: ", message);
	ERROR("%s\n", message);
}

/* Log runtime message */
static void plat_log_runtime_message(char *label, char *message)
{
	char log[MAX_NODE_STRING_LENGTH + 7];

	/* Add label to beginning of message */
	memcpy(log, label, strlen(label));
	memcpy(log + strlen(label), message, strlen(message) + 1);

	/* Log to runtime buffer */
	write_to_runtime_buffer(log);
}

/* Record runtime warning message */
void plat_runtime_warn_message(char *fmt, ...)
{
	char message[MAX_NODE_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsnprintf(message, MAX_NODE_STRING_LENGTH, fmt, args);
	va_end(args);

	plat_log_runtime_message("WARN: ", message);
	WARN("%s\n", message);
}

/* Record runtime error message */
void plat_runtime_error_message(char *fmt, ...)
{
	char message[MAX_NODE_STRING_LENGTH];
	va_list args;

	va_start(args, fmt);
	vsnprintf(message, MAX_NODE_STRING_LENGTH, fmt, args);
	va_end(args);

	plat_log_runtime_message("ERROR: ", message);
	ERROR("%s\n", message);
}

/* Record reset cause to log in device tree and to UART when available */
void plat_record_boot_log(uint8_t type, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	switch (type) {
	case RESET_CAUSE_MESSAGE:
		vsnprintf(reset_cause_log, MAX_NODE_STRING_LENGTH, fmt, args);
		break;
	case BOOT_COUNT_MESSAGE:
		vsnprintf(boot_count_log, MAX_NODE_STRING_LENGTH, fmt, args);
		break;
	case BOOT_EXHAUSTION_MESSAGE:
		vsnprintf(boot_exhaustion_log, MAX_NODE_STRING_LENGTH, fmt, args);
		break;
	default:
		break;
	}

	va_end(args);
}

/* Log reset cause, boot count, and boot exhaustion to device tree, if present */
void plat_log_dt_boot_messages(void)
{
	if (strlen(reset_cause_log) != 0) {
		plat_log_dt_message("ERROR: ", reset_cause_log);
		memset(reset_cause_log, 0, sizeof(reset_cause_log));
	}

	if (strlen(boot_count_log) != 0) {
		plat_log_dt_message("ERROR: ", boot_count_log);
		memset(boot_count_log, 0, sizeof(boot_count_log));
	}

	if (strlen(boot_exhaustion_log) != 0) {
		plat_log_dt_message("ERROR: ", boot_exhaustion_log);
		memset(boot_exhaustion_log, 0, sizeof(boot_exhaustion_log));
	}
}
