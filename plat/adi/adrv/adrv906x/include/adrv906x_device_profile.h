/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_DEVICE_PROFILE_H
#define ADRV906X_DEVICE_PROFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <plat_device_profile.h>
#include <adrv906x_pinmux_source_def.h>

#define SETTING_CLKPLL_FREQ_7G      0
#define SETTING_CLKPLL_FREQ_11G     1

#define SETTING_ORX_ADC_FREQ_3932M  0
#define SETTING_ORX_ADC_FREQ_7864M  1
#define SETTING_ORX_ADC_FREQ_5898M  2
#define SETTING_ORX_ADC_FREQ_2949M  3

#define MAX_NUM_MACS                6                                                   // Dual-tile configuration (3+3 eth interfaces)

#define FW_CONFIG_PIN_NUM_MAX       (ADRV906X_DIO_PIN_START + ADRV906X_DIO_PIN_COUNT)   // Include non-dedicated (0-102) and dedicated pins (115-138)

enum fw_config_periph_ids {
	FW_CONFIG_PERIPH_UART1,
	FW_CONFIG_PERIPH_UART3,
	FW_CONFIG_PERIPH_UART4,
	FW_CONFIG_PERIPH_SPI0,
	FW_CONFIG_PERIPH_SPI1,
	FW_CONFIG_PERIPH_SPI2,
	FW_CONFIG_PERIPH_SPI3,
	FW_CONFIG_PERIPH_SPI4,
	FW_CONFIG_PERIPH_SPI5,
	FW_CONFIG_PERIPH_I2C0,
	FW_CONFIG_PERIPH_I2C1,
	FW_CONFIG_PERIPH_I2C2,
	FW_CONFIG_PERIPH_I2C3,
	FW_CONFIG_PERIPH_I2C4,
	FW_CONFIG_PERIPH_I2C5,
	FW_CONFIG_PERIPH_I2C6,
	FW_CONFIG_PERIPH_I2C7,
	FW_CONFIG_PERIPH_NUM_MAX
};

void plat_dprof_init(void);
bool plat_check_ddr_size(void);
uint32_t plat_get_clkpll_freq_setting(void);
uint32_t plat_get_ethpll_freq_setting(void);
uint32_t plat_get_orx_adc_freq_setting(void);
bool plat_get_dual_tile_no_c2c_enabled(void);
bool plat_get_dual_tile_no_c2c_primary(void);
bool plat_get_dual_tile_enabled(void);
void plat_set_dual_tile_disabled(void);
bool plat_get_secondary_linux_enabled(void);
bool plat_is_primary_ecc_enabled(void);
bool plat_is_secondary_ecc_enabled(void);
size_t plat_get_dram_physical_size(void);
size_t plat_get_secondary_dram_physical_size(void);
size_t plat_get_secondary_dram_size(void);
size_t plat_get_secondary_dram_base(void);
size_t plat_get_primary_ddr_remap_window_size(void);
size_t plat_get_secondary_ddr_remap_window_size(void);
bool plat_is_secondary_phys_dram_present(void);
bool plat_is_hardware(void);
bool plat_is_protium(void);
bool plat_is_palladium(void);
bool *plat_get_secure_peripherals(uint32_t *len);
bool *plat_get_secure_pins(uint32_t *len);

#endif /* ADRV906X_DEVICE_PROFILE_H */
