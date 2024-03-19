/*
 * Copyright (c) 2022, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <plat_sip_svc.h>
#include <plat_wdt.h>
#include <plat_wdt_svc.h>
#include <platform_def.h>
#include <tools_share/uuid.h>

typedef enum {
	INIT		= 0,
	SET_TIMEOUT	= 1,
	ENABLE		= 2,
	PET		= 3,
	GET_TIMELEFT	= 4,
} func_id_t;

/*
 * Watchdog service SMC handler
 */
uintptr_t plat_wdt_smc_handler(unsigned int smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags)
{
	func_id_t id;

	if (smc_fid == PLAT_SIP_SVC_WDT) {
		id = (func_id_t)x1;
		switch (id) {
		case INIT:
			/* Watchdog is assumed already running (from BL1).
			 * Just return the configured timeout to the caller. */
			VERBOSE("WDT service: INIT\n");
			SMC_RET3(handle, SMC_OK, PLAT_WDT_TIMEOUT_MIN_SEC, PLAT_WDT_TIMEOUT_MAX_SEC);
			break;
		case SET_TIMEOUT:
			/* Ignore SET_TIMEOUT command if timeout value is outside the min/max range */
			if ((x2 >= PLAT_WDT_TIMEOUT_MIN_SEC) && (x2 <= PLAT_WDT_TIMEOUT_MAX_SEC)) {
				VERBOSE("WDT service: SET_TIMEOUT\n");
				plat_secure_wdt_refresh(x2);
				SMC_RET1(handle, SMC_OK);
			} else {
				WARN("WDT service: Timeout value is outside of the min/max range\n");
				SMC_RET1(handle, SMC_UNK);
			}
			break;
		case ENABLE:
			/* Ignore enable and disable commands.
			 * Assume BL1 already enabled the WDT, and we don't want
			 * non-secure software to disable the WDT. */
			WARN("WDT service: ENABLE/DISABLE not supported. WDT is enabled at boot and cannot be disabled.\n");
			SMC_RET1(handle, SMC_OK);
			break;
		case PET:
			VERBOSE("WDT service: PET\n");
			plat_secure_wdt_ping();
			SMC_RET1(handle, SMC_OK);
			break;
		case GET_TIMELEFT:
			/* GET_TIMELEFT support is optional and not supported here */
			WARN("WDT service: GET_TIMELEFT not supported\n");
			SMC_RET1(handle, SMC_UNK);
			break;
		default:
			WARN("WDT service: Unexpected command\n");
			SMC_RET1(handle, SMC_UNK);
			break;
		}
	}

	WARN("WDT service: Unexpected FID\n");
	SMC_RET1(handle, SMC_UNK);
}
