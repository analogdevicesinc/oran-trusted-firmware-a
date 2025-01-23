/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_RUNTIME_LOG_SVC_H
#define PLAT_RUNTIME_LOG_SVC_H

#include <plat_sip_svc.h>

uintptr_t plat_runtime_log_smc_handler(unsigned int smc_fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4, void *cookie, void *handle, u_register_t flags);

#endif /* PLAT_RUNTIME_LOG_SVC_H */
