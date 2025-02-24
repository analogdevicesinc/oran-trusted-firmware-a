/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <platform_def.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/pll.h>
#include <drivers/adi/adrv906x/temperature.h>
#include <drivers/delay_timer.h>
#include "temperature.h"

static void tempr_toggle_reset(uint32_t sensorIndex, const uint64_t base);
static int tempr_enable_temp_sensor(uint32_t sensorIndex, const uint64_t base);
static void tempr_stop_temp_meas(uint32_t sensorIndex, const uint64_t base);
static void tempr_start_temp_meas(adi_tempr_sensor_e sensorIndex, const uint64_t base);
static int tempr_start_adc_offset_measurement(adi_tempr_sensor_e sensorIndex, const uint64_t base);
static void tempr_read_adc_offset_measurement(adi_tempr_sensor_e sensorIndex, int error, const uint64_t base);
static void tempr_set_measure_state(adi_tempr_sensor_e sensorIndex, tempr_measure_state_e state);
static int tempr_poll_meas_ready(adi_tempr_sensor_e sensorIndex, const uint64_t base);
static int tempr_read_drogon(adi_tempr_sensor_e sensorIndex, const uint64_t base);
static int16_t tempr_get_temp_tenths_degreesC(adi_tempr_sensor_e sensorIndex);
static tempr_measure_state_e tempr_get_measure_state(adi_tempr_sensor_e sensorIndex);
static void tempr_toggle_start_measurement(adi_tempr_sensor_e sensorIndex, const uint64_t base);
static int tempr_adc_offset_measurement(adi_tempr_sensor_e sensorInde, const uint64_t base);


tempData_t tempData;
tempData_t *pTempData;
uint32_t useOldTempSensorStart[PLL_LAST_PLL];


/**
 *******************************************************************************
 * Function: tempr_init
 *
 * @brief       Initialize the full set of temp sensors
 *              on startup.
 *
 * @details     PLL temp sensors are done elsewhere
 *
 * Parameters:
 * @param       None
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
extern void tempr_init(void)
{
	uint32_t ndx;

	pTempData = &tempData;

	for (ndx = 0u; ndx < ADI_DEVTEMP_MAX_SENSORS; ndx++) {
		/*  set the initial values for all sensors  */
		pTempData->rawTemp[ndx] = (int16_t)RAWABSZERO;
		pTempData->tSensorAdcOffset[ndx] = 0;
		pTempData->tempSensorIsEnabled[ndx] = false;
		pTempData->rawRawTemp[ndx] = 0;
		pTempData->measureState[ndx] = DRVTEMP_MEASURING_TEMPERATURE;
	}
}

