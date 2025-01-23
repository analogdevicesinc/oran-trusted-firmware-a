/*
 * Copyright (c) 2016-2019,2021,2025, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <tools_share/uuid.h>

#include <plat_err.h>
#include <plat_pinctrl_svc.h>
#include <plat_pintmux_svc.h>
#include <plat_runtime_log_svc.h>
#include <plat_sip_svc.h>
#include <plat_wdt_svc.h>

/* ADI SiP Service UUID
 * Generated with: uuidgen -r
 */
DEFINE_SVC_UUID2(adi_sip_svc_uid,
		 0x3ebb9653, 0x40f5, 0x4d2d, 0x94, 0x60,
		 0xb1, 0xf5, 0x27, 0x4b, 0xba, 0x80);

static int sip_setup(void)
{
	return 0;
}

/*
 * This function handles platform SiP Calls
 */
static uintptr_t sip_handler(unsigned int smc_fid,
			     u_register_t x1,
			     u_register_t x2,
			     u_register_t x3,
			     u_register_t x4,
			     void *cookie,
			     void *handle,
			     u_register_t flags)
{
	/* Allow platform-specific handling of SiP first */
	if (plat_is_plat_smc(smc_fid))
		return plat_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags);

	switch (smc_fid) {
	case PLAT_SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, adi_sip_svc_uid);

	case PLAT_SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, PLAT_SIP_SVC_VERSION_MAJOR, PLAT_SIP_SVC_VERSION_MINOR);

	case PLAT_SIP_SVC_WDT:
		SMC_RET0(plat_wdt_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags));

	case PLAT_SIP_SVC_PINCTRL:
		SMC_RET0(plat_pinctrl_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags));

	case PLAT_SIP_SVC_PINTMUX:
		SMC_RET0(plat_pintmux_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags));

	case PLAT_SIP_SVC_LOG:
		SMC_RET0(plat_runtime_log_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle, flags));

	default:
		plat_runtime_warn_message("Unimplemented SiP Service Call: 0x%x ", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}


/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	plat_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	sip_setup,
	sip_handler
	);
