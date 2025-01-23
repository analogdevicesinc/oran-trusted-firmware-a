/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <plat_runtime_log.h>

/* This value must match the value in optee_os/core/pta/adi/runtime_log.c */
#define SIZE_OF_BL31_RUNTIME_BUFFER 500

#define GROUP_SEPARATOR '\x1D'  /* ASCII Group Separator */

static int buffer_length = 0;
static int read_index = 0;
static char runtime_log[SIZE_OF_BL31_RUNTIME_BUFFER];
static int write_index = 0;
static bool lock = false;

/* Returns true if buffer is full */
static bool is_buffer_full(void)
{
	if (buffer_length == SIZE_OF_BL31_RUNTIME_BUFFER) {
		INFO("BL31 runtime buffer is full\n");
		return true;
	}
	return false;
}

/* Writes one character to runtime buffer */
static void write_char_to_buffer(char write)
{
	/* If buffer is full, cannot write to it */
	if (is_buffer_full())
		return;

	/* Write character to buffer, increment buffer length and write index */
	runtime_log[write_index] = write;
	buffer_length++;
	write_index++;

	/* Circle write index back to beginning of buffer, if past the end */
	if (write_index == SIZE_OF_BL31_RUNTIME_BUFFER)
		write_index = 0;
}

/* Read one character from runtime buffer */
static char read_char_from_buffer(void)
{
	char read;

	/* Read one character from buffer, increment read index, and decrement buffer length */
	read = runtime_log[read_index];
	read_index++;
	buffer_length--;

	/* Circle read index back to beginning of buffer, if past the end */
	if (read_index == SIZE_OF_BL31_RUNTIME_BUFFER)
		read_index = 0;
	return read;
}

/* Write message to runtime buffer */
void write_to_runtime_buffer(const char *message)
{
	char *tmp = (char *)message;

	/* Obtain lock when available */
	while (lock)
		;
	lock = true;

	/* Write each character to runtime buffer */
	while (*tmp != '\0') {
		write_char_to_buffer(*tmp);
		tmp++;
	}

	/* Write termination character to buffer */
	write_char_to_buffer(GROUP_SEPARATOR);

	/* Unlock */
	lock = false;
}

/* Read messages from runtime buffer */
void read_from_runtime_buffer(char *message, int size)
{
	int index = 0;

	/* Obtain lock when available */
	while (lock)
		;
	lock = true;

	/* Read each character into buffer */
	while (read_index != write_index) {
		if (index >= size)
			break;
		message[index] = read_char_from_buffer();
		index++;
	}

	/* Clear buffer */
	memset(runtime_log, 0, sizeof(runtime_log));
	read_index = write_index;
	buffer_length = 0;

	/* Unlock */
	lock = false;
}