/**
 *******************************************************************************
 * Function: tempr_read
 *
 * @brief       Read ClkPLL and EthPLL temperature sensors
 * *
 * Parameters:
 * @param       None
 *
 * @return      Error Code, or 0 on succes
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
extern int tempr_read(float *clkpll_temp, float *ethpll_temp)
{
	int err = 0;

	err |= tempr_run_measurement_get_sensor_temp(ADI_DEVTEMP_CLKPLL, clkpll_temp, CLKPLL_BASE);
	err |= tempr_run_measurement_get_sensor_temp(ADI_DEVTEMP_ETHERNET_CLKPLL, ethpll_temp, ETH_PLL_BASE);
	return err;
}

/**
 *******************************************************************************
 * Function: tempr_read_secondary
 *
 * @brief       Read Secondary ClkPLL and EthPLL temperature sensors
 * *
 * Parameters:
 * @param       None
 *
 * @return      Error Code, or 0 on succes
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
extern int tempr_read_secondary(float *sec_clkpll_temp, float *sec_ethpll_temp)
{
	int err = 0;

	err |= tempr_run_measurement_get_sensor_temp(ADI_DEVTEMP_SEC_CLKPLL, sec_clkpll_temp, SEC_CLKPLL_BASE);
	err |= tempr_run_measurement_get_sensor_temp(ADI_DEVTEMP_SEC_ETHERNET_CLKPLL, sec_ethpll_temp, SEC_ETH_PLL_BASE);
	return err;
}

/**
 *******************************************************************************
 * Function: tempr_run_measurement_get_sensor_temp
 *
 * @brief       Get average temp sensor result.
 *
 * @details     Get average temp sensor result.
 *
 * Parameters:
 * @param       [in]  temp sensor number
 * @param       [out] temp sensor result in degree C
 * @param  [in] base        - Base address
 *
 * @return      Error code
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
extern int tempr_run_measurement_get_sensor_temp(uint16_t tempSensor, float *tempInC, const uint64_t base)
{
	int err = NO_ERROR;
	float avg = 0.0f;

	uint32_t i;

	for (i = 0; i != TEMP_AVG_LEN; i++) {
		/* Enable the sensor, reset it and start a "Measure Offset" */
		tempr_enable_temp_sensor(tempSensor, base);
		tempr_toggle_reset(tempSensor, base);
		tempr_toggle_start_measurement(tempSensor, base);

		err = tempr_adc_offset_measurement(tempSensor, base);

		/* Set the state variable for this sensor into "Measure Temperature" */
		tempr_set_measure_state(tempSensor, DRVTEMP_MEASURING_TEMPERATURE);

		tempr_start_temp_meas(tempSensor, base);

		/* wait for the measurement to be ready */
		if (err == NO_ERROR)
			err = tempr_poll_meas_ready(tempSensor, base);
		/* get the measurement  */
		if (err == NO_ERROR)
			/*  temp data ready on next TemperatureGet  */
			tempr_read_drogon(tempSensor, base);
		tempr_stop_temp_meas(tempSensor, base);
		*tempInC = (float)(tempr_get_temp_tenths_degreesC((adi_tempr_sensor_e)tempSensor) / 10.0f);
		avg = avg + *tempInC;

		/* wait sometime to get a better avg */
		udelay(100);
#ifdef DUMP_PLL_SETTINGS_BL2
		INFO("rawTemp = %d \n", pTempData->rawTemp[tempSensor]);
		INFO("tSensorAdcOffset = %d \n", pTempData->tSensorAdcOffset[tempSensor]);
		INFO("rawRawTemp = %d \n", pTempData->rawRawTemp[tempSensor]);
		INFO("temp = %f %f \n", avg, *tempInC);
#endif
	}

	*tempInC = avg / (float)TEMP_AVG_LEN;

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("temp = %f %f \n", avg, *tempInC);
#endif

	return err;
}


/**
 *******************************************************************************
 * Function: tempr_adc_offset_measurement
 *
 * @brief       Temp Sensor  ADC Offset Measurement
 *
 * @details     This function is called to
 *              measure ADC offset for all temp sensors
 *              the caller must determine if the index is valid for the cpu/core
 *              before using this function
 * Parameters:
 * @param  [in] sensorIndex -  Temp sensor index
 * @param  [in] base        - Base address
 *
 * @return      Error code
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static int tempr_adc_offset_measurement(adi_tempr_sensor_e sensorIndex, const uint64_t base)
{
	int err = NO_ERROR;

	tempr_measure_state_e measureStateSave = tempr_get_measure_state(sensorIndex);

	tempr_set_measure_state(sensorIndex, DRVTEMP_MEASURING_OFFSET);
	tempr_start_adc_offset_measurement(sensorIndex, base);

	/* wait for the measurement to be ready */
	err = tempr_poll_meas_ready(sensorIndex, base);

	tempr_read_adc_offset_measurement(sensorIndex, err, base);
	tempr_set_measure_state(sensorIndex, measureStateSave);

	return err;
}
/**
 *******************************************************************************
 * Function: tempr_toggle_start_measurement
 *
 * @brief       Toggle the start measurement bits of the specified sensor
 *
 * @details     The caller must determine if the index is valid for the cpu/core
 *              before using this function
 *              Rising Edge triggers start of measurement
 *
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to toggle start measurement bits of
 * @param  [in] base        - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static void tempr_toggle_start_measurement(adi_tempr_sensor_e sensorIndex, const uint64_t base)
{
	/* Start measurement*/
	WRITE_PLL_MEM_MAP_TEMPS_START_MEASUREMENT(base, RUN);

	/* then stop it */
	tempr_stop_temp_meas(sensorIndex, base);
}


