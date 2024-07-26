/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <common/debug.h>
#include <drivers/arm/sp805.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_pinctrl.h>
#include <adrv906x_pin_def.h>
#include <plat_pinctrl.h>
#include <plat_pinctrl_svc.h>
#include <platform_def.h>

/*
 * The following defines the bits used for the drive strength input on x4
 */
#define ADRV906X_PINCTRL_x4_DRIVE_STRENGTH_MASK  U(0x0F)

/*
 * MACRO to determine if specified pin is dedicated IO
 */
#define ADRV906X_IS_DIO_PIN(x) ((x) >= ADRV906X_DIO_PIN_START && (x) < (ADRV906X_DIO_PIN_START + ADRV906X_DIO_PIN_COUNT))

/**
 * This function returns true if pad_pin_num is secure_world access only
 */
static bool plat_pin_is_secure(uint32_t pad_pin_num)
{
	uint32_t len;
	bool *pin_list = plat_get_secure_pins(&len);

	if (pad_pin_num < len)
		return pin_list[pad_pin_num];

	WARN("PINCTRL: Pin number %d does not exist", pad_pin_num);

	return false;
}

/** Added settings to prohibit optimization which     */
#pragma GCC push_options
#pragma GCC optimize ("O0")

/**
 *	Pinmux set function, returns true if set command completes successfully, else false
 *		all configuration parameters within the settings parameter, secure_access = true
 * 		when request originates from secure_world.
 *
 */
bool plat_secure_pinctrl_set(const plat_pinctrl_settings settings, const bool secure_access, uintptr_t base_addr)
{
	adrv906x_cmos_pad_ds_t adrv906x_drive_strength = (adrv906x_cmos_pad_ds_t)(settings.drive_strength & ADRV906X_PINCTRL_x4_DRIVE_STRENGTH_MASK);
	adrv906x_pad_pupd_t pull_direction;

	if (base_addr == SEC_PINCTRL_BASE) {
		if (!plat_get_dual_tile_enabled()) {
			/*Can only try to set up secondary pinctrl if secondary actually exists*/
			WARN("PINCTRL: Secondary tile must be enabled to use secondary base address");
			return false;
		}
	} else if (base_addr != PINCTRL_BASE) {
		/*Base address isn't PINCTRL_BASE or SEC_PINCTRL_BASE, immediately exit*/
		WARN("PINCTRL: Unknown base address for GPIO pinmux");
		return false;
	}

	if (settings.pullup == true)
		pull_direction = PULL_UP;
	else
		pull_direction = PULL_DOWN;

	/*
	 * Verify the pin# is in range
	 */
	if (settings.pin_pad >= ADRV906X_PIN_COUNT && !ADRV906X_IS_DIO_PIN(settings.pin_pad)) {
		WARN("PINCTRL: Request Pin # = %d out of range \n", settings.pin_pad);
		return false;
	}

	/*
	 * Verify that non-dedicated IO have a valid src mux specified
	 */
	if (!ADRV906X_IS_DIO_PIN(settings.pin_pad) && settings.src_mux >= ADRV906X_PINMUX_SRC_PER_PIN) {
		WARN("PINCTRL: Invalid source mux value: %u specified for pin# %u\n", settings.src_mux, settings.pin_pad);
		return false;
	}

	/*
	 * Verify the incoming pin_source is withing range
	 */
	if (settings.src_mux >= ADRV906X_PINMUX_SRC_PER_PIN && settings.src_mux != ADRV906X_DIO_MUX_NONE) {
		WARN("PINCTRL: Invalid source mux value: %u \n", settings.src_mux);
		return false;
	}

	/*
	 * Verify that non-dedicated requested source is valid and not NO_SIGNAL
	 */
	if (!ADRV906X_IS_DIO_PIN(settings.pin_pad)) {
		if ((pinmux_config[settings.pin_pad][settings.src_mux] >= ADRV906X_PINMUX_NUM_SRCS) || (pinmux_config[settings.pin_pad][settings.src_mux] == NO_SIGNAL)) {
			WARN("PINCTRL: Invalid source %d requested \n", pinmux_config[settings.pin_pad][settings.src_mux]);
			return false;
		}
	}

	/*
	 * Prohibit normal world from configuring secure IO
	 */
	if (!secure_access && plat_pin_is_secure(settings.pin_pad)) {
		WARN("PINCTRL: Normal World request to configure secure Pin # = %d \n", settings.pin_pad);
		return false;
	}
	/*
	 * Perform the request
	 * per hardware (Sonu John's email), it is preferred that we configure PAD settings prior to the pinmux configuration.
	 */

	/*
	 * Set the drive strength
	 */
	pinctrl_set_pad_drv_strn(base_addr, settings.pin_pad, adrv906x_drive_strength);

	/*
	 * Setup Schmitt Trigger
	 */
	pinctrl_set_pad_st_en(base_addr, settings.pin_pad, settings.schmitt_trigger_enable);

	/*
	 * If PULL enablement, set the state here
	 */
	if (settings.pullup_pulldown_enablement == true)
		pinctrl_set_pad_ps(base_addr, settings.pin_pad, pull_direction);

	/*
	 * Set PUll enablement
	 */
	pinctrl_set_pad_pen(base_addr, settings.pin_pad, settings.pullup_pulldown_enablement);

	/*
	 * Setup the pinmux sel / pinmux source, for non-dedicated IO
	 */
	if (!ADRV906X_IS_DIO_PIN(settings.pin_pad))
		pinctrl_set_pinmux_sel(base_addr, settings.pin_pad, settings.src_mux);

	return true;
}

/**
 *	Pinmux set group function, for use by secure_world software
 *		configures groups of I/O defined by the incoming plat_pinctrl_settings array
 *		returns true upon success.  secure_access = true
 *		when request originates from secure_world
 *
 */
bool plat_secure_pinctrl_set_group(const plat_pinctrl_settings pin_group_settings[], const size_t pin_grp_members, const bool secure_access, uintptr_t base_addr)
{
	size_t group_member;

	if (pin_group_settings == NULL || pin_grp_members == 0U)
		return false;

	for (group_member = 0U; group_member < pin_grp_members; group_member++) {
		if (!plat_secure_pinctrl_set(pin_group_settings[group_member], secure_access, base_addr)) {
			WARN("<<Error>> plat_secure_pinctrl_set_group: plat_secure_pinctrl_set returned error\n");
			return false;
		}
	}

	return true;
}

/**
 *	Pinmux GPIO-to-Pin mapping function.
 *		the parameters define which GPIO, i.e., "GPIO_S_5" would be (5, true)
 *		returns pin number on success
 *		returns -1 on fail (requested GPIO does not exist, or is configured for another purpose)
 */
int plat_pinctrl_gpio_to_pin(unsigned int gpio, bool secure, uintptr_t base_addr)
{
	pinmux_source_t source;
	unsigned int pin;

	if (!pinctrl_gpio_to_source(gpio, secure, &source))
		return -1;

	for (pin = 0; pin < ADRV906X_PIN_COUNT; pin++) {
		pinmux_source_t s = pinctrl_get_pinmux(base_addr, pin);
		if (s == source)
			return pin;
	}

	return -1;
}

#pragma GCC pop_options
