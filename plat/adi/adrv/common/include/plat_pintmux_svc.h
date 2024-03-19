/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLAT_PINTMUX_SVC_H__
#define __PLAT_PINTMUX_SVC_H__

#include <plat_sip_svc.h>

uintptr_t plat_pintmux_smc_handler(unsigned int smc_fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4, void *cookie, void *handle, u_register_t flags);

#endif /* __PLAT_PINTMUX_SVC_H__ */
