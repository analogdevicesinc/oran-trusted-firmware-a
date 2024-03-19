/*
 * Copyright (c) 2018-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/arm/pl011.h>
#include <drivers/console.h>

#include <adrv906x_device_profile.h>
#include <plat_console.h>
#include <plat_pinctrl.h>
#include <platform_def.h>

/* Define pinmux settings for boot UART */
#if PLAT_BOOT_UART_BASE == PL011_0_BASE
#define BOOT_UART_PIN_GRP uart0_pin_grp
#define BOOT_UART_PIN_GRP_MEMBERS uart0_pin_grp_members
#else
#error "PLAT_BOOT_UART_BASE can only be PL011_0_BASE"
#endif

/* Define pinmux settings for runtime UART */
#if PLAT_RUN_UART_BASE == PL011_0_BASE
#define RUN_UART_PIN_GRP uart0_pin_grp
#define RUN_UART_PIN_GRP_MEMBERS uart0_pin_grp_members
#elif PLAT_RUN_UART_BASE == PL011_1_BASE
#define RUN_UART_PIN_GRP uart1_pin_grp
#define RUN_UART_PIN_GRP_MEMBERS uart1_pin_grp_members
#else
#error "PLAT_RUN_UART_BASE can only be PL011_0_BASE or PL011_1_BASE"
#endif

/*******************************************************************************
 * Functions that set up the console
 ******************************************************************************/
static console_t boot_console;
static console_t runtime_console;

extern const plat_pinctrl_settings uart0_pin_grp[];
extern const size_t uart0_pin_grp_members;
extern const plat_pinctrl_settings uart1_pin_grp[];
extern const size_t uart1_pin_grp_members;

/* Initialize the console to provide early debug support */
void __init plat_console_boot_init(void)
{
	plat_secure_pinctrl_set_group(BOOT_UART_PIN_GRP, BOOT_UART_PIN_GRP_MEMBERS, true, PINCTRL_BASE);

	int rc = console_pl011_register(PLAT_BOOT_UART_BASE,
					clk_get_freq(CLK_CTL, CLK_ID_SYSCLK),
					PLAT_CONSOLE_BAUDRATE,
					&boot_console);

	if (rc == 0)
		/*
		 * The crash console doesn't use the multi console API, it uses
		 * the core console functions directly. It is safe to call panic
		 * and let it print debug information.
		 */
		panic();

	console_set_scope(&boot_console, CONSOLE_FLAG_BOOT | CONSOLE_FLAG_CRASH);
}

void plat_console_boot_end(void)
{
	console_flush();
	(void)console_unregister(&boot_console);
}

/* Initialize the runtime console */
void plat_console_runtime_init(void)
{
	plat_secure_pinctrl_set_group(RUN_UART_PIN_GRP, RUN_UART_PIN_GRP_MEMBERS, true, PINCTRL_BASE);

	int rc = console_pl011_register(PLAT_RUN_UART_BASE,
					clk_get_freq(CLK_CTL, CLK_ID_SYSCLK),
					PLAT_CONSOLE_BAUDRATE,
					&runtime_console);

	if (rc == 0)
		panic();

	console_set_scope(&runtime_console, CONSOLE_FLAG_RUNTIME | CONSOLE_FLAG_CRASH);
}

void plat_console_runtime_end(void)
{
	console_flush();
}
