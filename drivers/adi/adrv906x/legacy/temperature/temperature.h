/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "../adi_errors/adi_errors.h"
#include "../utils/utils.h"
#include "../hardware_if/hardware_if.h"
#include "../pll/pll.h"
#include "../regmap/pll_regmap.h"

#define         TEMPS_MEAS_READY_TIMEOUT_USEC        (1000u)    /*!< 1 msec */
#define         TEMP_SENSOR_DFL_OFFSET_CODE_VALUE    (16.0f)    /* Default RTL offset code value */
#define         KELVIN_TO_CELSIUS                    (273.0f)   /* Kelvin to Celsius conversion */
#define         TEMP_SLOPE_VALUE                     (1.00f)    /* Slope part of the Kelvin to Celsius conversion */
#define         TCIDAC_125C                          (328.0f)
#define         TCIDAC_MAX                           (4095.0f)
#define         IDAC_INT_LOWBAND                     (2556.2f)
#define         TEMP_TIMEOUT_US                      (10000u)

#define         SCHED_TEMPSENS_UPDATE_TIME_MS        (1000u)
#define         RAWABSZERO                           (0xD6AE)   /* equiv of -274.0 deg C */
#define         TEMP_ROUND                           (5000)     /* half to add for rounding */
#define         TEMP_ADC_DITHER_SETTING1             (0xFu)     /* static negative input offset inside the comparator (DS ADC) set to 3u amp */

#define                 TEMP_ADC_DITHER_SETTING2                         (0xBu)
/* 0x01; random dither to output rounding of decimation filter
 * 0x02; Powersdown_bar the mbias block
 * 0x08; Adds extra cap to reduce oscillator frequency in temp sensor
 */
#define TEMP_AVG_LEN                                                            (10)


typedef enum adi_tempr_sensor {
	ADI_DEVTEMP_CLKPLL,                     /*!< Clk PLL temperature sensor      */
	ADI_DEVTEMP_SEC_CLKPLL,                 /*!< Secondary Clk PLL temperature sensor */
	ADI_DEVTEMP_ETHERNET_CLKPLL,            /*!< Ethernet Clk PLL temperature sensor */
	ADI_DEVTEMP_SEC_ETHERNET_CLKPLL,        /*!< Secondary Ethernet Clk PLL temperature sensor */
	ADI_DEVTEMP_MAX_SENSORS                 /*!< Max number of temperature sensors */
} adi_tempr_sensor_e;

typedef enum {
	DRVTEMP_MEASURING_TEMPERATURE,
	DRVTEMP_MEASURING_OFFSET,
} tempr_measure_state_e;


typedef struct {
	int16_t rawTemp[ADI_DEVTEMP_MAX_SENSORS];                               /* = (sign extended rawRawTemp - sensorAdcOffset) */
	int16_t tSensorAdcOffset[ADI_DEVTEMP_MAX_SENSORS];                      /* sensor adcOffset value in counts */
	bool tempSensorIsEnabled[ADI_DEVTEMP_MAX_SENSORS];                      /* bool array of what's enabled     */
	int16_t rawRawTemp[ADI_DEVTEMP_MAX_SENSORS];                            /* raw temp counts directly from the registers */
	tempr_measure_state_e measureState[ADI_DEVTEMP_MAX_SENSORS];            /* per sensor state that tracks whether offset or temperature is being measured by the HW */
} tempData_t;

extern int tempr_run_measurement_get_sensor_temp(uint16_t tempSensor, float *tempInC, const uint64_t base);

#endif /* TEMPERATURE_H */
