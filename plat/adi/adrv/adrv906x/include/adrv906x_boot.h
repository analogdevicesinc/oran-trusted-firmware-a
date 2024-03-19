/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_BOOT_H
#define ADRV906X_BOOT_H

typedef enum {
	CLK_SEL_DEV_CLK		= 0,
	CLK_SEL_ROSC_CLK	= 1
} boot_info_clk_sel_t;

#define ADRV906X_TEST_CONTROL_ENTER_CLI   8

/* Deinitialize/disable the current boot device */
void plat_deinit_boot_device(void);

/* Determine if current boot device is initialized */
bool plat_is_boot_dev_initialized(void);

/* Get the value of the current clock selection pin (devclk or ROSC) */
boot_info_clk_sel_t plat_get_boot_clk_sel(void);

/* Determine if devclk clock source is available */
bool plat_is_devclk_available(void);

/* Get the current value of test_enable*/
bool plat_get_test_enable(void);

/* Get the current value of test_control*/
int plat_get_test_control(void);

#endif /* ADRV906X_BOOT_H */
