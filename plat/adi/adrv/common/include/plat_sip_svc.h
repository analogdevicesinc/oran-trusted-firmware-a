/*
 * Copyright (c) 2016-2019,2021,2025, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_SIP_SVC_H
#define PLAT_SIP_SVC_H

#include <stdint.h>

#include <lib/cassert.h>
#include <lib/utils_def.h>

/* SMC function IDs for SiP Service queries */

/* General queries, defined in SMCCC */
/* CALL_COUNT (0x8200FF00) is deprecated in SMCCC v1.2 and above */
#define PLAT_SIP_SVC_UID                U(0x8200FF01)
/*					U(0x8200FF02) is reserved */
#define PLAT_SIP_SVC_VERSION            U(0x8200FF03)

/* Watchdog Timer Service. Set to match default ID in Linux arm_smc_wdt.c driver. */
#define PLAT_SIP_SVC_WDT                U(0x82003D06)

/* SMC64/HVC64 SiP fast calling convention - bottom 2 bytes are the function ID */
/* For more info: https://developer.arm.com/documentation/den0028/latest */
#define PLAT_SIP_SVC_PINCTRL            U(0xC2000001)
#define PLAT_SIP_SVC_PINTMUX            U(0xC2000002)
#define PLAT_SIP_SVC_LOG                U(0xC2000003)

/* Max function ID used by the common service.
 * IDs beyond this number, up to the SMCCC reserved
 * number (0xFF00), are available for use by the
 * platform-specific code.
 */
#define PLAT_SIP_SVC_MAX              U(0xC20000FF)

/* ADI SiP Service Calls version numbers */
#define PLAT_SIP_SVC_VERSION_MAJOR              U(0x0)
#define PLAT_SIP_SVC_VERSION_MINOR              U(0x1)

#define plat_is_plat_smc(_fid) ((_fid) > PLAT_SIP_SVC_MAX)

/* Platform-specific SiP/SMC handler. To be implemented in platform-specific code. */
uintptr_t plat_smc_handler(unsigned int smc_fid, u_register_t x1, u_register_t x2, u_register_t x3, u_register_t x4, void *cookie, void *handle, u_register_t flags);

#endif /* PLAT_SIP_SVC_H */
