/*
 * Copyright (c) 2023, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bl31/interrupt_mgmt.h>
#include <common/debug.h>
#include <lib/cassert.h>
#include <lib/mmio.h>

#include <drivers/adi/adrv906x/transmuter.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <plat/common/platform.h>
#include <plat_pinctrl.h>
#include <plat_pintmux.h>

#define PINTMUX_LANE_DISABLED 0x7E
#define MAX_GPIO_TO_GIC_SYNC_LANES 32
#define PINTMUX_MAPPING_BAD_ADDRESS 0
static uint32_t pintmux_lane_map[MAX_GPIO_TO_GIC_SYNC_LANES];
static uint32_t pintmux_secondary_lane_map[MAX_GPIO_TO_GIC_SYNC_LANES];

/**
 * Returns the associated pinctrl base address with the given pintmux base address
 */
static uintptr_t plat_get_pintmux_to_pinctrl_mapping(uintptr_t base_addr_pintmux)
{
	if (base_addr_pintmux == PINTMUX_BASE) {
		return PINCTRL_BASE;
	} else if (base_addr_pintmux == SEC_PINTMUX_BASE) {
		if (!plat_get_dual_tile_enabled()) {
			/*Can only try to set up secondary pinctrl if secondary actually exists*/
			WARN("PINTMUX: Secondary tile must be enabled to use secondary base \n");
			return PINTMUX_MAPPING_BAD_ADDRESS;
		}
		return SEC_PINCTRL_BASE;
	} else {
		return PINTMUX_MAPPING_BAD_ADDRESS;
	}
}

/**
 * Returns the transmuter signal associated with the given lane's IRQ
 */
static uint32_t plat_pintmux_lane_to_transmute_signal(unsigned int lane)
{
	if (lane < 24)
		return IRQ_GPIO_TO_GIC_SYNC_0 + lane;
	if (lane < MAX_GPIO_TO_GIC_SYNC_LANES)
		return IRQ_GPIO_TO_GIC_SYNC_24 - 24 + lane;
	return 0;
}

static int plat_pintmux_map_update(unsigned int lane, uint32_t val, bool pos_mask, uintptr_t base_addr)
{
	uintptr_t addr;
	uintptr_t transmuter_addr;
	int transmute_status;

	if (lane >= MAX_GPIO_TO_GIC_SYNC_LANES)
		return ERR_MAP_FAIL;

	if (base_addr == SEC_PINTMUX_BASE) {
		if (!plat_get_dual_tile_enabled()) {
			/*Can only try to set up secondary pinctrl if secondary actually exists*/
			WARN("PINTMUX: Secondary tile must be enabled to use secondary base address\n");
			return ERR_LOOKUP_FAIL;
		}
		transmuter_addr = SEC_INTERRUPT_TRANSMUTER_BASE;
	} else if (base_addr == PINTMUX_BASE) {
		/*Caller has passed in the GPIO_PINCTRL_BASE, set transmuter address to primary*/
		transmuter_addr = INTERRUPT_TRANSMUTER_BASE;
	} else {
		/*Base address isn't PINCTRL_BASE or SEC_PINCTRL_BASE, immediately exit*/
		WARN("PINTMUX: Unknown base address for GPIO pinmux\n");
		return ERR_LOOKUP_FAIL;
	}

	if (val != PINTMUX_LANE_DISABLED) {
		if (pos_mask) {
			transmute_status = transmuter_change_bit(transmuter_addr, plat_pintmux_lane_to_transmute_signal(lane), SET_POS_MASK);
			if (transmute_status < 0) {
				ERROR("Transmuter action failed, action id:%d\n", SET_POS_MASK);
				return ERR_MAP_FAIL;
			}
		} else {
			transmute_status = transmuter_change_bit(transmuter_addr, plat_pintmux_lane_to_transmute_signal(lane), SET_NEG_MASK);
			if (transmute_status < 0) {
				ERROR("Transmuter action failed, action id:%d\n", SET_NEG_MASK);
				return ERR_MAP_FAIL;
			}
		}

		/*Enable the interrupt in the transmuter once everything has been set up*/
		transmute_status = transmuter_change_bit(transmuter_addr, plat_pintmux_lane_to_transmute_signal(lane), SET_ENABLE_MASK);
		if (transmute_status < 0) {
			ERROR("Transmuter action failed, action id:%d\n", SET_ENABLE_MASK);
			return ERR_MAP_FAIL;
		}
	}

	addr = base_addr + (4 * lane);
	mmio_write_32(addr, val);
	if (base_addr == PINTMUX_BASE)
		pintmux_lane_map[lane] = val;
	else
		pintmux_secondary_lane_map[lane] = val;

	if (val == PINTMUX_LANE_DISABLED) {
		/*Disable the interrupt in the transmuter*/
		transmute_status = transmuter_change_bit(transmuter_addr, plat_pintmux_lane_to_transmute_signal(lane), CLEAR_ENABLE_MASK);
		if (transmute_status < 0) {
			ERROR("Transmuter action failed, action id:%d\n", CLEAR_ENABLE_MASK);
			return ERR_MAP_FAIL;
		}

		/* Set the mask back to the default*/
		transmute_status = transmuter_change_bit(transmuter_addr, plat_pintmux_lane_to_transmute_signal(lane), SET_POS_MASK);
		if (transmute_status < 0) {
			ERROR("Transmuter action failed, action id:%d\n", SET_POS_MASK);
			return ERR_MAP_FAIL;
		}
	}

	return 0;
}