/**
 *******************************************************************************
 * Function: tempr_get_temp_tenths_degreesC
 *
 * @brief       Get degrees Celsius measurement from a specific temp sensor
 *
 * @details     The degrees Celcius are now calculated using a second order equation:
 *              (-1 * X * X) + (26100 * X) +  6100000
 *              the caller must determine if the index is valid for the cpu/core
 *              before using this function
 *
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to read measurement from in degrees Celsius
 *
 * @return      temp sensor measurement in degrees Celsius
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static int16_t tempr_get_temp_tenths_degreesC(adi_tempr_sensor_e sensorIndex)
{
	int16_t tempDegC;
	int32_t result;
	int32_t tempRaw = (int32_t)(pTempData->rawTemp[sensorIndex]);

	if (pTempData) {
		const int32_t coeffA = -1;
		const int32_t coeffB = 26100;
		const int32_t coeffC = 6100000;

		result = tempRaw * tempRaw * coeffA;

		result += tempRaw * coeffB;
		result += coeffC;
		result += TEMP_ROUND;

		result /= 10000;
		tempDegC = (int16_t)(result);
	}

	return tempDegC;
}


/**
 *******************************************************************************
 * Function: tempr_set_measure_state
 *
 * @brief       Set the state of the Sensor HW Measure State Machine
 *
 * Parameters:
 * @param  [in] sensorIndex - sensor index
 * @param  [in] state       - DRVTEMP_MEASURING_TEMPERATURE or DRVTEMP_MEASURING_OFFSET
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static void tempr_set_measure_state(adi_tempr_sensor_e sensorIndex, tempr_measure_state_e state)
{
	pTempData->measureState[sensorIndex] = state;
}

/**
 *******************************************************************************
 * Function: tempr_poll_meas_ready
 *
 * @brief       Get temperature measurement from a specific temp sensor (PLL)
 *
 * @details     intended for soon-after-startup reads for PLLS
 *              the caller must determine if the index is valid for the cpu/core
 *              before using this function
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to poll for ready
 * @param  [in] base        - Base address
 *
 * @return      err
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static int tempr_poll_meas_ready(adi_tempr_sensor_e sensorIndex, const uint64_t base)
{
	int err = NO_ERROR;
	uint32_t looping = 1u;
	uint32_t durTime = 0u;

	while (looping) {
		udelay(1u);

		if (looping) {
			if (READ_PLL_MEM_MAP_TEMPS_MEASUREMENT_READY(base))
				looping = 0u;

			durTime++;
			if (durTime >= TEMPS_MEAS_READY_TIMEOUT_USEC) {
				/* error codes for Tx temp sensors are sequential */
				err = 0u;
				err += (int)sensorIndex;
				looping = 0u;
			}
		}
	}

	return err;
}



