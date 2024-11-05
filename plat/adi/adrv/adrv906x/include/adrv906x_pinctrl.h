/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <adrv906x_pinmux_source_def.h>

#ifndef ADRV906X_PINCTRL_H
#define ADRV906X_PINCTRL_H

/* Adrv906x pad drive strength enum */
typedef enum {
	CMOS_PAD_DS_0000	= 0,
	CMOS_PAD_DS_0001	= 1,
	CMOS_PAD_DS_0010	= 2,
	CMOS_PAD_DS_0011	= 3,
	CMOS_PAD_DS_0100	= 4,
	CMOS_PAD_DS_0101	= 5,
	CMOS_PAD_DS_0110	= 6,
	CMOS_PAD_DS_0111	= 7,
	CMOS_PAD_DS_1000	= 8,
	CMOS_PAD_DS_1001	= 9,
	CMOS_PAD_DS_1010	= 10,
	CMOS_PAD_DS_1011	= 11,
	CMOS_PAD_DS_1100	= 12,
	CMOS_PAD_DS_1101	= 13,
	CMOS_PAD_DS_1110	= 14,
	CMOS_PAD_DS_1111	= 15,
} adrv906x_cmos_pad_ds_t;

/*Pull up / Pull down enum */
typedef enum {
	PULL_DOWN	= 0,
	PULL_UP		= 1,
} adrv906x_pad_pupd_t;

/* Set pad drive strength */
void pinctrl_set_pad_drv_strn(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_cmos_pad_ds_t pad_ds);

/* Get pad drive strength */
void pinctrl_get_pad_drv_strn(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_cmos_pad_ds_t *pad_ds);

/* Set pad schmitt trigger enablement */
void pinctrl_set_pad_st_en(const uintptr_t baseaddr, uint32_t pad_num, bool enable);

/* Get pad schmitt trigger enablement */
void pinctrl_get_pad_st_en(const uintptr_t baseaddr, uint32_t pad_num, bool *enable);

/* Set enablement of pad pull up or pull down */
void pinctrl_set_pad_pen(const uintptr_t baseaddr, uint32_t pad_num, bool enable);

/* Get enablement of pad pull up or pull down */
void pinctrl_get_pad_pen(const uintptr_t baseaddr, uint32_t pad_num, bool *enable);

/* Set pad pull up / pull down selection */
void pinctrl_set_pad_ps(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_pad_pupd_t pull);

/* Get pad pull up / pull down selection */
void pinctrl_get_pad_ps(const uintptr_t baseaddr, uint32_t pad_num, adrv906x_pad_pupd_t *pull);

/* Get/Set pinmux selection */
uint32_t pinctrl_get_pinmux_sel(const uintptr_t baseaddr, uint32_t pin_num);
void pinctrl_set_pinmux_sel(const uintptr_t baseaddr, uint32_t pin_num, uint32_t sel);

/* Get/Set pinmux selection */
pinmux_source_t pinctrl_get_pinmux(const uintptr_t baseaddr, uint32_t pin_num);
bool pinctrl_set_pinmux(const uintptr_t baseaddr, uint32_t pin_num, pinmux_source_t pinmux_src);

/* GPIO mappings */
bool pinctrl_s_gpio_to_source(unsigned int gpio, pinmux_source_t *source);
bool pinctrl_ns_gpio_to_source(unsigned int gpio, pinmux_source_t *source);
bool pinctrl_gpio_to_source(unsigned int gpio, bool secure, pinmux_source_t *source);

#endif
