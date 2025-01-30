/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_ERRNO_H
#define PLAT_ERRNO_H

#include <lib/libc/errno.h>

#define ADI_ERRNO(x) (ELAST + x)

#define ECECC ADI_ERRNO(1)      /* Cache ECC Error */
#define EDECC ADI_ERRNO(2)      /* DRAM ECC Error */
#define EDINIT ADI_ERRNO(3)     /* DRAM Init Error */
#define EMCS ADI_ERRNO(4)       /* MCS Failure */
#define EMBCAL ADI_ERRNO(5)     /* MBias Cal Failure */

#endif