/**
 *******************************************************************************
 * Function: tempr_get_measure_state
 *
 * @brief       Get the state of the Sensor HW Measure State Machine
 *
 * Parameters:
 *
 * @param  [in] sensorIndex - sensor index
 *
 * @return      state       - DRVTEMP_MEASURING_TEMPERATURE or DRVTEMP_MEASURING_OFFSET
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static tempr_measure_state_e tempr_get_measure_state(adi_tempr_sensor_e sensorIndex)
{
	return pTempData->measureState[sensorIndex];
}

/**
 *******************************************************************************
 * Function: tempr_read_drogon
 *
 * @brief       get a raw temp reading from a specific sensor
 *
 * @details     this is where the measurement is read from a temp sensor
 *              the caller must determine if the index is valid for the cpu/core
 *              before using this function
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to read raw measurement from in counts
 * @param  [in] base        - Base address
 *
 * @return      error Code
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static int tempr_read_drogon(adi_tempr_sensor_e sensorIndex, const uint64_t base)
{
	uint8_t readybit = 0u;
	int errorCode = NO_ERROR;
	int16_t tempRawTemp = 0;
	int16_t tempTempRawTemp = 0;

	if (pTempData) {
		readybit = READ_PLL_MEM_MAP_TEMPS_MEASUREMENT_READY(base);

		if (readybit > 0u) {
			tempRawTemp = (int16_t)((uint32_t)READ_PLL_MEM_MAP_TEMP_SENSE_REGISTERS_TEMPS_MAIN_01_TEMPS_TEMPERATURE(base) << 4u);           /* MS byte */
			tempRawTemp |= (int16_t)((uint32_t)READ_PLL_MEM_MAP_TEMP_SENSE_REGISTERS_TEMPS_MAIN_02_TEMPS_TEMPERATURE(base) & 0x0fu);        /* LS nibble byte */

			pTempData->rawRawTemp[sensorIndex] = (int16_t)(tempRawTemp);

			if (tempRawTemp & 0x0800u)
				tempRawTemp |= (int16_t)0xf000; /* sign extension for 12bit number in a 16bit word */

			/* Apply ADC Offset correction to raw temp sensor measurement - note that in the case of an offset measurement the tSensorAdcOffset will be 0 */
			tempTempRawTemp = (int16_t)((int16_t)tempRawTemp - (int16_t)pTempData->tSensorAdcOffset[sensorIndex]);

			/* Update the offset or temperature variable depending on what the HW was actually measuring */
			if (tempr_get_measure_state(sensorIndex) == DRVTEMP_MEASURING_TEMPERATURE)
				pTempData->rawTemp[sensorIndex] = tempTempRawTemp;
			else
				pTempData->tSensorAdcOffset[sensorIndex] = tempTempRawTemp;
		}
	} else {
		errorCode = ERROR_SYSTEM_NULL_PTR;
	}

	return errorCode;
}

/**
 *******************************************************************************
 * Function: tempr_read_adc_offset_measurement
 *
 * @brief       Read Temp Sensor ADC Offset Measurement
 *
 * @details     This function is called to
 *              read the measured ADC offset for sensorIndex temp sensors
 *              the caller must determine if the index is valid for the cpu/core
 *              before using this function
 * Parameters:
 * @param  [in] sensorIndex -  Temp sensor index
 * @param  [in] base        - Base address
 *
 * @return      none
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static void tempr_read_adc_offset_measurement(adi_tempr_sensor_e sensorIndex, int error, const uint64_t base)
{
	if (error == NO_ERROR)
		/* get measured adc offset  */
		tempr_read_drogon(sensorIndex, base);
	/*  stop taking measurements */
	tempr_stop_temp_meas(sensorIndex, base);

	WRITE_PLL_MEM_MAP_TEMPS_OFFSET_ADJ(base, 0u);
	WRITE_PLL_MEM_MAP_TEMPS_RESET(base, RESET);
	WRITE_PLL_MEM_MAP_TEMPS_SEL_MUX_VP(base, 0x0u);
	WRITE_PLL_MEM_MAP_TEMPS_RESET(base, CLEAR);
}


/**
 *******************************************************************************
 * Function: tempr_start_adc_offset_measurement
 *
 * @brief       Start Temp Sensor ADC Offset Measurement
 *
 * @details     This function is called to
 *              start a measurement for ADC offset for sensorIndex temp sensors
 *              the caller must determine if the index is valid for the cpu/core
 *              before using this function
 * Parameters:
 * @param  [in] sensorIndex -  Temp sensor index
 * @param  [in] base        - Base address
 *
 * @return      Error code
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static int tempr_start_adc_offset_measurement(adi_tempr_sensor_e sensorIndex, const uint64_t base)
{
	int errorCode = NO_ERROR;

	if (pTempData) {
		/*  reset the offset to zero  */
		pTempData->tSensorAdcOffset[sensorIndex] = 0;

		WRITE_PLL_MEM_MAP_TEMPS_OFFSET_ADJ(base, 0u);
		WRITE_PLL_MEM_MAP_TEMPS_RESET(base, RESET);
		WRITE_PLL_MEM_MAP_TEMPS_SEL_MUX_VP(base, 0x03u);
		WRITE_PLL_MEM_MAP_TEMPS_RESET(base, CLEAR);
		/* Start measurement*/
		WRITE_PLL_MEM_MAP_TEMPS_START_MEASUREMENT(base, RUN);
	} else {
		errorCode = ERROR_SYSTEM_NULL_PTR;
	}

	return errorCode;
}

