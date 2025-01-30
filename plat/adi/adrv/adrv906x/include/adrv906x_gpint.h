/*
 * Copyright (c) 2025, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_GPINT_H
#define ADRV906X_GPINT_H

#include <stdbool.h>
#include <stdint.h>

#include <adrv906x_gpint_def.h>

#define GPINT0  (0)
#define GPINT1  (1)
#define GPINT_INTS_PER_WORD     (48)
#define TOTAL_GPINTS                    (96)

struct gpint_settings {
	uint64_t upper_word;
	uint64_t lower_word;
	uint64_t upper_word_route_nonsecure;
	uint64_t lower_word_route_nonsecure;
};

void adrv906x_gpint_init(uintptr_t base_address);
void adrv906x_gpint_get_status(uintptr_t gpint_base_addr, struct gpint_settings *settings);
void adrv906x_gpint_get_masked_status(uintptr_t gpint_base_addr, struct gpint_settings *settings, uint32_t gpint);
void adrv906x_gpint_enable(uintptr_t gpint_base_addr, uint32_t gpint, struct gpint_settings *settings);
void adrv906x_gpint_disable(uintptr_t gpint_base_addr, uint32_t gpint, struct gpint_settings *settings);
void adrv906x_gpint_warm_reset_enable(void);
bool adrv906x_gpint_is_nonsecure(bool upper_word, uint64_t mask);
void adrv906x_gpint_set_routing(struct gpint_settings *settings);
void adrv906x_gpint_print_status(const struct gpint_settings *settings);

#endif /* ADRV906X_GPINT_H */
