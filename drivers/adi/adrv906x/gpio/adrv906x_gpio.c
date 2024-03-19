/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stdint.h>

#include <drivers/gpio.h>
#include <lib/mmio.h>

#include "adrv906x_gpio_mode.h"
#include <drivers/adi/adrv906x/adrv906x_gpio.h>
#include <platform_def.h>

/* Calculate the GPIO register and bit offset based on the pin number (0-115) */
#define GET_GPIO_REG(pin)       (pin / 32)
#define GET_GPIO_OFFSET(pin)    (pin % 32)

#define GPIO_PIN_NUM    (115)           /* Number of GPIO pins */
#define GPIO_DIR_CONTROL_SIZE    (4)    /* Size of each GPIO mode direction control register */
#define GPIO_REG_NUM    (4)             /* Number of GPIO registers */
#define GPIO_FUNCTION_NUM       (5)     /* Number of GPIO functions (write, clear, set, toggle, read) */

static int get_direction(int gpio);
static void set_direction(int gpio, int direction);
static int get_value(int gpio);
static void set_value(int gpio, int value);

static uint32_t gpio_mode_base_addr;

static uintptr_t adrv906x_reg_base_s[GPIO_REG_NUM][GPIO_FUNCTION_NUM] = {
	{ GPIO_WRITE_REG0_OFFSET, GPIO_WRITE_REG0_CLEAR_OFFSET, GPIO_WRITE_REG0_SET_OFFSET,
	  GPIO_WRITE_REG0_TOGGLE_OFFSET, GPIO_READ_REG0_OFFSET },
	{ GPIO_WRITE_REG1_OFFSET, GPIO_WRITE_REG1_CLEAR_OFFSET, GPIO_WRITE_REG1_SET_OFFSET,
	  GPIO_WRITE_REG1_TOGGLE_OFFSET, GPIO_READ_REG1_OFFSET },
	{ GPIO_WRITE_REG2_OFFSET, GPIO_WRITE_REG2_CLEAR_OFFSET, GPIO_WRITE_REG2_SET_OFFSET,
	  GPIO_WRITE_REG2_TOGGLE_OFFSET, GPIO_READ_REG2_OFFSET },
	{ GPIO_WRITE_REG3_OFFSET, GPIO_WRITE_REG3_CLEAR_OFFSET, GPIO_WRITE_REG3_SET_OFFSET,
	  GPIO_WRITE_REG3_TOGGLE_OFFSET, GPIO_READ_REG3_OFFSET }
};

/* This enum must match the order of actions in adrv906x_reg_base_s */
typedef enum {
	GPIO_WRITE	= 0,
	GPIO_CLEAR	= 1,
	GPIO_SET	= 2,
	GPIO_TOGGLE	= 3,
	GPIO_READ	= 4
} gpio_mode_action_t;

static const gpio_ops_t gpio_ops = {
	.get_direction	= get_direction,
	.set_direction	= set_direction,
	.get_value	= get_value,
	.set_value	= set_value,
};

static int get_direction(int gpio)
{
	uintptr_t base_addr;
	uint32_t offset, data;

	assert((gpio >= 0) && (gpio < GPIO_PIN_NUM));

	base_addr = gpio_mode_base_addr + GPIO_DIR_CONTROL_OFFSET;
	offset = gpio * GPIO_DIR_CONTROL_SIZE;

	data = mmio_read_32(base_addr + offset) & GPIO_DIR_SEL_MASK;

	if (data == 0x1) {
		return GPIO_DIR_OUT;
	} else if (data == 0x2) {
		return GPIO_DIR_IN;
	} else {
		WARN("Neither input or output, defaulting to input\n");
		assert(0);
		return GPIO_DIR_IN;
	}
}

static void set_direction(int gpio, int direction)
{
	uintptr_t base_addr;
	uint32_t offset, data, cleared_data;

	assert((gpio >= 0) && (gpio < GPIO_PIN_NUM));

	base_addr = gpio_mode_base_addr + GPIO_DIR_CONTROL_OFFSET;
	offset = gpio * GPIO_DIR_CONTROL_SIZE;

	data = mmio_read_32(base_addr + offset);
	cleared_data = data & ~GPIO_DIR_SEL_MASK;        /* clear OE and IE bits */

	if (direction == GPIO_DIR_OUT)
		mmio_write_32(base_addr + offset, cleared_data | (0x1U << GPIO_DIR_SEL_POS));
	else
		mmio_write_32(base_addr + offset, cleared_data | (0x1U << (GPIO_DIR_SEL_POS + 1)));
}

static int get_value(int gpio)
{
	uintptr_t base_addr;
	uint32_t offset, data;
	uint32_t bitmask;

	assert((gpio >= 0) && (gpio < GPIO_PIN_NUM));

	base_addr = gpio_mode_base_addr + adrv906x_reg_base_s[GET_GPIO_REG(gpio)][GPIO_READ];
	offset = GET_GPIO_OFFSET(gpio);
	bitmask = 0x1U << offset;

	data = mmio_read_32(base_addr) & bitmask;

	if (data)
		return GPIO_LEVEL_HIGH;
	else
		return GPIO_LEVEL_LOW;
}

static void set_value(int gpio, int value)
{
	uintptr_t base_addr;
	uint32_t offset, bitmask;

	assert((gpio >= 0) && (gpio < GPIO_PIN_NUM));

	offset = GET_GPIO_OFFSET(gpio);
	bitmask = 0x1U << offset;

	if (value == 0x01)
		base_addr = gpio_mode_base_addr + adrv906x_reg_base_s[GET_GPIO_REG(gpio)][GPIO_SET];
	else
		base_addr = gpio_mode_base_addr + adrv906x_reg_base_s[GET_GPIO_REG(gpio)][GPIO_CLEAR];

	mmio_setbits_32(base_addr, bitmask);
}

void adrv906x_gpio_init(uint32_t base_addr)
{
	gpio_mode_base_addr = base_addr;

	gpio_init(&gpio_ops);
}
