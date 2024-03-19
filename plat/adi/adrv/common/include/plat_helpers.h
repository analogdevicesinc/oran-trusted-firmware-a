/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_HELPERS_H
#define PLAT_HELPERS_H

unsigned int plat_calc_core_pos(u_register_t mpidr);
int plat_validate_ns_entrypoint(uintptr_t entrypoint);

#endif /* PLAT_HELPERS_H */
