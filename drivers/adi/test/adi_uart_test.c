/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <lib/utils.h>
#include <drivers/console.h>

// This function is used to test if uart can receive input;
// people can add calling of this function from test_main manually.
int test_protium_uart_rx(void)
{
	printf("Inside test for Protium Uart Rx\n");
	printf("please input char, space ends\n");

	int c = 0;
	int abort = 0;

	while (abort == 0) {
		c = console_getc();
		if (c == '\n')
			printf("\n");
		else if (c == ' ')
			abort = 1;
		else
			console_putc(c);
	}
	printf("bye\n");

	return 0;
}
