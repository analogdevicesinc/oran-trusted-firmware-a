/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_ERRNO_H
#define PLAT_ERRNO_H

#include <lib/libc/errno.h>

#define ADI_ERRNO(x) (ELAST + x)

#define ECECC ADI_ERRNO(1)      /* Cache ECC Error */
#define EDECC ADI_ERRNO(2)      /* DRAM ECC Error */

#endif
