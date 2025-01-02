/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_ERR_H
#define PLAT_ERR_H

void plat_panic_reset_cause(void);
int plat_warm_reset(void);
void plat_error_message(char *fmt, ...);
void plat_warn_message(char *fmt, ...);

#endif
