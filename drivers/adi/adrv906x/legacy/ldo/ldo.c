/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <platform_def.h>


#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/adi/adrv906x/pll.h>
#include "ldo.h"


extern void ldo_powerdown(const uint64_t base);
extern int ldo_powerup(const uint64_t base, PllSelName_e pll);
static int ldo_shunt_powerup(const uint64_t base, Ldo_ShuntLdoMask_e shuntLdos);


/**
 *******************************************************************************
 * Function: ldo_powerdown
 *
 * @brief      Power down the requested VCO LDO
 *
 * @details    Power down the requested VCO LDO
 *
 * Parameters:
 * @param [in]    base   - Base address
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
extern void ldo_powerdown(const uint64_t base)
{
	WRITE_PLL_MEM_MAP_SLDO0P8_PD(base, POWERDOWN);
	udelay(LDO_DEFAULT_PWR_UP_DEL);

	WRITE_PLL_MEM_MAP_SLDO1P0_RAMP(base, 0u);
	udelay(LDO_DEFAULT_PWR_UP_DEL);

	WRITE_PLL_MEM_MAP_SLDO1P0_PD(base, POWERDOWN);
	udelay(LDO_DEFAULT_PWR_UP_DEL);

	WRITE_PLL_MEM_MAP_VCO_LDO_PD(base, POWERDOWN);
	udelay(LDO_DEFAULT_PWR_UP_DEL);

	WRITE_PLL_MEM_MAP_VCO_LDO_OUTPUT_PD(base, POWERDOWN);
	udelay(LDO_DEFAULT_ENB_OUT_DEL);

	WRITE_PLL_MEM_MAP_VCOLCR_PD(base, POWERDOWN);
	udelay(VCOLCR_WAIT_TIME);
}



/**
 *******************************************************************************
 * Function: ldo_powerup
 *
 * @brief      Power up the requested VCO LDO
 *
 * @details    Power up the requested VCO LDO
 *
 * Parameters:
 * @param [in]    base   - Base address
 *
 * @return        error status - NO_ERROR on success.
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
extern int ldo_powerup(const uint64_t base, PllSelName_e pll)
{
	int rtnVal = NO_ERROR;

	uint32_t delay = 0u;
	uint8_t val = 0u;
	uint8_t regVal = 0u;

	/* Powerup VCO LCR first, if not already powered up */
	regVal = READ_PLL_MEM_MAP_VCOLCR_PD(base);
	if (regVal != POWERUP) {
		WRITE_PLL_MEM_MAP_VCOLCR_PD(base, POWERUP);
		udelay(VCOLCR_WAIT_TIME);
	}

	WRITE_PLL_MEM_MAP_VCO_LDO_PD(base, POWERUP);
	udelay(LDO_DEFAULT_PWR_UP_SETTLE);

	WRITE_PLL_MEM_MAP_VCO_LDO_OUTPUT_PD(base, POWERUP);
	udelay(LDO_DEFAULT_ENB_OUT_DEL);

	WRITE_PLL_MEM_MAP_LDO_VOUT_ADJ(base, LDO_NOMINAL_VOL_SEL);

	for (delay = 0u; delay != LDO_PWRGOOD_MAX_WAIT_TIME; delay++) {
		val = READ_PLL_MEM_MAP_LDO_POWERGOOD(base);
		if (val == 1u)
			break;
		udelay(1u);
	}

	if (delay == LDO_PWRGOOD_MAX_WAIT_TIME) {
		/* Check the state machine to see if it is in a bad state */
		regVal = READ_BF_8BIT_REG(pREG_PLL_MEM_MAP_NEW_PLL_BASE_REGS_VCO_LDO_STAT(base), 0u, 0xFFu);

		if (regVal == 0x00u) {
			WRITE_PLL_MEM_MAP_LDO_CLEAR_STATUS(base, 1u);
			udelay(100u);
			WRITE_PLL_MEM_MAP_LDO_CLEAR_STATUS(base, 0u);
			udelay(100u);
		}


		if (READ_PLL_MEM_MAP_LDO_POWERGOOD(base) == 1u)
			/* LDO is up and running. */
			rtnVal = NO_ERROR;
		else if (READ_PLL_MEM_MAP_LDO_UVL(base) == 1u)
			/* Input supply voltage is below undervoltage-lockout threshold */
			rtnVal = ERROR_VCO_LDO_UVL;
		else if (READ_PLL_MEM_MAP_LDO_CURLIM(base) == 1u)
			/* Load current is above high side of target range dictated by VCO */
			rtnVal = ERROR_VCO_LDO_CURLIM;
		else if (READ_PLL_MEM_MAP_LDO_LOWOUTPUT(base) == 1u)
			/* LDO output voltage is below low side of target range */
			rtnVal = ERROR_VCO_LDO_LOWOUTPUT;
		else if (READ_PLL_MEM_MAP_LDO_OVERVOLT(base) == 1u)
			/* LDO output voltage is above high side of target range */
			rtnVal = ERROR_VCO_LDO_OVERVOLT;
		else if (READ_PLL_MEM_MAP_LDO_THERMSDN(base) == 1u)
			/* Temperature is above high side of target range */
			rtnVal = ERROR_VCO_LDO_THERMSDN;
		else if (READ_PLL_MEM_MAP_LDO_NOREF(base) == 1u)
			/* Reference input voltage below low side of target range */
			rtnVal = ERROR_VCO_LDO_NOREF;
		else
			rtnVal = ERROR_VCO_LDO_BAD_STATE;
	}
	/* Turn on the Shunt LDO if there were no LDO failures */
	if (rtnVal == NO_ERROR) {
		/* Need some time to let the LDO to settle */
		udelay(LDO_DEFAULT_PWR_UP_SETTLE);

		/* Powerup the shunt LDOs (except for Ethernet PLL)*/
		if (pll != PLL_ETHERNET_PLL && pll != PLL_SEC_ETHERNET_PLL)
			rtnVal = ldo_shunt_powerup(base, LDO_ALL_SHUNT);
	}
	return rtnVal;
}



