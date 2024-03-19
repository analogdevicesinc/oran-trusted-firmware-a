/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TRANSMUTER_DEF_H__
#define __TRANSMUTER_DEF_H__

#include <stdint.h>

typedef struct {
	uint16_t intID;
	uint16_t transmuterPin;
} transmuter_pin_pair_t;

#define TRANSMUTER_INTERRUPT_PINS 332
#define TRANSMUTER_TRU_PINS 48

extern const transmuter_pin_pair_t transmuter_interrupt_lookup[TRANSMUTER_INTERRUPT_PINS];
extern const transmuter_pin_pair_t transmuter_tru_lookup[TRANSMUTER_TRU_PINS];

#endif /* __TRANSMUTER_DEF_H__ */
