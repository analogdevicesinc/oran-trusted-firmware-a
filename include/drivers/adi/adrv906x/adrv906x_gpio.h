/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_GPIO_H
#define ADRV906X_GPIO_H

void adrv906x_gpio_init(uint32_t primary_base_address, uint32_t secondary_base_address);

int  adrv906x_primary_gpio_get_direction(int gpio);
void adrv906x_primary_gpio_set_direction(int gpio, int direction);
int  adrv906x_primary_gpio_get_value(int gpio);
void adrv906x_primary_gpio_set_value(int gpio, int value);

int  adrv906x_secondary_gpio_get_direction(int gpio);
void adrv906x_secondary_gpio_set_direction(int gpio, int direction);
int  adrv906x_secondary_gpio_get_value(int gpio);
void adrv906x_secondary_gpio_set_value(int gpio, int value);

#endif /* ADRV906X_GPIO_H */
