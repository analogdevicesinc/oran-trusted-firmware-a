/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ADI_ZL30732_H__
#define __ADI_ZL30732_H__

bool adi_zl30732_init(uintptr_t spi_base, unsigned int clock_freq);
bool adi_zl30732_sysref_enable(uintptr_t spi_base, uint8_t cs, bool dual_tile);
bool adi_zl30732_sysref_disable(uintptr_t spi_base, uint8_t cs, bool dual_tile);
uint32_t adi_zl30732_get_firmware_version(uintptr_t spi_base, uint8_t cs);

#endif /* __ADI_ZL30732_H__ */
