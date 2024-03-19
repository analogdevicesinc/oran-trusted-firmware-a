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

#define SETTING_CLKPLL_FREQ_7G      0
#define SETTING_CLKPLL_FREQ_11G     1

#define SETTING_ORX_ADC_FREQ_3932M  0
#define SETTING_ORX_ADC_FREQ_7864M  1
#define SETTING_ORX_ADC_FREQ_5898M  2
#define SETTING_ORX_ADC_FREQ_2949M  3

void plat_dprof_init(void);
bool plat_check_ddr_size(void);
uint32_t plat_get_clkpll_freq_setting(void);
uint32_t plat_get_orx_adc_freq_setting(void);
bool plat_get_dual_tile_enabled(void);
void plat_set_dual_tile_disabled(void);
bool plat_get_secondary_linux_enabled(void);
bool plat_is_primary_ecc_enabled();
bool plat_is_secondary_ecc_enabled();
size_t plat_get_dram_physical_size();
size_t plat_get_secondary_dram_physical_size();
size_t plat_get_secondary_dram_size();
size_t plat_get_secondary_dram_base();
size_t plat_get_primary_ddr_remap_window_size();
size_t plat_get_secondary_ddr_remap_window_size();
bool plat_is_secondary_phys_dram_present();
bool plat_is_sysc(void);
bool plat_is_protium(void);
bool plat_is_palladium(void);

#endif /* ADRV906X_DEVICE_PROFILE_H */
