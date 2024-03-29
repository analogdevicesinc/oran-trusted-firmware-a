/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <adrv906x_sip_svc.h>
#include <common/debug.h>
#include <common/runtime_svc.h>

uintptr_t plat_smc_handler(unsigned int smc_fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4, void *cookie, void *handle, u_register_t flags)
{
	switch (smc_fid) {
	/* TODO: Remove this when real functions are defined */
	case ADRV906X_SIP_SVC_TEST:
		SMC_RET1(handle, 0xDEADBEEF);

	default:
		WARN("Unimplemented SiP Service Call: 0x%x \n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}
