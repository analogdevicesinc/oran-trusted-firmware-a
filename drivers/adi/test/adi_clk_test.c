/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include <drivers/adi/adrv906x/clk.h>

#include <adrv906x_def.h>
#include <plat_console.h>

/************************** Test INSTRUCTIONS ***************************/
/* This is a test framework to verify the functionality of clock driver.
 * By default, this framework is not included in TF-A build.
 * Do the following before attempting to test the driver.
 * 1. Add the following in drivers/adi/test/test_framework.c
 *    -> extern void adi_clk_test(); or define the prototype
 *       of adi_clk_test() in a suitable header and include
 *       the header in test_framework.c
 *    -> Call adi_clk_test() inside test_main()
 * 2. Include this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 * 3. If needed, add more clk tests and define each error code
 *    and invoke them in adi_clk_test().
 */

/* error code needs to be bitwise exclusive to notify each error in the result */
#define TEST_NO_ERROR                           (0x00000000)
#define TEST_CLK_SRC_SWITCH_DEVCLK_ERROR        (0x00000001)
#define TEST_CLK_SRC_SWITCH_ROSC_ERROR          (0x00000002)

static int test_clk_switch(void)
{
	int ret = TEST_NO_ERROR;

	/* 1. test clk src switch between devclk and ROSC,
	 *    and test ROSC on/off
	 */
	printf("Test clk src switch and ROSC on/off\n");
	/* 1-1. test switching clk src from rosc to dev clk
	 *      boot device isn't inited yet, only dis/enable
	 *      console driver.
	 *      ROSC is turned off in clk_set_src().
	 */
	plat_console_boot_end();
	clk_set_src(CLK_CTL, CLK_SRC_DEVCLK);
	plat_console_boot_init();

	if (clk_get_src(CLK_CTL) != CLK_SRC_DEVCLK) {
		printf("Switching clk src from rosc to devclk failed\n");
		ret |= TEST_CLK_SRC_SWITCH_DEVCLK_ERROR;
	}

	/* 1-2. test switching back to rosc from dev clk. */
	plat_console_boot_end();
	clk_set_src(CLK_CTL, CLK_SRC_ROSC);
	plat_console_boot_init();

	if (clk_get_src(CLK_CTL) != CLK_SRC_ROSC) {
		printf("Switching clk src from devclk to rosc failed\n");
		ret |= TEST_CLK_SRC_SWITCH_ROSC_ERROR;
	}

	return ret;
}

int adi_clk_test(void)
{
	return test_clk_switch();
}
