/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_PINCTRL_H
#define PLAT_PINCTRL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t pin_pad;                                               /**		Pin (or pad) Number to configured */
	uint32_t src_mux;                                               /**		The pinmux source mux select value */
	uint32_t drive_strength;                                        /**		The drive strength setting value */
	bool schmitt_trigger_enable;                                    /**		Set to true to configure input pins with Schmitt Trigger */
	bool pullup_pulldown_enablement;                                /**		Set to true if pullup/pulldown enablement is desired */
	bool pullup;                                                    /**		When PullupPulldownEnablement is true, this field
	                                                                 *                              sets the desired pull direction, true denotes pullUp, false=pulldown */
	uint32_t extended_options;                                      /** 32-bit field for additional pinmux options/settings, from SMC register x7 */
} plat_pinctrl_settings;

/**
 *	Pinmux set function, returns true if set command completes successfully, else false
 *		all configuration parameters within the options parameter, secure_access = true
 * 		when request originates from secure_world
 *
 */
bool plat_secure_pinctrl_set(const plat_pinctrl_settings settings, const bool secure_access, uintptr_t base_addr);

/**
 *	Pinmux set group function, for use by secure_world software
 *		configures groups of I/O defined by the incoming plat_pinctrl_settings array
 *		returns true upon success.  secure_access = true
 *		when request originates from secure_world
 *
 */
bool plat_secure_pinctrl_set_group(const plat_pinctrl_settings pin_group_settings[], const size_t pin_grp_members, const bool secure_access, uintptr_t base_addr);

/**
 *	Pinmux GPIO-to-Pin mapping function.
 *		the parameters define which GPIO, i.e., "GPIO_S_5" would be (5, true)
 *		returns pin number on success
 *		returns -1 on fail (requested GPIO does not exist, or is configured for another purpose)
 */
int plat_pinctrl_gpio_to_pin(unsigned int gpio, bool secure, uintptr_t base_addr);

#endif /* PLAT_PINCTRL_H */
