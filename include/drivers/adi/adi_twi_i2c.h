/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_TWI_I2C_H__
#define __ADI_TWI_I2C_H__

#include <stdint.h>

struct adi_i2c_handle {
	uint32_t base;          /* TWI base address */
	uint32_t sclk;          /* TWI source clock (Hz) */
	uint32_t twi_clk;       /* TWI interface clock (Hz) */
};



int adi_twi_i2c_write(struct adi_i2c_handle *h2ic, uint8_t dev_addr, uint32_t addr, uint32_t addr_len, uint8_t *data, uint32_t data_len);
int adi_twi_i2c_read(struct adi_i2c_handle *h2ic, uint8_t dev_addr, uint32_t addr, uint32_t addr_len, uint8_t *data, uint32_t data_len);
int adi_twi_i2c_write_read(struct adi_i2c_handle *hi2c, uint8_t dev_addr, uint32_t addr, uint32_t addr_len, uint8_t *data, uint32_t write_data_len, uint32_t read_data_len);
int adi_twi_i2c_init(struct adi_i2c_handle *hi2c);

#endif /* __ADI_TWI_I2C_H__ */
