/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_CONSOLE_H
#define PLAT_CONSOLE_H

/* Console utility functions */
void plat_console_boot_init(void);
void plat_console_boot_end(void);
void plat_console_runtime_init(void);
void plat_console_runtime_end(void);

#endif /* PLAT_CONSOLE_H */
