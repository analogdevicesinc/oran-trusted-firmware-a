/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TRANSMUTER_H
#define TRANSMUTER_H

typedef enum {
	SET_ENABLE_MASK,
	SET_LEVEL_EDGE_MASK,
	SET_POS_MASK,
	SET_NEG_MASK,
	SET_TRIGGER_MASK,
	CLEAR_ENABLE_MASK,
	CLEAR_LEVEL_EDGE_MASK,
	CLEAR_POS_MASK,
	CLEAR_NEG_MASK,
	CLEAR_TRIGGER_MASK,
} transmuter_action_t;

#define TRANSMUTER_NO_ERROR 0
#define TRANSMUTER_UNKNOWN_INT_ID -1
#define TRANSMUTER_UNKNOWN_ACTION -2

int transmuter_change_bit(uintptr_t base_addr_transmuter, int int_id, transmuter_action_t action);
int transmuter_get_status_bit(uintptr_t base_addr_transmuter, int int_id);
int transmuter_get_raw_status_bit(uintptr_t base_addr_transmuter, int int_id);
int transmuter_get_edge_status_bit(uintptr_t base_addr_transmuter, int int_id);

#endif /* TRANSMUTER_H */