/**
 * Check which IRQ lane a pin is currently mapped to.
 * On success, returns a number between [0-32). This corresponds to the
 * IRQ_GPIO_TO_GIC_SYNC_* IRQ that this pin is currently connected to.
 * On failure (i.e., pin does not exist or is not connected to GIC),
 * returns -1.
 */
int plat_pintmux_get_lane(unsigned int pin, uintptr_t base_addr)
{
	unsigned int lane;

	if (base_addr == PINTMUX_BASE) {
		for (lane = 0; lane < MAX_GPIO_TO_GIC_SYNC_LANES; lane++)
			if (pintmux_lane_map[lane] == pin)
				return lane;
	} else {
		for (lane = 0; lane < MAX_GPIO_TO_GIC_SYNC_LANES; lane++)
			if (pintmux_secondary_lane_map[lane] == pin)
				return lane;
	}
	return ERR_LOOKUP_FAIL;
}

/**
 * Check whether a given IRQ lane is secure or not.
 * This function defaults to true if the lane does not exist.
 */
bool plat_pintmux_is_lane_secure(unsigned int lane, uintptr_t base_addr)
{
	uint32_t irq = plat_pintmux_lane_to_irq(lane, base_addr);

	if (irq == 0)
		return true;
	return plat_ic_get_interrupt_type(irq) != INTR_TYPE_NS;
}

/**
 * Check whether a given IRQ lane is mapped (has a GPIO connected) or not.
 * This function defaults to true if the lane does not exist.
 */
bool plat_pintmux_is_lane_mapped(unsigned int lane, uintptr_t base_addr)
{
	if (lane >= MAX_GPIO_TO_GIC_SYNC_LANES)
		return false;

	if (base_addr == PINTMUX_BASE)
		return pintmux_lane_map[lane] != PINTMUX_LANE_DISABLED;
	else
		return pintmux_secondary_lane_map[lane] != PINTMUX_LANE_DISABLED;
}

/**
 * Given a number between [0-32) representing the GPIO-to-GIC-sync lane,
 * returns the associated IRQ_GPIO_TO_GIC_SYNC_* IRQ value.
 * Returns 0 if the lane does not exist.
 */
CASSERT(IRQ_GPIO_TO_GIC_SYNC_23 - IRQ_GPIO_TO_GIC_SYNC_0 == 23, pintmux_gpio_gic_lane_layout_changed);
CASSERT(IRQ_GPIO_TO_GIC_SYNC_31 - IRQ_GPIO_TO_GIC_SYNC_24 == 7, pintmux_gpio_gic_lane_layout_changed);
CASSERT(IRQ_C2C_OUT_HW_INTERRUPT_78 - IRQ_C2C_OUT_HW_INTERRUPT_55 == 23, pintmux_gpio_gic_lane_layout_changed);
CASSERT(IRQ_C2C_OUT_HW_INTERRUPT_230 - IRQ_C2C_OUT_HW_INTERRUPT_223 == 7, pintmux_gpio_gic_lane_layout_changed);

uint32_t plat_pintmux_lane_to_irq(unsigned int lane, uintptr_t base_addr)
{
	if (base_addr == PINTMUX_BASE) {
		/* assumed and asserted: 0-23 are contiguous, 24-31 are contiguous */
		if (lane < 24)
			return IRQ_GPIO_TO_GIC_SYNC_0 + lane;
		if (lane < MAX_GPIO_TO_GIC_SYNC_LANES)
			return IRQ_GPIO_TO_GIC_SYNC_24 - 24 + lane;
	} else {
		/* These are GPIO_TO_GIC signals coming across C2C bridge, assumed and asserted: 0-23 are contiguous, 24-31 are contiguous. */
		if (lane < 24)
			return IRQ_C2C_OUT_HW_INTERRUPT_55 + lane;
		if (lane < MAX_GPIO_TO_GIC_SYNC_LANES)
			return IRQ_C2C_OUT_HW_INTERRUPT_223 - 24 + lane;
	}
	return 0;
}

/**
 * This function returns true if pad_pin_num is secure_world access only
 */
static bool plat_pin_is_secure(uint32_t pad_pin_num)
{
	uint32_t len;
	bool *pin_list = plat_get_secure_pins(&len);

	if (pad_pin_num < len)
		return pin_list[pad_pin_num];

	WARN("PINTMUX service: Pin number %d does not exist", pad_pin_num);

	return false;
}

