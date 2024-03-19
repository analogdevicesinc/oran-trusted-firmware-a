/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_PINCTRL_SVC_H
#define PLAT_PINCTRL_SVC_H

#include <stdint.h>

#include <plat_sip_svc.h>



uintptr_t plat_pinctrl_smc_handler(unsigned int smc_fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4, void *cookie, void *handle, u_register_t flags);

#endif
