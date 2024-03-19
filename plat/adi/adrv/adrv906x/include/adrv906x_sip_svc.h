/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_SIP_SVC_H
#define ADRV906X_SIP_SVC_H

#include <plat_sip_svc.h>

/* SMC function IDs for SiP Service queries */
/* TODO: Remove this when real functions are defined */
#define ADRV906X_SIP_SVC_TEST     U(0xC2000100)

/* If the common function ID range has moved, we need to know about it */
CASSERT(PLAT_SIP_SVC_MAX == U(0xC20000FF), plat_max_sip_has_moved);

#endif /* ADRV906X_SIP_SVC_H */