/**
 * Map the requested pin to any of the available GPIO-to-GIC-sync lanes
 * (thereby hooking that pin up to the interrupt controller).
 * There are 32 total available lanes -- some of them are routed to the
 * secure world, while others are routed to the non-secure world.
 * Depending on the `is_secure` flag, this function will pull from either
 * the pool of secure lanes or the pool of non-secure lanes.
 * On success, this returns a number between [0-32). This corresponds
 * with one of the IRQ_GPIO_TO_GIC_SYNC_* IRQs (the lane to which this
 * GPIO has been connected).
 * On failure, this returns an error from plat_pintmux.h
 * The function fails if the pin does not exist, if the pin is already routed
 * to the GIC, the transmuter is unable to disable the interrupt before the
 * mapping, the pool of lanes is already full, or a request from the non-secure
 * world is done for a secure pin.
 */
int plat_secure_pintmux_map(unsigned int gpio, bool is_secure, bool pos_mask, uintptr_t base_addr)
{
	int pin;
	int lane;
	int map_status;
	uintptr_t pinctrl_addr;

	pinctrl_addr = plat_get_pintmux_to_pinctrl_mapping(base_addr);
	if (pinctrl_addr == PINTMUX_MAPPING_BAD_ADDRESS) {
		WARN("PINTMUX: Bad base address for GPIO pinmux\n");
		return ERR_LOOKUP_FAIL;
	}

	pin = plat_pinctrl_gpio_to_pin(gpio, is_secure, pinctrl_addr);
	if (pin < 0) {
		WARN("PINTMUX service: Pin %u does not exist or is not configured as GPIO_%s_%u.\n", gpio, is_secure ? "S" : "NS", gpio);
		return ERR_LOOKUP_FAIL;
	}

	if (!is_secure && plat_pin_is_secure(pin)) {
		WARN("PINTMUX service: Request rejected (request coming from non-secure world for a secure pin (%d)\n", gpio);
		return ERR_LOOKUP_FAIL;
	}

	if (plat_pintmux_get_lane(pin, base_addr) >= 0)
		return ERR_MAP_FAIL;

	for (lane = 0; lane < MAX_GPIO_TO_GIC_SYNC_LANES; lane++) {
		if (plat_pintmux_is_lane_mapped(lane, base_addr) || plat_pintmux_is_lane_secure(lane, base_addr) != is_secure)
			continue;
		map_status = plat_pintmux_map_update(lane, pin, pos_mask, base_addr);
		if (map_status < 0) {
			ERROR("PINTMUX service: Failed to map GPIO_%s_%u - already mapped or no lanes available.\n", is_secure ? "S" : "NS", gpio);
			return map_status;
		}
		return lane;
	}
	return ERR_MAP_FAIL;
}

/**
 * Unmap the requested GPIO from its associated lane (thereby disconnecting the
 * associated pin from the interrupt controller).
 * Returns unmapped lane on success, error on failure (the requested gpio does
 * not have a lane associated or is not mapped).
 */
int plat_secure_pintmux_unmap(unsigned int gpio, bool is_secure, uintptr_t base_addr)
{
	int pin;
	int lane;
	int map_status;
	uintptr_t pinctrl_addr;

	pinctrl_addr = plat_get_pintmux_to_pinctrl_mapping(base_addr);
	if (pinctrl_addr == PINTMUX_MAPPING_BAD_ADDRESS) {
		WARN("PINTMUX: Bad base address for GPIO pinmux\n");
		return ERR_LOOKUP_FAIL;
	}

	pin = plat_pinctrl_gpio_to_pin(gpio, is_secure, pinctrl_addr);
	if (pin < 0) {
		WARN("PINTMUX service: Pin %u does not exist or is not configured as GPIO_%s_%u.\n", gpio, is_secure ? "S" : "NS", gpio);
		return ERR_LOOKUP_FAIL;
	}
	lane = plat_pintmux_get_lane(pin, base_addr);
	if (lane < 0) {
		WARN("PINTMUX service: GPIO_%s_%u (Pin %u) is not mapped.\n", is_secure ? "S" : "NS", gpio, pin);
		return ERR_NOT_MAPPED;
	}
	if (plat_pintmux_is_lane_secure((unsigned int)lane, base_addr) != is_secure) {
		WARN("PINTMUX service: GPIO_%s_%u (Pin %u) security state mis-match.\n", is_secure ? "S" : "NS", gpio, pin);
		return ERR_SECURITY;
	}
	if (!plat_pintmux_is_lane_mapped(lane, base_addr))
		return ERR_NOT_MAPPED;

	map_status = plat_pintmux_map_update(lane, PINTMUX_LANE_DISABLED, true, base_addr);
	if (map_status < 0) {
		ERROR("PINTMUX service: Failed to un-map GPIO_%s_%u.\n", is_secure ? "S" : "NS", gpio);
		return map_status;
	}
	return lane;
}

/**
 * Initialize the pintmux platform layer.
 */
void plat_pintmux_init(void)
{
	unsigned int lane;

	for (lane = 0; lane < MAX_GPIO_TO_GIC_SYNC_LANES; lane++) {
		plat_pintmux_map_update(lane, PINTMUX_LANE_DISABLED, true, PINTMUX_BASE);
		if (plat_get_dual_tile_enabled())
			plat_pintmux_map_update(lane, PINTMUX_LANE_DISABLED, true, SEC_PINTMUX_BASE);
	}
}
