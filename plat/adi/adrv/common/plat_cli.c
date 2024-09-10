/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>

#include <common/debug.h>
#include <lib/mmio.h>

#include <plat_cli.h>
#include <plat_err.h>
#include <plat_mmap.h>
#include <plat_security.h>
#include <plat_status_reg.h>
#include <plat_wdt.h>

#define MAX_INPUT_COMMAND_LENGTH    250

typedef struct {
	int errno;
	char cmd[MAX_COMMAND_NAME_LENGTH + 1];
} adrv_err_t;

/* Max command length is 100. Adding an extra index so we can ensure input strings are properly terminated before parsing */
static uint8_t input_buffer[MAX_INPUT_COMMAND_LENGTH + 1];
static adrv_err_t adrv_last_cmd_err = {
	0,
	{ '\0' }
};

static void set_command_error(char *cmd, int err)
{
	if (cmd && strcmp(cmd, "getlasterror") && strcmp(cmd, "end")) {
		strlcpy(adrv_last_cmd_err.cmd, cmd, MAX_COMMAND_NAME_LENGTH);
		if (err)
			adrv_last_cmd_err.errno = -1;
		else
			adrv_last_cmd_err.errno = 0;
	}
}

static int add_mmap_region(uintptr_t base, size_t size, unsigned int attr)
{
	uintptr_t base_aligned;
	int rc;
	size_t size_aligned;

	/* Align base addr and size on page boundaries */
	base_aligned = page_align(base, DOWN);
	size_aligned = page_align(size, UP);
	rc = mmap_add_dynamic_region((unsigned long long)base_aligned, base_aligned, size_aligned, attr);

	return rc;
}

static int remove_mmap_region(uintptr_t base, size_t size)
{
	uintptr_t base_aligned;
	int rc;
	size_t size_aligned;

	/* Align base addr and size on page boundaries */
	base_aligned = page_align(base, DOWN);
	size_aligned = page_align(size, UP);
	rc = mmap_remove_dynamic_region((unsigned long long)base_aligned, size_aligned);

	return rc;
}

