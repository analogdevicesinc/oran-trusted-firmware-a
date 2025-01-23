/*
 * Copyright (c) 2025, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/runtime_svc.h>
#include <lib/smccc.h>

#include <plat_runtime_log.h>
#include <plat_runtime_log_svc.h>
#include <plat_sip_svc.h>
#include <platform_def.h>

/*
 * Runtime log service SMC handler
 */
uintptr_t plat_runtime_log_smc_handler(unsigned int smc_fid,
				       u_register_t x1,
				       u_register_t x2,
				       u_register_t x3,
				       u_register_t x4,
				       void *cookie,
				       void *handle,
				       u_register_t flags)
{
	uint32_t buffer_addr = x1;
	uint32_t size = x2;

	/* Return if not a secure caller */
	if (!is_caller_secure(flags))
		SMC_RET1(handle, SMC_UNK);

	/* Get BL31 runtime buffer and flush cache */
	read_from_runtime_buffer((char *)(uintptr_t)buffer_addr, size);
	flush_dcache_range((uintptr_t)buffer_addr, size);

	SMC_RET1(handle, SMC_OK);
}
