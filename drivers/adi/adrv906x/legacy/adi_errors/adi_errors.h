/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_ERRORS_H
#define ADI_ERRORS_H

#define NO_ERROR                                  (0u)
#define ERROR_MASTER_BIAS_RB0_PTAT_CAL_FAILED     (1)
#define ERROR_MASTER_BIAS_RB1_PTAT_CAL_FAILED     (2)

#define ERROR_VCO_LDO_LOWOUTPUT                   (3u)
#define ERROR_VCO_LDO_UVL                         (4u)
#define ERROR_VCO_LDO_NOREF                       (5u)
#define ERROR_VCO_LDO_THERMSDN                    (6u)
#define ERROR_VCO_LDO_CURLIM                      (7u)
#define ERROR_VCO_LDO_OVERVOLT                    (8u)
#define ERROR_SHUNT_LDO_SLDO1P0_UV                (9u)
#define ERROR_SHUNT_LDO_SLDO1P0_OV                (10u)
#define ERROR_SHUNT_LDO_SLDO1P0_POWER_NOT_OK      (11u)
#define ERROR_SHUNT_LDO_SLDO0P8_POWER_NOT_OK      (12u)
#define ERROR_VCO_LDO_BAD_STATE                   (13u)
#define ERROR_SYSTEM_NULL_PTR                     (14u)
#define ERROR_PLL_INVALID_PLL_ERROR               (15u)
#define ERROR_DEV_TEMP_SENSOR_INVALID_EN_ERROR    (16u)
#define ERROR_PLL_FORCED_ALC_TIMEOUT              (17u)
#define ERROR_PLL_BLEED_CAL_FAILED_ERROR          (18u)
#define ERROR_PLL_VCO_CAL_FAILED_ERROR            (19u)
#define ERROR_PLL_CP_CAL_FAILED_ERROR             (20u)
#define ERROR_PLL_SYNTH_LOCK_FAILED_ERROR         (21u)
#define ERROR_PLL_INVALID_FREQ_ERROR              (22u)
#endif /* ADI_ERRORS_H */