/**
 *******************************************************************************
 * Function: tempr_start_temp_meas
 *
 * @brief       Start temp measurement for given sensor
 *
 * @details     the caller must determine if the index is valid for the cpu/core
 *              before using this function
 *
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to start
 * @param  [in] base       - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static void tempr_start_temp_meas(adi_tempr_sensor_e sensorIndex, const uint64_t base)
{
	/* Stop any measurement in progress */
	tempr_stop_temp_meas(sensorIndex, base);
	tempr_toggle_reset(sensorIndex, base);

	/* Start measurement*/
	WRITE_PLL_MEM_MAP_TEMPS_START_MEASUREMENT(base, RUN);
}


/**
 *******************************************************************************
 * Function: tempr_stop_temp_meas
 *
 * @brief       Stop temp measurement for given sensor
 *
 * @details     the caller must determine if the index is valid for the cpu/core
 *              before using this function
 *
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to stop
 * @param  [in] base       - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static void tempr_stop_temp_meas(uint32_t sensorIndex, const uint64_t base)
{
	WRITE_PLL_MEM_MAP_TEMPS_START_MEASUREMENT(base, CLEAR);
}

/**
 *******************************************************************************
 * Function: tempr_enable_temp_sensor
 *
 * @brief       Enable the specified sensor
 *
 * @details     the caller must determine if the index is valid for the cpu/core
 *              before using this function
 *
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to enable
 * @param  [in] base       - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static int tempr_enable_temp_sensor(uint32_t sensorIndex, const uint64_t base)
{
	int errorCode = NO_ERROR;

	WRITE_PLL_MEM_MAP_TEMPS_CLK_PD(base, POWERUP);
	WRITE_PLL_MEM_MAP_TEMPS_PTAT_PD(base, POWERUP);
	WRITE_PLL_MEM_MAP_TEMPS_REF_PD(base, POWERUP);
	WRITE_PLL_MEM_MAP_TEMPS_ADC_PD(base, POWERUP);

	/*This bit must be set to 1 based on TRDPALAU-996 */
	WRITE_PLL_MEM_MAP_TEMPS_STARTUP_PD(base, 1);
	WRITE_PLL_MEM_MAP_TEMPS_CURR_FLASHO_N(base, TEMP_ADC_DITHER_SETTING1);

	WRITE_PLL_MEM_MAP_TEMP_SENSE_REGISTERS_TEMPS_TEST_SPARE_00_TEMPS_CTRL(base, 0x40u);
	WRITE_PLL_MEM_MAP_TEMP_SENSE_REGISTERS_TEMPS_TEST_SPARE_01_TEMPS_CTRL(base, TEMP_ADC_DITHER_SETTING2);

	WRITE_PLL_MEM_MAP_TEMPS_WAIT_TO_MEASURE(base, 3u);
	WRITE_PLL_MEM_MAP_TEMPS_WAKE_SETTING(base, 2u);

	return errorCode;
}

/**
 *******************************************************************************
 * Function: tempr_toggle_reset
 *
 * @brief       Toggle the reset bits of the specified sensor
 *
 * @details    The caller must determine if the index is valid for the cpu/core
 *              before using this function
 *              RESET is active high
 *
 * Parameters:
 * @param  [in] sensorIndex - temp sensor to toggle reset bits of
 * @param  [in] base       - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *******************************************************************************
 */
static void tempr_toggle_reset(uint32_t sensorIndex, const uint64_t base)
{
	WRITE_PLL_MEM_MAP_TEMPS_RESET_ADC(base, RESET);
	WRITE_PLL_MEM_MAP_TEMPS_RESET_ADC(base, CLEAR);
	WRITE_PLL_MEM_MAP_TEMPS_RESET(base, RESET);
	WRITE_PLL_MEM_MAP_TEMPS_RESET(base, CLEAR);
}