/* Debug write function. Writes a value of specifed width to the address specified */
static int common_debug_write_function(uint8_t *command_buffer, bool help)
{
	uint64_t width;
	uint64_t address;
	uint64_t data;
	int rc = 0;

	if (help) {
		printf("write <width> <address> <data>     ");
		printf("Writes a (hex)value of specified width (8,16,32,64) to the (hex)address specified.\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &width);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &address);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &data);
		if (command_buffer == NULL)
			return -1;


		if (width != 8 && width != 16 && width != 32 && width != 64) {
			printf("Invalid write width specified. Supported widths: 8, 16, 32, 64\n");
			return -1;
		} else if ((address % (width / 8)) != 0) {
			printf("Address must be aligned to width\n");
			return -1;
		} else {
			rc = add_mmap_region(address, width, MT_DEVICE | MT_RW | MT_SECURE);
			switch (width) {
			case 8:
				mmio_write_8(address, data);
				printf("0x%lx written successfully, command OK\n", data);
				break;
			case 16:
				mmio_write_16(address, data);
				printf("0x%lx written successfully, command OK\n", data);
				break;
			case 32:
				mmio_write_32(address, data);
				printf("0x%lx written successfully, command OK\n", data);
				break;
			case 64:
				mmio_write_64(address, data);
				printf("0x%lx written successfully, command OK\n", data);
				break;
			default:
				printf("Invalid write width specified. Supported widths: 8, 16, 32, 64\n");
				return -1;
			}

			if (rc == 0)
				rc = remove_mmap_region(address, width);
		}
	}
	return rc;
}

/* Debug read function. Reads from a location in memory */
static int common_debug_read_function(uint8_t *command_buffer, bool help)
{
	uint64_t width;
	uint64_t address;
	uint64_t data = 0U;
	int rc = 0;

	if (help) {
		printf("read <width> <address>             ");
		printf("Reads a (hex)value of specified width (8,16,32,64) from the (hex)address specified.\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &width);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &address);
		if (command_buffer == NULL)
			return -1;

		if (width != 8 && width != 16 && width != 32 && width != 64) {
			printf("Invalid write width specified. Supported widths: 8, 16, 32, 64\n");
			return -1;
		} else if ((address % (width / 8)) != 0) {
			printf("Address must be aligned to width\n");
			return -1;
		} else {
			rc = add_mmap_region(address, width, MT_DEVICE | MT_RW | MT_SECURE);
			switch (width) {
			case 8:
				data = mmio_read_8(address);
				printf("Value at %x:0x%lx\n", (uint32_t)address, data);
				break;
			case 16:
				data = mmio_read_16(address);
				printf("Value at %x:0x%lx\n", (uint32_t)address, data);
				break;
			case 32:
				data = mmio_read_32(address);
				printf("Value at %x:0x%lx\n", (uint32_t)address, data);
				break;
			case 64:
				data = mmio_read_64(address);
				printf("Value at %x:0x%lx\n", (uint32_t)address, data);
				break;
			default:
				printf("Invalid read width specified. Supported widths: 8, 16, 32, 64\n");
				return -1;
			}

			if (rc == 0)
				rc = remove_mmap_region(address, width);
		}
	}
	return rc;
}

/* Debug hexdump function. Dumps a block of memory */
static int common_debug_hexdump_function(uint8_t *command_buffer, bool help)
{
	uint64_t size;
	uint64_t width;
	uint64_t address;
	int rc = 0;

	if (help) {
		printf("hexdump <width> <address> <size>   ");
		printf("Reads <size> bytes from the specified (hex)address by using mmio_read_<width> and display in hexadecimal format.\n");
	} else {
		command_buffer = parse_next_param(10, command_buffer, &width);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(16, command_buffer, &address);
		if (command_buffer == NULL)
			return -1;

		command_buffer = parse_next_param(10, command_buffer, &size);
		if (command_buffer == NULL)
			return -1;

		rc = add_mmap_region(address, size, MT_DEVICE | MT_RW | MT_SECURE);

		/* Example:    mmio_read_32    addr    # bytes
		 * >> hexdump      32        20732000    19
		 * 20732000: 00000000500000000000000000000000
		 * 20732010: 00
		 */
		if (width != 8 && width != 32) {
			printf("Invalid regmap width. Supported widths: 8, 32\n");
		} else if ((address % (width / 8)) != 0) {
			printf("Address must be aligned to width\n");
		} else {
			switch (width) {
			case 8:
				while (size > 0) {
					printf("\n%08x: ", (uint32_t)address);
					for (unsigned int i = 0; (i < 16) && (i < size); i++) {
						uint8_t data;

						data = mmio_read_8((uint32_t)address);
						printf("%x%x", (data >> 4) & 0x0f, data & 0x0f);
						address += 1;
					}
					size = (size >= 16) ? size - 16 : 0;
				}
				break;
			case 32:
				while (size > 0) {
					printf("\n%08x: ", (uint32_t)address);
					for (unsigned int i = 0; (i < 4) && (i < ((size + 3) / 4)); i++) {
						uint32_t data;

						data = mmio_read_32((uint32_t)address);
						for (unsigned int j = 0; (j < 4) && (((4 * i) + j) < size); j++) {
							printf("%x%x", (data >> 4) & 0x0f, data & 0x0f);
							data /= 256;
						}
						address += 4;
					}
					size = (size >= 16) ? size - 16 : 0;
				}
				break;
			default:
				printf("Invalid regmap width. Supported widths: 8, 32\n");
				return -1;
			}
		}

		printf("\n");
		if (rc == 0)
			rc = remove_mmap_region(address, width);
	}
	return rc;
}

/* Performs a warm reset of the board */
static int common_reset_function(uint8_t *command_buffer, bool help)
{
	bool ret = true;

	if (help) {
		printf("reset                              ");
		printf("Performs immediate reset of the board.\n");
	} else {
		/* Clear reset cause prior to reset */
		ret = plat_wr_status_reg(RESET_CAUSE, RESET_VALUE);
		if (ret == false) return -1;
		ret = plat_wr_status_reg(RESET_CAUSE_NS, RESET_VALUE);
		if (ret == false) return -1;

		plat_warm_reset();
	}
	/* If we reach here it mean the reset failed */
	return -1;
}

static int common_get_last_error_function(uint8_t *command_buffer, bool help)
{
	if (help) {
		printf("getlasterror                       ");
		printf("Get the last command's execution status and report back error code to console.\n");
	} else {
		if (strlen(adrv_last_cmd_err.cmd))
			printf("Last command '%s' %s with status code = %d.\n", adrv_last_cmd_err.cmd, adrv_last_cmd_err.errno == 0 ? "succeeded" : "failed", adrv_last_cmd_err.errno);
		else
			printf("Last command not found!\n");
	}
	return 0;
}

/* Function for signifying we have reach the end of the command function list.
* Prints out warning that command could not be found for debugging purposes */
static int common_end_function(uint8_t *command_buffer, bool help)
{
	NOTICE("Could not find command in any command list.\n");
	return 0;
}

cli_command_t common_command_list[] = {
	{ "write",	  common_debug_write_function	 },
	{ "read",	  common_debug_read_function	 },
	{ "hexdump",	  common_debug_hexdump_function	 },
	{ "reset",	  common_reset_function		 },
	{ "getlasterror", common_get_last_error_function },
	{ "end",	  common_end_function		 }
};

/* Converts ASCII string to an integer */
static bool atoi(uint8_t *string, uint64_t *data)
{
	uint64_t value = 0u;
	uint64_t digit = 0u;
	uint64_t c = (uint64_t)*string++;
	uint8_t count = 0;

	while (c >= '0' && c <= '9') {
		digit = (uint64_t)(c - '0');
		value = value * 10 + digit;
		c = (uint64_t)*string++;
		count++;
	}

	/* If count == 0, we failed to convert the string for some reason */
	if (count == 0) {
		return false;
	} else {
		*data = value;
		return true;
	}
}

/* Converts ASCII string to hex value. */
static bool atoh(uint8_t *string, uint64_t *data)
{
	uint64_t value = 0u;
	uint64_t digit = 0u;
	uint64_t c = (uint64_t)*string++;
	uint8_t count = 0;

	while ((c >= '0' && c <= '9') ||
	       (c >= 'A' && c <= 'F') ||
	       (c >= 'a' && c <= 'f')) {
		if (c >= '0' && c <= '9')
			digit = (uint64_t)(c - '0');
		else if (c >= 'a' && c <= 'f')
			digit = (uint64_t)(c - 'a') + 10u;
		else if (c >= 'A' && c <= 'F')
			digit = (uint64_t)(c - 'A') + 10u;
		else
			break;
		value = (value << 4u) + digit;
		c = (uint64_t)*string++;
		count++;
	}

	/* If count == 0, we failed to convert the string for some reason */
	if (count == 0) {
		return false;
	} else {
		*data = value;
		return true;
	}
}

/* Parses the next parameter in the command buffer as a base 10(for int) or base 16(for hex) value.
 * The delimiter between parameters is a space, ' '. Prints an error and returns NULL if a paramater was
 * not successfully extracted, otherwise, returns a point to the remaining part of the buffer. */
uint8_t *parse_next_param(uint32_t base, uint8_t *buffer, uint64_t *data)
{
	uint8_t *str;
	uint8_t status;

	/* Check for invalid string */
	if (buffer == NULL) {
		ERROR("Invalid Input Param\n");
		str = 0u;
	} else {
		str = (uint8_t *)strchr((char const *)buffer, ' ');
		if (str != NULL) {
			str++; /* skip over the ' ' delimiter */
			if (base == 10u) {
				status = atoi(str, data);
				if (!status) {
					ERROR("Failed to interpret parameter, check command input\n");
					return NULL;
				}
			} else if (base == 16u) {
				status = atoh(str, data);
				if (!status) {
					ERROR("Failed to interpret parameter, check command input\n");
					return NULL;
				}
			} else {
				ERROR("Invalid Base for Param: %s\n", str);
				return NULL;
			}
		} else {
			WARN("Input parameter was empty\n");
		}
	}
	return str;
}

/* Prints out the help output for each command by calling every command in both lists
 * with the help input set to true. It is up to each command to print out what it thinks
 * the help string should be. */
static void print_help(void)
{
	int i = 0;

	/* Iterate through each command and force help argument to true */
	while (strcmp("end", plat_command_list[i].cmd_name)) {
		plat_command_list[i].cmd_function(input_buffer, true);
		i++;
	}

	i = 0;
	while (strcmp("end", common_command_list[i].cmd_name)) {
		common_command_list[i].cmd_function(input_buffer, true);
		i++;
	}
	/* Exit doesn't have an official function, but we should still print out a listing for it */
	printf("exit                               ");
	printf("Exits the CLI and resumes boot.\n");
}

/* At finding a command in the command list, execute its corresponding function and set its err status to the
 *   global struct variable 'adrv_last_cmd_err'.
 *   This function returns true at command found and executed, otherwise false.
 */
static bool execute_command(uint8_t *command, int cmd_len, cli_command_t *command_list)
{
	int i;
	int err = 0;
	char cmd_name[MAX_COMMAND_NAME_LENGTH + 1] = { '\0' };

	if (!command_list) return false;
	strlcpy(cmd_name, (char *)command, cmd_len + 1);

	/* Search the command list with user input command for a match */
	i = 0;
	while (strcmp("end", command_list[i].cmd_name)) {
		/*****************************************
		* Scan the table until a match is found.
		*****************************************/
		if (strncmp((char *)command, (char *)command_list[i].cmd_name, (size_t)cmd_len) == 0u) {
			/*reset command error status before execution*/
			set_command_error(cmd_name, 0);
			err = command_list[i].cmd_function(command, false);
			set_command_error(cmd_name, err);
			return true;
		}

		i++;
		if (i > MAX_COMMAND_COUNT) {
			printf("Command not found!\n");
			return false;
		}
	}

	/* If we reach here it means we have reached the "end" entry in the platform specific command list without finding a match */
	command_list[i].cmd_function(command, false);
	return false;
}

/* Parses the command in the buffer and attempts to find its corresponding function in the command lists. */
static void parse_command(uint8_t *command)
{
	uint8_t *find_equal;
	int cmd_len;

	find_equal = (uint8_t *)strchr((char *)command, ' ');

	/* Simple command with no input parameters to process. */
	if (!find_equal)
		cmd_len = strlen((char *)command) - 1u;
	else
		cmd_len = find_equal - command;

	if (!execute_command(command, cmd_len, plat_command_list))
		execute_command(command, cmd_len, common_command_list);
}

/* Main cli function. Sits here infinitely accepting command until the exit command is given or the board is reset */
void plat_enter_cli(void)
{
	uint8_t keypress = 0x00;
	uint8_t lastKeypress = 0x00;
	uint16_t pos = 0U;

	/* Disable WDT */
	plat_secure_wdt_stop();

#ifndef RMA_CLI
	/* Do security setup so writes/reads don't get blocked by various things */
	plat_security_setup();
#endif

	/*Clear out input buffer */
	memset(input_buffer, 0x00, MAX_INPUT_COMMAND_LENGTH);
	printf("\nEnter command: ");
	while (1) {
		keypress = console_getc();
		/* Got garbage, normally this shouldn't happen */
		if (keypress == 0) {
			continue;
		}
		/* BACKSPACE/DEL key, in case the user wants to change the command */
		else if ((keypress == '\b') || (keypress == 0x7F)) {
			if (pos != 0) {
				/* For backspace, delete the last char by sending a backspace,
				 * sending a space, then sending a backspace.
				 */
				console_putc('\b');
				console_putc(' ');
				console_putc('\b');
				input_buffer[pos] = 0x00;
				pos--;
			}
		} else {
			console_putc(keypress);

			/* Save the character in buffer for later parsing & inc position in buffer */
			if (pos < MAX_INPUT_COMMAND_LENGTH) {
				input_buffer[pos] = keypress;
				pos++;
			}

			/* Check when user enters "ENTER". Start to process command seq. */
			if ((keypress == '\n') || (keypress == '\r')) {
				/* Insert a LF if a CR was received. If the user's terminal is sending
				 * CRLF for ENTER, this will result in an extra LF, but this is the best
				 * we can do (it is still functional).
				 */
				if (keypress == '\r')
					console_putc('\n');
				if (pos == 1) {
					/* If the user sent an empty command, print out the command prompt.
					 * If the user's terminal is using CRLF for ENTER, the LF will be
					 * accidentally interpreted as an empty command. To work around this,
					 * if the current character is LF and the last character was CR, don't
					 * print the start character.
					 */
					if ((keypress != '\n') || (lastKeypress != '\r'))
						printf("No command was entered, try again\n");
				} else {
					/* Either process the command or run help */
					if (strncmp((char *)input_buffer, "help", 4u) == 0u) {
						/* Parse the command */
						print_help();
					} else if (strncmp((char *)input_buffer, "exit", 4u) == 0u) {
						/* Re-enable WDT and exit CLI */
						plat_secure_wdt_start();
						printf("Continuing boot\n");
						break;
					} else {
						/* Ensure input buffer is null-terminated no matter what input we have */
						input_buffer[MAX_INPUT_COMMAND_LENGTH] = 0x0;
						parse_command(input_buffer);
					}
				}
				/* Clear out the input buffer and prompt for next command */
				pos = 0;
				memset(input_buffer, 0x00, MAX_INPUT_COMMAND_LENGTH);
				printf("\nEnter command: ");
			}
		}
		lastKeypress = keypress;
	}
}
