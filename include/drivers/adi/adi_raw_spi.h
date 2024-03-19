/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_RAW_SPI_H
#define ADI_RAW_SPI_H

#include <stdint.h>

bool adi_raw_spi_init(uintptr_t spi_base, unsigned int mode, unsigned int clock_freq, unsigned int hz);
bool adi_raw_spi_write(uintptr_t spi_base, uint8_t cs, uint8_t address, const uint8_t *buf, size_t len);
bool adi_raw_spi_read(uintptr_t spi_base, uint8_t cs, uint8_t address, uint8_t *buf, size_t len);
void adi_raw_spi_deinit(uintptr_t spi_base);

#endif /* ADI_RAW_SPI_H */
