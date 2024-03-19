/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <common/debug.h>
#include <drivers/console.h>
#include <errno.h>
#include <lib/mmio.h>
#include <platform.h>

#include <plat_err.h>
#include <plat_status_reg.h>
#include <plat_wdt.h>

void plat_error_handler(int err)
{
	/*
	 * Set the RESET_CAUSE boot register with the appropriate value
	 * Errno error's mapped to reset causes
	 * If errno is not mapped, reset cause defaults to OTHER_RESET_CAUSE
	 */

	switch (-err) {
	case EAUTH:
		plat_wr_status_reg(RESET_CAUSE, IMG_VERIFY_FAIL);
		break;
	default:
		plat_wr_status_reg(RESET_CAUSE, OTHER_RESET_CAUSE);
	}

	console_flush();
	plat_warm_reset();

	while (1)
		;
}

void plat_panic_reset_cause(void)
{
	/* Set the RESET_CAUSE boot register with the appropriate value for panic case */
	plat_wr_status_reg(RESET_CAUSE, OTHER_RESET_CAUSE);
}

void plat_halt_handler(void)
{
	ERROR("Unable to recover, boot halted\n");

	plat_secure_wdt_stop();

	while (1)
		;
}
