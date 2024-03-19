/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>

#include <drivers/adi/adrv906x/transmuter.h>
#include "transmuter_def.h"
#include "transmuter_regmap.h"

/*Get the offset of the register holding the id by dividing by 32(>>5) to get the register number, then multiplying by 4(<<2) to get the hex address*/
#define TRANSMUTER_REGISTER_OFFSET(id) ((id >> 5) << 2)
/*Since each register holds 32 bits and thus 32 ids, finding the bit shift is done by AND'ing with 31(0x11111), like the GIC does*/
#define TRANSMUTER_BIT_SHIFT(id) (id & ((1U << 5) - 1U))

/*This function probes the lookup table for the corresponding transmuter bit for a given INTID, return -1 if it could not be found*/
static int get_transmuter_bit_number(int int_id)
{
	int i;

	for (i = 0; i < TRANSMUTER_INTERRUPT_PINS; i++)
		if (transmuter_interrupt_lookup[i].intID == int_id)
			return transmuter_interrupt_lookup[i].transmuterPin;

	return TRANSMUTER_UNKNOWN_INT_ID;
}

/*Sets the transmuter bit in the given register set(ENABLE, POS_MASK, etc.)*/
static void set_transmuter_bit(uintptr_t base_addr_transmuter, uintptr_t register_base, int transmuter_bit)
{
	uint32_t data = 0U;
	uintptr_t address = base_addr_transmuter + register_base + TRANSMUTER_REGISTER_OFFSET(transmuter_bit);

	data = mmio_read_32(address);
	data |= (1U << TRANSMUTER_BIT_SHIFT(transmuter_bit));
	mmio_write_32(address, data);
}

/*Clears the transmuter bit in the given register set(ENABLE, POS_MASK, etc.)*/
static void clear_transmuter_bit(uintptr_t base_addr_transmuter, uintptr_t register_base, int transmuter_bit)
{
	uint32_t data = 0U;
	uintptr_t address = base_addr_transmuter + register_base + TRANSMUTER_REGISTER_OFFSET(transmuter_bit);

	data = mmio_read_32(address);
	data &= ~(1U << TRANSMUTER_BIT_SHIFT(transmuter_bit));
	mmio_write_32(address, data);
}

/*This function sets or clears the given int_id's transmuter bit based on the action passed in. Actions are set/clear a certain register, definitions in transmuter.h*/
int transmuter_change_bit(uintptr_t base_addr_transmuter, int int_id, transmuter_action_t action)
{
	int transmuter_bit = get_transmuter_bit_number(int_id);

	/*Bail out early with an error message if we can't find the INTID in the lookup table*/
	if (transmuter_bit == -1) {
		ERROR("Could not find INTID %d in the transmuter lookup table\n", int_id);
		return TRANSMUTER_UNKNOWN_INT_ID;
	}

	switch (action) {
	case SET_ENABLE_MASK:
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_ENABLE_BASE, transmuter_bit);
		break;
	case SET_LEVEL_EDGE_MASK:
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_LEVEL_EDGE_BASE, transmuter_bit);
		break;
	case SET_POS_MASK:
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_POS_MASK_BASE, transmuter_bit);
		/*POS Mask and NEG Mask are mutually exclusive, so clear the NEG MasK bit in case it was set*/
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_NEG_MASK_BASE, transmuter_bit);
		break;
	case SET_NEG_MASK:
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_NEG_MASK_BASE, transmuter_bit);
		/*POS Mask and NEG Mask are mutually exclusive, so clear the POS MasK bit in case it was set*/
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_POS_MASK_BASE, transmuter_bit);
		break;
	case SET_TRIGGER_MASK:
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_TRIGGER_BASE, transmuter_bit);
		break;
	case CLEAR_ENABLE_MASK:
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_ENABLE_BASE, transmuter_bit);
		break;
	case CLEAR_LEVEL_EDGE_MASK:
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_LEVEL_EDGE_BASE, transmuter_bit);
		break;
	case CLEAR_POS_MASK:
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_POS_MASK_BASE, transmuter_bit);
		/*POS Mask and NEG Mask are mutually exclusive, so clear the POS mask bit and set the NEG mask */
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_NEG_MASK_BASE, transmuter_bit);
		break;
	case CLEAR_NEG_MASK:
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_NEG_MASK_BASE, transmuter_bit);
		/*POS Mask and NEG Mask are mutually exclusive, so clear the NEG mask bit and set the POS mask */
		set_transmuter_bit(base_addr_transmuter, TRANSMUTER_POS_MASK_BASE, transmuter_bit);
		break;
	case CLEAR_TRIGGER_MASK:
		clear_transmuter_bit(base_addr_transmuter, TRANSMUTER_TRIGGER_BASE, transmuter_bit);
		break;
	default:
		ERROR("Not a valid transmuter action: %d\n", action);
		return TRANSMUTER_UNKNOWN_ACTION;
	}

	return TRANSMUTER_NO_ERROR;
}

/*Gets the status transmuter bit for the given int_id.
 * Affected by the enable bit for the id, i.e. if enable is 0, this bit will always return false*/
int transmuter_get_status_bit(uintptr_t base_addr_transmuter, int int_id)
{
	int transmuter_bit;
	uintptr_t address;

	transmuter_bit = get_transmuter_bit_number(int_id);
	if (transmuter_bit < 0) {
		ERROR("Could not find transmuter bit for given int id:%d\n", int_id);
		return TRANSMUTER_UNKNOWN_INT_ID;
	}

	address = base_addr_transmuter + TRANSMUTER_STATUS_BASE + TRANSMUTER_REGISTER_OFFSET(transmuter_bit);

	return (mmio_read_32(address) & (1U << TRANSMUTER_BIT_SHIFT(transmuter_bit))) >> TRANSMUTER_BIT_SHIFT(transmuter_bit);
}

/*Gets the raw status transmuter bit for given int_id. This reports the status regardless of the status of the enable bit for the id */
int transmuter_get_raw_status_bit(uintptr_t base_addr_transmuter, int int_id)
{
	int transmuter_bit;
	uintptr_t address;

	transmuter_bit = get_transmuter_bit_number(int_id);
	if (transmuter_bit < 0) {
		ERROR("Could not find transmuter bit for given int id:%d\n", int_id);
		return TRANSMUTER_UNKNOWN_INT_ID;
	}

	address = base_addr_transmuter + TRANSMUTER_RAW_STATUS_BASE + TRANSMUTER_REGISTER_OFFSET(transmuter_bit);

	return (mmio_read_32(address) & (1U << TRANSMUTER_BIT_SHIFT(transmuter_bit))) >> TRANSMUTER_BIT_SHIFT(transmuter_bit);
}

/*Gets the edge status bit for the given int id.
 * This reports which edge triggered the interrupt, 0 for falling edge, 1 for rising edge. Useful for toggle interrupt sources*/
int transmuter_get_edge_status_bit(uintptr_t base_addr_transmuter, int int_id)
{
	int transmuter_bit;
	uintptr_t address;

	transmuter_bit = get_transmuter_bit_number(int_id);
	if (transmuter_bit < 0) {
		ERROR("Could not find transmuter bit for given int id:%d\n", int_id);
		return TRANSMUTER_UNKNOWN_INT_ID;
	}

	address = base_addr_transmuter + TRANSMUTER_EDGE_STATUS_BASE + TRANSMUTER_REGISTER_OFFSET(transmuter_bit);

	return (mmio_read_32(address) & (1U << TRANSMUTER_BIT_SHIFT(transmuter_bit))) >> TRANSMUTER_BIT_SHIFT(transmuter_bit);
}