/**
 *******************************************************************************
 * Function: ldo_shunt_powerup
 *
 * @brief      Power up the Shunt LDO
 *
 * @details    Power up the Shunt LDO
 *
 * Parameters:
 * @param [in] pll - PLL ID for which LDOs should be powered up
 * @param [in] shuntLdos - Shunt LDOs to power up (mask of LDOs)
 *
 * @param [in]    base   - Base address
 *
 * Reference to other related functions
 * @sa
 *
 * Notes: Left this an extern function in case we need to move the order.
 *
 *******************************************************************************
 */
static int ldo_shunt_powerup(const uint64_t base, Ldo_ShuntLdoMask_e shuntLdos)
{
	int rtnVal = NO_ERROR;
	uint8_t ovReadback = 0u;
	uint8_t uvReadback = 0u;
	uint8_t pwrgoodReadback = 0u;
	uint8_t regVal = 0u;
	uint32_t curUs = 0u;

	if ((shuntLdos == LDO_ALL_SHUNT) || (shuntLdos == LDO_VCO_SHUNT)) {
		/* Powerup the VCO/Tcidac shunt LDO, if not already powered */
		regVal = READ_PLL_MEM_MAP_SLDO1P0_PD(base);
		if (regVal != POWERUP) {
			WRITE_PLL_MEM_MAP_SLDO1P0_PD(base, POWERUP);
			udelay(1u);
			WRITE_PLL_MEM_MAP_SLDO1P0_RAMP(base, 1u);

			for (curUs = 0u; curUs < SHUNT_LDO_MAX_WAIT_TIME; curUs++) {
				udelay(1u);

				uvReadback = READ_PLL_MEM_MAP_SLDO1P0_UVFLAG(base);
				ovReadback = READ_PLL_MEM_MAP_SLDO1P0_OVFLAG(base);

				/* Wait for voltage to ramp */
				if ((ovReadback == 0u) && (uvReadback == 0u))
					break;
			}

			if (curUs == SHUNT_LDO_MAX_WAIT_TIME) {
				if ((ovReadback == 1u) && (uvReadback == 1u))
					/* Unexpected hardware behavior, both flags shouldn't be 1 */
					rtnVal = ERROR_SHUNT_LDO_SLDO1P0_POWER_NOT_OK;
				else if (ovReadback == 1u)
					rtnVal = ERROR_SHUNT_LDO_SLDO1P0_OV;
				else
					rtnVal = ERROR_SHUNT_LDO_SLDO1P0_UV;
			}
		}
	}

	if (rtnVal == NO_ERROR) {
		if ((shuntLdos == LDO_ALL_SHUNT) || (shuntLdos == LDO_SDM_SHUNT)) {
			/* Powerup the SDM shunt LDO, if not already powered */
			regVal = READ_PLL_MEM_MAP_SLDO0P8_PD(base);
			if (regVal != POWERUP) {
				WRITE_PLL_MEM_MAP_SLDO0P8_PD(base, POWERUP);

				for (curUs = 0u; curUs < SHUNT_LDO_MAX_WAIT_TIME; curUs++) {
					udelay(1u);

					pwrgoodReadback = READ_PLL_MEM_MAP_PORB_0P8(base);

					/* Wait for voltage to ramp */
					if (pwrgoodReadback == 1u)
						break;
				}

				if (curUs == SHUNT_LDO_MAX_WAIT_TIME)
					rtnVal = ERROR_SHUNT_LDO_SLDO0P8_POWER_NOT_OK;
			}
		}
	}

	return rtnVal;
}
