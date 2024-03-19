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
#include <drivers/delay_timer.h>
#include "pll.h"

extern bool plat_is_sysc(void);

LoopFilterResult_t LoopFilterSettings[NUM_CLK_SPD] = {
	{ CLK_VCO_7G_MHZ,  10, 3, 0, 0, 5, 18, 18, 31, 0, 0, 525917 }, /* ClkPLL  8G */
	{ CLK_VCO_11G_MHZ, 25, 9, 0, 0, 8, 25, 25, 31, 0, 0, 505867 }  /* ClkPLL  11G */
};


/* Default settings for Loopfilters for all the PLL's  */
const LoopFilterParam_t LoopFilterDefault[PLL_LAST_PLL] = {
	/* loopBW    phaseMargin  voutLvl */
	{ 500000.0f, 60.0f, 10u, },     /* PLL_CLKGEN_PLL  */
	{ 500000.0f, 60.0f, 10u },      /* PLL_SEC_CLKGEN_PLL_PLL  */
};


CpCalcs_t cpCalcValue[NUM_REF_CLOCK_SETTINGS] = {
	{ 0, 0, 0, 0.0f },
	{ 0, 0, 0, 0.0f }
};



/* VCO coef tables for all PLLs */
const PeakTempCoef_t PLL_PeakTempCoef[PLL_LAST_PLL] = {
	/* LBvcoint      LBvcoSlope     HBvcoint    HBvcoSlope*/
	{ 1.089f, -0.00054f, 1.065f, -0.00101f },               /* Clock Gen PLL */
	{ 1.089f, -0.00054f, 1.065f, -0.00101f }                /* Secondary Clock Gen PLL */
};


/* VCO coef tables for all PLLs */
const TempCoef_t PLL_TempComp[PLL_LAST_PLL] = {
	/* idacSlpLow     idacIntLow   idacSlpHigh     idacIntHigh   */
	{ -14.60f, 2470.0f, -14.60f, 2470.0f },         /* Clock Gen PLL */
	{ -14.60f, 2470.0f, -14.60f, 2470.0f },         /* Secondary CLock Gen PLL */
};



const PeakDet_t PLL_PeakDetector[] = {
	{ 1.252f, 2, 15, 0 }, { 1.236f, 3, 15, 0 }, { 1.224f, 2, 14, 0 }, { 1.208f, 3, 14, 0 },
	{ 1.194f, 2, 13, 0 }, { 1.18f,	3, 13, 0 }, { 1.166f, 2, 12, 0 }, { 1.152f, 4, 14, 0 },
	{ 1.138f, 2, 11, 0 }, { 1.124f, 5, 15, 0 }, { 1.108f, 2, 10, 0 }, { 1.096f, 5, 14, 0 },
	{ 1.08f,  2, 9,	 0 }, { 1.066f, 4, 11, 0 }, { 1.038f, 5, 12, 0 }, { 1.01f,  5, 11, 0 },
	{ 0.98f,  5, 10, 0 }, { 0.964f, 2, 8,  0 }, { 0.952f, 5, 9,  0 }, { 0.892f, 4, 8,  0 },
	{ 0.846f, 2, 7,	 0 }, { 0.836f, 5, 8,  0 }, { 0.832f, 3, 7,  0 }, { 0.79f,  2, 15, 1 },
	{ 0.774f, 3, 15, 1 }, { 0.76f,	2, 14, 1 }, { 0.744f, 3, 14, 1 }, { 0.73f,  2, 13, 1 },
	{ 0.716f, 3, 13, 1 }, { 0.686f, 3, 12, 1 }, { 0.672f, 2, 11, 1 }, { 0.658f, 3, 11, 1 },
	{ 0.628f, 3, 10, 1 }, { 0.614f, 2, 9,  1 }, { 0.6f,   5, 13, 1 }, { 0.54f,  5, 11, 1 },
	{ 0.51f,  5, 10, 1 }, { 0.496f, 2, 8,  1 }, { 0.48f,  3, 8,  1 }, { 0.358f, 5, 8,  1 },
	{ 0.294f, 4, 7,	 1 }, { 0.246f, 2, 6,  1 }, { 0.228f, 3, 6,  1 }, { 0.000f, 3, 6,  1 }
};




static const VcoCoef_t PLL_TempCompClkPLL_1[VCO_COEFF_CLK_TABLE_SIZE] = {
/*   f1,                               f2,                                    VCOVARTC  VARACTOR  VCOPKT  LDO_PTATCTRL */
	{ 1u,				 MIN_VCO_FREQ_CLK_MHZ,		       15u, 2u, 3u, 1u },
	{ 7101u,			 9900u,				       15u, 2u, 3u, 1u },
	{ 9901u,			 9950u,				       15u, 2u, 3u, 1u },
	{ 9951u,			 9980u,				       15u, 2u, 3u, 1u },
	{ 9981u,			 (VCO_HB_THRESHOLD_FREQ_CLK_MHZ - 1u), 15u, 2u, 3u, 1u },
	{ VCO_HB_THRESHOLD_FREQ_CLK_MHZ, 11500u,			       15u, 2u, 3u, 1u },
	{ 11501u,			 13000u,			       15u, 2u, 3u, 1u },
	{ 13001u,			 13900u,			       15u, 2u, 3u, 1u },
	{ 13901u,			 MAX_VCO_FREQ_CLK_MHZ,		       9u,  2u, 3u, 1u }
};

/* These are the threshold freq in kHz where low to highband VCO is selected
 * NOTE: The order MUST match the order def'n in PllSelName_e */
const uint32_t pllBandSelectFreq_kHz[PLL_LAST_PLL] =
{
	VCO_HB_THRESHOLD_FREQ_CLK_KHZ,          /* Clock Gen PLL */
	VCO_HB_THRESHOLD_FREQ_CLK_KHZ,          /* Secondary Clock Gen PLL */
};



uint8_t clkRefClkDiv[NUM_CLK_SPD] = { 1, 1 };

static const VcoCoef_t *pPLL_TempCompClkPLL = NULL;

uint8_t clkLoDiv[NUM_CLK_SPD] = { 8, 12 };

PllSynthParam_t pllStateSettings[PLL_LAST_PLL];
PllSynthParam_t *gpPllStateSettings[PLL_LAST_PLL];

static void pll_calc_loop_filter_coef(PllSelName_e pll, PllSynthParam_t *pPll);
static void pll_init_loop_filter(PllSelName_e pll, PllSynthParam_t *pPll);
static void pll_init_pll_tables(PllSelName_e pll);

void pll_set_dut_modulus(PllSelName_e pll, uint32_t modulus, const uint64_t base);
int pll_bleed_ramp(PllSelName_e pll, const uint64_t base);

static uint32_t pll_get_dut_modulus(PllSelName_e pll, const uint64_t base);
static void pll_power_ctrl(PllSelName_e pll, uint8_t state, const uint64_t base, const uint64_t dig_core_base);
static int pll_sdm_ref_clk_init(PllSelName_e pll, const uint64_t base, const uint32_t refclk_freq);
static int pll_update_temp_comp(PllSelName_e pll, const uint64_t base);
static void pll_reset(PllSelName_e pll, const uint64_t base);
static void pll_reset_dividers(PllSelName_e pll, const uint64_t base);
static void pll_init_hw_defaults(PllSelName_e pll, const uint64_t base);
static int pll_compute_synth_parameters(PllSelName_e pll, const uint64_t base);
static void pll_calc_int_frac_words(PllSelName_e pll, const uint64_t base);
static void pll_update_vco_tables(PllSelName_e pll);
static int pll_write_synth_parameters(PllSelName_e pll, const uint64_t base);
static void pll_write_loop_RCs(PllSelName_e pll, LoopFilterResult_t *pLf, const uint64_t base);
static int pll_run_cp_cal(PllSelName_e pll, const uint64_t base);
static float pll_calculate_cp_params(PllSelName_e pll);
static int pll_wait_for_cp_valid_complete(uint64_t base);
static int pll_wait_for_cp_init_complete(uint64_t base);
static int pll_run_vco_cal(PllSelName_e pll, const uint64_t base);
static int pll_lock_detect(PllSelName_e pll, uint32_t waitUsec, const uint64_t base);
static int pll_rf_synth_cal_and_locked(PllSelName_e pll, const uint64_t base);

/**
 *******************************************************************************
 * Function: pll_init_loop_filter
 * @brief
 *
 * @details  Init loopfilter PLL structure
 *
 * Parameters:
 * @param [in]   pll - PLL name.
 * @param [in]   Pointer to PLL state structure.
 *
 * @return      none
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_init_loop_filter(PllSelName_e pll, PllSynthParam_t *pPll)
{
	pPll->loopBW = LoopFilterDefault[pll].loopBW;
	pPll->phaseMargin = LoopFilterDefault[pll].phaseMargin;
	pPll->vOut = LoopFilterDefault[pll].voutLvl;
}




/*******************************************************************************
 * Function: pll_calc_loop_filter_coef
 * @brief
 *
 * @details  Returns the loopfilter h/w settings for a specified PLL
 *
 * Parameters:
 * @param [in]   pll name.
 * @param [in]   Pointer to PLL state structure.
 *
 * @return
 *
 * Reference to other related functions
 * @sa
 *
 *   NOTE: This was a direct port from Matlab code provided by H/W
 *         the structure was not modified at this time to minimize
 *         introducing implementation issues. Optimization can be
 *         revisited latter.
 *
 *******************************************************************************
 */
static void pll_calc_loop_filter_coef(PllSelName_e pll, PllSynthParam_t *pPll)
{
	if (pPll->vcoFreqHz == CLK_VCO_7G_HZ)
		pPll->LoopFilter = LoopFilterSettings[CLK_7G];
	else
		pPll->LoopFilter = LoopFilterSettings[CLK_11G];

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("LF %d\n", pPll->LoopFilter.C1);
	INFO("LF %d\n", pPll->LoopFilter.C2);
	INFO("LF %d\n", pPll->LoopFilter.C3);
	INFO("LF %d\n", pPll->LoopFilter.R1);
	INFO("LF %d\n", pPll->LoopFilter.R3);
	INFO("LF %d\n", pPll->LoopFilter.ICP);
	INFO("Ibleed %d\n", pPll->LoopFilter.Ibleed);
	INFO("LF %d\n", pPll->LoopFilter.effectiveloopBW);
	INFO("LF %d\n", pPll->vcoBandSelFreq_kHz);
	INFO("vcoVaractor %d\n", pPll->vcoVaractor);
	INFO("loopBW %f\n", pPll->loopBW);
	INFO("phaseMargin %f\n", pPll->phaseMargin);
	INFO("vco %f\n", (float)pPll->vcoFreqHz);
	INFO("kVco %f\n", (float)pPll->kVco);
	INFO("refClock %d\n", pPll->refClock);
#endif
}


/**
 *******************************************************************************
 * Function: pll_init_pll_tables
 * @brief
 *
 * @details   Init all the PLL tables from a powerup
 *
 * Parameters:
 * @param [in]    pll  - PLL to be init'd
 *
 * @return       none
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_init_pll_tables(PllSelName_e pll)
{
	/* Set to not-override */
	gpPllStateSettings[pll]->vcoAmpOverride = 0u;
	gpPllStateSettings[pll]->pfdOverride = 0u;

	/* Invalidate the ICP so we force a Charge Pump Cal for first time the PLL is started */
	gpPllStateSettings[pll]->LoopFilter.oldIcp = PLL_INIT_CP_CURRENT;

	pll_init_loop_filter(pll, gpPllStateSettings[pll]);

	/* Set up pointers to VCO coefficients */
	pPLL_TempCompClkPLL = PLL_TempCompClkPLL_1;

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("Table Init\n");
#endif
}


/**
 *******************************************************************************
 * Function: pll_init_hw_defaults
 * @brief
 *
 * @details  Sets non-freq depenedent h/w settings at powerup
 *
 * Parameters:
 * @param [in]    pll  - PLL to be controlled
 * @param [in]    base - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_init_hw_defaults(PllSelName_e pll, const uint64_t base)
{
	WRITE_PLL_MEM_MAP_FREQ_CAL_CNT_RDSEL(base, 1u);
/*    WRITE_PLL_MEM_MAP_QUICK_FREQ_CAL_EN(base, 1u);   */

	/* Set the lock detector counter */
	WRITE_PLL_MEM_MAP_LOCKDET_CNT(base, PLL_LOCK_DETECTOR_TIME);

	/* Update the freq cal registers */
	{
		uint8_t working;
		working = (uint8_t)(PLL_CAL_MAX_CNT_NORMAL & 0x0FFu);
		WRITE_PLL_MEM_MAP_NEW_VCO_CAL_REGS_FREQ_CAL_MAX_CNT0_FREQ_CAL_MAX_CNT(base, working);
		working = (uint8_t)((PLL_CAL_MAX_CNT_NORMAL >> 8u) & 0x0FFu);
		WRITE_PLL_MEM_MAP_NEW_VCO_CAL_REGS_FREQ_CAL_MAX_CNT1_FREQ_CAL_MAX_CNT(base, working);
		working = (uint8_t)((PLL_CAL_MAX_CNT_NORMAL >> 16u) & 0x0FFu);
		WRITE_PLL_MEM_MAP_NEW_VCO_CAL_REGS_FREQ_CAL_MAX_CNT2_FREQ_CAL_MAX_CNT(base, working);
	}

	/* These set the charge pump voltage window for tempcomp operation to ~425mV - 575mV [note that Yoda bitfield descriptions for voltage levels are different from silicon */
	WRITE_PLL_MEM_MAP_CP_VLEVEL_LOW_TC(base, 2u);
	WRITE_PLL_MEM_MAP_CP_VLEVEL_HIGH_TC(base, 1u);

	/* Setup the One-time VCO registers */
/*    WRITE_PLL_MEM_MAP_VCO_COARSE_CAL_EN(base, 1u);  */
/*    WRITE_PLL_MEM_MAP_VCO_FINE_CAL_EN(base, 1u);   */
	WRITE_PLL_MEM_MAP_VCO_CAL_INIT_DEL(base, 6u);
	WRITE_PLL_MEM_MAP_VCO_CAL_ALC_INIT_WAIT(base, 3u);
	WRITE_PLL_MEM_MAP_QUICK_FREQ_CAL_THRESHOLD(base, 0x8u);
	WRITE_PLL_MEM_MAP_CP_SEL_ADC_TC(base, ENABLE);

/*    WRITE_PLL_MEM_MAP_VCO_CAL_ALC_CLK_DIV(base, 5u);  */
/*    WRITE_PLL_MEM_MAP_VCO_CAL_ALC_WAIT(base, 7u);  */
/*    WRITE_PLL_MEM_MAP_FORCE_VCO_INIT_ALC_VALUE(base, 1u);  */
	/* Set Quick Cal thresholds */

/*    WRITE_PLL_MEM_MAP_VCO_ALC_CAL_EN(base, 1u);*/

/*    WRITE_PLL_MEM_MAP_VCO_TC_TRACKING_EN(base, ENABLE);  */

	WRITE_PLL_MEM_MAP_CP_VLEVEL_HIGH_FLAG(base, 0xEu);      /* 850mV */
	WRITE_PLL_MEM_MAP_CP_VLEVEL_LOW_FLAG(base, 0xDu);       /* 125mV */

	WRITE_PLL_MEM_MAP_CP_AUXADC_HYST_TC(base, 1u);

	WRITE_PLL_MEM_MAP_CP_AUXADC_HYST_ON_TC(base, 0u);
	WRITE_PLL_MEM_MAP_AUXADC_HYST_FLAG(base, 1u);
	WRITE_PLL_MEM_MAP_AUXADC_HYST_ON_FLAG(base, 1u);
	WRITE_PLL_MEM_MAP_MCS_WAIT_COUNT(base, 3u);
	WRITE_PLL_MEM_MAP_SEL_ADC_FLAG(base, 1u);

	/* ?????????? NOT SURE OF THIS IS NEEDED Check with Jason Fan */
	WRITE_PLL_MEM_MAP_LDO_SPARE(base, 10u);


	WRITE_PLL_MEM_MAP_LD_LOL_HYST_EN(base, 1u);
	WRITE_PLL_MEM_MAP_LD_EN(base, 1u);
	WRITE_PLL_MEM_MAP_LD_LOL_EN(base, 1u);
	WRITE_PLL_MEM_MAP_LD_LOL_LDP(base, 0u);
	WRITE_PLL_MEM_MAP_LD_COUNT(base, 1u);
	WRITE_PLL_MEM_MAP_CP_SEL_ADC_TC(base, 1u);

	WRITE_PLL_MEM_MAP_CP_F_CAL(base, 1u);
	WRITE_PLL_MEM_MAP_CP_F_CAL_BITS(base, 32u);

	WRITE_PLL_MEM_MAP_TC_FLAG_STEP(base, 15u);
	/*Enable RTN feature for this PLL*/
	WRITE_PLL_MEM_MAP_TC_FLAG_STEP_EN(base, 1u);
}


/**
 *******************************************************************************
 * Function: pll_power_ctrl
 * @brief
 *
 * @details  Turn on/off the PLL power
 *
 * Parameters:
 * @param [in]    pll  - PLL to be controlled
 * @param [in]    state  - POWERUP/POWERDOWN
 * @param  [in]   base - Base address
 * @param  [in]   dig_core_base - Digital core base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_power_ctrl(PllSelName_e pll, uint8_t state, const uint64_t base, const uint64_t dig_core_base)
{
	/*  Powerup the PLL slices steps outlined by H/W */
	WRITE_PLL_MEM_MAP_SYNTH_PD(base, state);
	WRITE_PLL_MEM_MAP_SDM_PD(base, state);
	WRITE_PLL_MEM_MAP_CP_LEVELDET_PD(base, state);
	WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_PD(base, state);
	WRITE_PLL_MEM_MAP_PRESCALER_PD(base, state);
	WRITE_PLL_MEM_MAP_VCOTC_DAC_PD(base, state);
	WRITE_PLL_MEM_MAP_VCOTC_I2V_PD(base, state);

	/* Analog lock detector */
	WRITE_PLL_MEM_MAP_LD_PD(base, state);

	/* Power this down it will be turned on BEFORE vco cal */
	WRITE_PLL_MEM_MAP_VCOBUF_TO_PS_PD(base, POWERDOWN);

	/* Reset the PLL blocks, dividers and set default hw settings */
	pll_reset(pll, base);
	pll_reset_dividers(pll, base);
	pll_init_hw_defaults(pll, base);

	/* powerup the CP H/W */
	WRITE_PLL_MEM_MAP_CP_EN(base, ENABLE);

	WRITE_PLL_MEM_MAP_PFD_KILLUPDN(base, POWERUP);

	WRITE_PLL_MEM_MAP_DIGCORE_INTERFACE_KILLCLK(base, state);
	WRITE_PLL_MEM_MAP_DIGCORE_SAMPLE_KILLCLK(base, state);

	WRITE_PLL_MEM_MAP_LO_SYNC_SAMPLER_FLOPS_PD(base, POWERUP);
	WRITE_PLL_MEM_MAP_LO_SYNC_SAMPLER_LO_INPUT_BUF_PD(base, POWERUP);

	WRITE_PLL_MEM_MAP_ROOT_CLKDIV_RB(base, 1u);
	WRITE_PLL_MEM_MAP_ROOT_CLKDIV_KILLCLK(base, POWERUP);
	WRITE_PLL_MEM_MAP_LO_PHSYNC_QUAD2_KILLCLK(base, POWERUP);
	WRITE_PLL_MEM_MAP_LO_PHSYNC_LEAF_KILLCLK(base, POWERUP);

	/* Only for serdes but wont hurt other plls This needs to be the LAST reset*/
	WRITE_PLL_MEM_MAP_SERDES_PLL_ODIV_RB(base, 0u);
	WRITE_PLL_MEM_MAP_SERDES_PLL_ODIV_RB(base, 1u);
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("Power up Ctrl\n");
#endif
}

/**
 *******************************************************************************
 * Function: pll_reset_dividers
 * @brief
 *
 * @details  Reset PLL dividers
 *
 * Parameters:
 * @param [in]    pll  - PLL to be controlled
 * @param  [in]   base - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_reset_dividers(PllSelName_e pll, const uint64_t base)
{
	/* Toggle the phase sync */
	WRITE_PLL_MEM_MAP_LO_PHSYNC_LEAF_RB(base, 0u);
	WRITE_PLL_MEM_MAP_LO_PHSYNC_LEAF_RB(base, RESET);
	WRITE_PLL_MEM_MAP_LO_PHSYNC_QUAD2_RB(base, 0u);
	WRITE_PLL_MEM_MAP_LO_PHSYNC_QUAD2_RB(base, RESET);

	/* Setup one-time writes to the PLL H/W, toggle all resets. */
	WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_AB_COUNTER_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_SDM_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_LOCK_DETECT_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_MCS_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_LO_SYNC_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_VCO_TC_CAL_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_VCO_CAL_LOGIC_RESETB(base, 0u);

	WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_AB_COUNTER_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_SDM_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_LOCK_DETECT_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_MCS_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_LO_SYNC_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_VCO_TC_CAL_RESETB(base, RESET);
	WRITE_PLL_MEM_MAP_VCO_CAL_LOGIC_RESETB(base, RESET);

	WRITE_PLL_MEM_MAP_DIGCORE_DIV_RSTB(base, 0u);
	WRITE_PLL_MEM_MAP_DIGCORE_DIV_RSTB(base, RESET);

	WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(base, RESET);
}



/**
 *******************************************************************************
 * Function: pll_reset
 * @brief
 *
 * @details  Reset the PLL blocks
 *
 * Parameters:
 * @param [in]    pll  - PLL to be controlled
 * @param  [in]   base - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_reset(PllSelName_e pll, const uint64_t base)
{
	/*****************************************************************
	*  Toggle all the reset of each block in the PLL
	*****************************************************************/
	WRITE_PLL_MEM_MAP_PFD_RESETB(base, 0u);
	WRITE_PLL_MEM_MAP_PFD_RESETB(base, 1u);
	WRITE_PLL_MEM_MAP_NDIV_PS_RB(base, 0u);
	WRITE_PLL_MEM_MAP_NDIV_PS_RB(base, 1u);
	WRITE_PLL_MEM_MAP_LD_RSTB(base, 0u);
	WRITE_PLL_MEM_MAP_LD_RSTB(base, 1u);
	WRITE_PLL_MEM_MAP_VCOTC_DAC_RST(base, 1u);
	WRITE_PLL_MEM_MAP_VCOTC_DAC_RST(base, 0u);
}
/**
 *******************************************************************************
 * Function: pll_sdm_ref_clk_init()
 * @brief
 *
 * @details  Init the SDM and ref clocks for all the PLLS
 *
 * Parameters:
 * @param [in]    pll  - PLL to be controlled
 * @param  [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_sdm_ref_clk_init(PllSelName_e pll, const uint64_t base, const uint32_t refclk_freq)
{
	int rtnVal = NO_ERROR;

	gpPllStateSettings[pll]->refClkDiv = READ_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(base);
	gpPllStateSettings[pll]->refClock = refclk_freq;
	gpPllStateSettings[pll]->iBleedEnb = 1;
	gpPllStateSettings[pll]->doGcntMcs = false;
	gpPllStateSettings[pll]->cpCalEnb = 0u;
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("RefClk Init %d\n", gpPllStateSettings[pll]->refClkDiv);
#endif

	return rtnVal;
}


/**
 *******************************************************************************
 * Function: pll_calc_int_frac_words
 * @brief
 *
 * @details   Calculates the int and frac words for the PLL h/w.
 *
 * Parameters:
 * @param [in]    pll          PLL to be updated
 * @param  [in]   base - Base address
 *
 * @return      none.
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:   Set Clock PLL int/fraction dividers (Double precision math version)
 *             N = vcoFreqMhz / scaledRefClkMhz;
 *             integerWord=(uint16_t)floor(N);
 *             fractionalWord=((N )-integerWord);
 *             fractionalWord=fractionalWord * CLK_PLL_MODULUS;
 *             intFractionalWord=(uint32_t)ROUND(fractionalWord);
 *
 *******************************************************************************
 */
static void pll_calc_int_frac_words(PllSelName_e pll, const uint64_t base)
{
	uint16_t integerWord;
	uint32_t fractionalWord;
	uint64_t fractionalRemainder;
	const uint64_t pllModulus = (uint64_t)pll_get_dut_modulus(pll, base);

	integerWord = (uint16_t)(gpPllStateSettings[pll]->vcoFreqHz / gpPllStateSettings[pll]->refClock);
	fractionalRemainder = (uint32_t)(gpPllStateSettings[pll]->vcoFreqHz % gpPllStateSettings[pll]->refClock);

	/* +1 >> 1 is rounding (add .5) */
	fractionalWord = (uint32_t)(((fractionalRemainder * (pllModulus << 1ULL)
				      / (uint64_t)gpPllStateSettings[pll]->refClock) + 1ULL) >> 1ULL);

	/* if fractionalWord rounded up and == PLL modulus, fix it */
	if (fractionalWord == pllModulus) {
		fractionalWord = 0u;
		integerWord++;
	}

	/* Write the Int/Frac words to the PLL h/w Step 19. NOTE:  integerByte0 must BE LAST */
	gpPllStateSettings[pll]->fractionalByte0 = (uint8_t)(fractionalWord & BITM_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE0_SDM_FRAC);
	gpPllStateSettings[pll]->fractionalByte1 = (uint8_t)((fractionalWord >> 8u) & BITM_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE1_SDM_FRAC);
	gpPllStateSettings[pll]->fractionalByte2 = (uint8_t)((fractionalWord >> 16u) & BITM_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE2_SDM_FRAC);
	gpPllStateSettings[pll]->fractionalByte3 = (uint8_t)((fractionalWord >> 24u) & BITM_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE3_SDM_FRAC);
	gpPllStateSettings[pll]->integerByte1 = (uint8_t)((integerWord >> 8u) & BITM_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_INT_BYTE1_SDM_INT);
	gpPllStateSettings[pll]->integerByte0 = (uint8_t)(integerWord & BITM_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_INT_BYTE0_SDM_INT);
}

/*!
 *******************************************************************************
 * Function: pll_update_temp_comp
 * @brief
 *
 * @details  Update temp compensation in PLL
 *
 * @param [in]      pll - Pll index.
 * @param [in]      base - base address of the PLL
 *
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_update_temp_comp(PllSelName_e pll, const uint64_t base)
{
	float tcidac;
	uint8_t reg;
	float temperature;
	float idacSlp;
	float idacInt;
	float vcoampInt;
	float vcoampSlp;
	int rtnVal = NO_ERROR;
	uint16_t Sensor;
	bool found = false;

	/* Map the PLL into temp sensor # */
	/* note that since these calls are specifically for the PLL temp sensors
	 * which are on the primary core, checking for a valid index is not needed */
	switch (pll) {
	case PLL_CLKGEN_PLL:
	{
		Sensor = ADI_DEVTEMP_CLKPLL;
		break;
	}
	case PLL_SEC_CLKGEN_PLL:
	{
		Sensor = ADI_DEVTEMP_SEC_CLKPLL;
		break;
	}


	default:
	{
		rtnVal = ERROR_PLL_INVALID_PLL_ERROR;
		break;
	}
	}

	if (rtnVal == NO_ERROR) {
		/* Set the VCO select based on the desired VCO freq */
		if (gpPllStateSettings[pll]->vcoBand == PLL_VCO_HIGH_BAND) {
			idacSlp = PLL_TempComp[pll].idacSlpHigh;
			idacInt = PLL_TempComp[pll].idacIntHigh;
			vcoampInt = PLL_PeakTempCoef[pll].vcoampIntHigh;
			vcoampSlp = PLL_PeakTempCoef[pll].vcoampSlpHigh;
		} else {
			idacSlp = PLL_TempComp[pll].idacSlpLow;
			idacInt = PLL_TempComp[pll].idacIntLow;
			vcoampInt = PLL_PeakTempCoef[pll].vcoampIntLow;
			vcoampSlp = PLL_PeakTempCoef[pll].vcoampSlpLow;
		}

		rtnVal = tempr_run_measurement_get_sensor_temp(Sensor, &temperature, base);
#ifdef DUMP_PLL_SETTINGS_BL2
		INFO("Pll temp  %f\n", temperature);
#endif

		/* Calculate the Peak detector, search the PLL_PeakDetector table */
		if (rtnVal == NO_ERROR) {
			uint32_t numVals;
			uint32_t indx;

			/* Override the vcoAmp if override is selected */
			if (gpPllStateSettings[pll]->vcoAmpOverride == 0u)
				gpPllStateSettings[pll]->vcoAmp = vcoampInt + vcoampSlp * temperature;
			else
				gpPllStateSettings[pll]->vcoAmp = gpPllStateSettings[pll]->vcoAmpOverrideValue;

			/* Calc number of Power Scaling entries in the table to scan */
			numVals = sizeof(PLL_PeakDetector) / sizeof(PeakDet_t);

			/* Find the BW index*/
			for (indx = 0u; indx < numVals; indx++) {
				if (gpPllStateSettings[pll]->vcoAmp >= PLL_PeakDetector[indx].vAmp) {
					found = true;
					break;
				}
			}

			if (found == true) {
				/* Save values to be written later */
				gpPllStateSettings[pll]->vcoVrefSel = PLL_PeakDetector[indx].refSel;
				gpPllStateSettings[pll]->cmSel = PLL_PeakDetector[indx].cmSel;
				gpPllStateSettings[pll]->bypasDet = PLL_PeakDetector[indx].bypass;

				/* Set VCO output level and peak detectors */
				WRITE_PLL_MEM_MAP_VCOPKDET_CM_SEL(base, gpPllStateSettings[pll]->cmSel);
				WRITE_PLL_MEM_MAP_VCOPKDET_VREF_SEL(base, gpPllStateSettings[pll]->vcoVrefSel);
				WRITE_PLL_MEM_MAP_VCOPKDET_BYP_RES_DET(base, gpPllStateSettings[pll]->bypasDet);
			} else {
				rtnVal = ERROR_DEV_TEMP_SENSOR_INVALID_EN_ERROR;
			}
		}

		if (rtnVal == NO_ERROR) {
			tcidac = temperature * idacSlp + idacInt;
			/* Need to clip tcidac to max/min extreems and convert to int value */
			if (tcidac <= 0.0f)
				tcidac = 0.0f;
			else if (tcidac >= 4095.0f)
				tcidac = 4095.0f;
			else
				tcidac = tcidac;

			/* Write temp into TCIDAC Low word reg TCIDAC_0 */
			reg = (uint8_t)((uint16_t)tcidac & 0xFFu);
			WRITE_PLL_MEM_MAP_VCO_TC_CAL_REGS_TCIDAC_0_TCIDAC(base, reg);
			/* Write temp into TCIDAC High word reg TCIDAC_1 */
			reg = (uint8_t)((uint16_t)tcidac >> 8u);
			WRITE_PLL_MEM_MAP_VCO_TC_CAL_REGS_TCIDAC_1_TCIDAC(base, reg);

			/* Write TCUPDINIT reg TCCTL */
			WRITE_PLL_MEM_MAP_TCUPDINIT(base, 1u);

#ifdef DUMP_PLL_SETTINGS_BL2
			INFO("Pll %d Temp = %f %f \n", pll, temperature, tcidac);
#endif
		}
	}

	return rtnVal;
}


/**
 *******************************************************************************
 * Function: pll_wait_for_cp_valid_complete
 * @brief
 *
 * @details  Read CP CAL VALID and exit when equal to 1, read the bit twice
 *           before valid.
 *
 * Parameters:
 * @param  [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:  This code should not be altered in anyway. There is a h/w bug
 *         that this function is addressing and sequencing of hardware reads
 *         is critical.
 *
 *
 *******************************************************************************
 */
static int pll_wait_for_cp_valid_complete(const uint64_t base)
{
	uint8_t cpValid;
	int rtnVal = ERROR_PLL_CP_CAL_FAILED_ERROR;
	uint32_t exitLoop;

	for (exitLoop = 0; exitLoop != CP_CAL_WAIT_US; exitLoop++) {
		/* Read CP valid  and check if high */
		cpValid = READ_PLL_MEM_MAP_CP_CAL_VALID(base);

		if (cpValid == 1u) {
			/* Read CP valid  and check if high */
			cpValid = READ_PLL_MEM_MAP_CP_CAL_VALID(base);

			if (cpValid == 1u) {
				rtnVal = NO_ERROR;
				break;
			}
		} else {
			udelay(1u);
		}
	}
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("CP Cal %d\n", exitLoop);
#endif

	return rtnVal;
}

/**
 *******************************************************************************
 * Function: pll_wait_for_cp_init_complete
 * @brief
 *
 * @details  Read CP CAL INIT and exit when equal to 0, read the bit twice
 *           before valid.
 *
 * Parameters:
 * @param  [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:  This code should not be altered in anyway. There is a h/w bug
 *         that this function is addressing and sequencing of hardware reads
 *         is critical.
 *
 *******************************************************************************
 */
static int pll_wait_for_cp_init_complete(const uint64_t base)
{
	uint8_t cpInit;
	int rtnVal = ERROR_PLL_CP_CAL_FAILED_ERROR;
	uint32_t exitLoop;

	for (exitLoop = 0; exitLoop != CP_CAL_WAIT_US; exitLoop++) {
		/* Read CP Init and check if low */
		cpInit = READ_PLL_MEM_MAP_CP_CAL_INIT(base);

		if (cpInit == 0u) {
			/* Read CP Init and check if low */
			cpInit = READ_PLL_MEM_MAP_CP_CAL_INIT(base);

			if (cpInit == 0u) {
				rtnVal = NO_ERROR;
				break;
			}
		}
		udelay(1u);
	}
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_wait_for_cp_init_complete %d\n", exitLoop);
#endif

	return rtnVal;
}

/**
 *******************************************************************************
 * Function: pll_run_vco_cal
 * @brief
 *
 * @details  Runs the VCO cal, Coarse and Fine
 *
 * Parameters:
 * @param [in]    pll  - PLL instance ID
 * @param  [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_run_vco_cal(PllSelName_e pll, const uint64_t base)
{
	int rtnVal = NO_ERROR;
	uint8_t vcoCalBusy = 0u;
	uint8_t vcoCalInitAcked = 0u;
	uint32_t done;

	/* Turn ON but turn off after cal to reduce spurs */
	WRITE_PLL_MEM_MAP_KILLRDIV_VCOCAL(base, POWERUP);

	/* Enable the VCO cal, must be here to avoid noise */
	WRITE_PLL_MEM_MAP_VCOBUF_TO_PS_PD(base, POWERUP);

	/* Disable tracking and force update */
	WRITE_PLL_MEM_MAP_TCFORCEN(base, DISABLE);


#if (VCO_CAL_BESTBAND_FIX == 1u)
	/*Workaround to make 'pick best band' cal routine work correctly */

	/* Read whether tempcomp loop is shut down */
	uint8_t vco_tc_tracking_en;
	vco_tc_tracking_en = READ_PLL_MEM_MAP_VCO_TC_TRACKING_EN(base);

	/* Shut down VCO tempcomp loop */
	WRITE_PLL_MEM_MAP_VCO_TC_TRACKING_EN(base, DISABLE);

	/* Stop forcing coarse band from MEMMAP */
	WRITE_PLL_MEM_MAP_VCO_F_COARSE_BAND_EN(base, 0u);

	/* Disable fine band cal, force fine band to 18d */
	WRITE_PLL_MEM_MAP_VCO_FINE_CAL_EN(base, 0u);
	/* Must be done in this order */
	WRITE_PLL_MEM_MAP_VCO_F_FINE_BAND_EN(base, 1u);
	WRITE_PLL_MEM_MAP_VCO_F_FINE_BAND(base, 18u);
#endif

	/* Start the VCO cal */
	WRITE_PLL_MEM_MAP_VCO_CAL_INIT(base, 1u);

	/* Wait for VCO cal to start, up to SYNTH_CAL_WAIT_US microseconds */
	rtnVal = ERROR_PLL_VCO_CAL_FAILED_ERROR;
	for (done = 0; done != SYNTH_CAL_WAIT_US; done++) {
		/* This bitfield self clears when acked, so wait for it to
		 * go to zero.
		 */
		vcoCalInitAcked = READ_PLL_MEM_MAP_VCO_CAL_INIT(base);
		udelay(1u);
		if (vcoCalInitAcked == 0u) {
			rtnVal = NO_ERROR;
			break;
		}
	}
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("VCO cal %d\n", done);
#endif

	/* Wait for VCO cal to complete, up to SYNTH_CAL_WAIT_US microseconds */
	if (rtnVal == NO_ERROR) {
		rtnVal = ERROR_PLL_VCO_CAL_FAILED_ERROR;
		for (done = 0; done != SYNTH_CAL_WAIT_US; done++) {
			vcoCalBusy = READ_PLL_MEM_MAP_VCO_CAL_BUSY(base);
			udelay(1u);
			if (vcoCalBusy == 0u) {
				rtnVal = NO_ERROR;
				break;
			}
		}
	}

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_run_vco_cal %d\n", done);
#endif

#if (VCO_CAL_BESTBAND_FIX == 1u)
	/*Workaround for pick best band cal routine */

	/* Set VCO_TX_TRACKING_EN to whatever it was prior to cal start */
	WRITE_PLL_MEM_MAP_VCO_TC_TRACKING_EN(base, vco_tc_tracking_en);

	/* Read back coarse band result and store in VCO_F_COARSE_BAND*/
	uint8_t reg;

	reg = READ_PLL_MEM_MAP_VCO_F_FINE_BAND(base);

	reg = READ_PLL_MEM_MAP_PLL_BASE_REGS_VCO_F_COARSE_BAND_BYTE1_VCO_F_COARSE_BAND(base);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_VCO_F_COARSE_BAND_BYTE1_VCO_F_COARSE_BAND(base, reg);

	reg = READ_PLL_MEM_MAP_PLL_BASE_REGS_VCO_F_COARSE_BAND_BYTE0_VCO_F_COARSE_BAND(base);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_VCO_F_COARSE_BAND_BYTE0_VCO_F_COARSE_BAND(base, reg);

	/* Disable coarse band cal.  Force coarse band from MEMMAP */
	WRITE_PLL_MEM_MAP_VCO_COARSE_CAL_EN(base, 0u);

	WRITE_PLL_MEM_MAP_VCO_F_COARSE_BAND_EN(base, 1u);
	/* Enable fine band cal. Stop forcing fine band from MEMMAP */
	WRITE_PLL_MEM_MAP_VCO_FINE_CAL_EN(base, 1u);
	WRITE_PLL_MEM_MAP_VCO_F_FINE_BAND_EN(base, 0u);
	WRITE_PLL_MEM_MAP_VCO_F_FINE_BAND(base, 18u);


	/* Start the VCO cal */
	WRITE_PLL_MEM_MAP_VCO_CAL_INIT(base, 1u);
	rtnVal = ERROR_PLL_VCO_CAL_FAILED_ERROR;
	/* Wait for VCO cal to start, up to SYNTH_CAL_WAIT_US microseconds */
	for (done = 0; done != SYNTH_CAL_WAIT_US; done++) {
		/* This bitfield self clears when acked, so wait for it to
		 * go to zero.
		 */
		vcoCalInitAcked = READ_PLL_MEM_MAP_VCO_CAL_INIT(base);
		udelay(1);
		if (vcoCalInitAcked == 0u) {
			rtnVal = NO_ERROR;
			break;
		}
	}

	/* Wait for VCO cal to complete, up to SYNTH_CAL_WAIT_US microseconds */
	if (rtnVal == NO_ERROR) {
		done = false;
		while (done == false) {
			vcoCalBusy = READ_PLL_MEM_MAP_VCO_CAL_BUSY(base);

			if (vcoCalBusy == 0u) {
				done = true;
			} else if (GPT_Expired(timer)) {
				rtnVal = ERROR_PLL_VCO_CAL_FAILED_ERROR;
				done = true;
			} else {
				done = false;
			}
		}
	}
	/* Re-enable coarse band cal. */
	WRITE_PLL_MEM_MAP_VCO_COARSE_CAL_EN(base, 1u);
	reg = READ_PLL_MEM_MAP_VCO_F_FINE_BAND(base);

#endif

	/* Turn off to reduce spurs */
	WRITE_PLL_MEM_MAP_KILLRDIV_VCOCAL(base, POWERDOWN);

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("VCO cal  rtn %d\n", rtnVal);
#endif
	return rtnVal;
}


/**
 *******************************************************************************
 * Function: pll_rf_synth_cal_and_locked
 * @brief
 *
 * @details  Waits for a PLL lock for the selected PLL, return error if
 *           a timeout occurs.
 *
 * Parameters:
 * @param [in]   Pll name.
 * @param [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_rf_synth_cal_and_locked(PllSelName_e pll, const uint64_t base)
{
	int rtnVal = NO_ERROR;

	/* Clear CP Tri-States */
	WRITE_PLL_MEM_MAP_PFD_KILLUPDN(base, POWERUP);

	if (gpPllStateSettings[pll]->cpCalEnb == 1u)
		rtnVal = pll_run_cp_cal(pll, base);

#ifdef CP_CAL_NOT_NEEDED_YET
	/* Designers want to run CP cal as a test mode for now, there may be a time
	 * when it will always be enabled. The following code would be used if
	 * CP cal is needed
	 */

	/* Only run CP cal IF the new ICP is diff then the old ICP */
	if (gpPllStateSettings[pll]->LoopFilter.oldIcp != gpPllStateSettings[pll]->LoopFilter.ICP)
		/* Run the CP cal */
		rtnVal = PLL_RunCpCal(pll, TIMER_SYNTH_LOCK);

#endif
	/* Write to the charge pump current register, note this is done AFTER CP cal since
	 *  CP cal will change the settings. OR if its disabled they will never get written */
	WRITE_PLL_MEM_MAP_CP_I(base, gpPllStateSettings[pll]->LoopFilter.ICP);
	WRITE_PLL_MEM_MAP_CP_IBLEED(base, gpPllStateSettings[pll]->LoopFilter.Ibleed);

	/* check if we cal'd successfully */
	if (rtnVal == NO_ERROR) {
		/* Update the old ICP with new ICP */
		gpPllStateSettings[pll]->LoopFilter.oldIcp = gpPllStateSettings[pll]->LoopFilter.ICP;

		/* Write in the calculated LF values after CP, since CP over wrote them */
		pll_write_loop_RCs(pll, &(gpPllStateSettings[pll]->LoopFilter), base);

		/* Update the PLL temp compensation h/w */
		rtnVal = pll_update_temp_comp(pll, base);
	}

	/* Run VCO cal */
	if (rtnVal == NO_ERROR)
		rtnVal = pll_run_vco_cal(pll, base);
	/* check if we cal'd successfully */

	if (rtnVal == NO_ERROR) {
		/* if cal was successful, wait for lock, the wait time is EffectLoopBW/10 as defined by H/W */
		/* Wait for LOCK bit to end */
		rtnVal = pll_lock_detect(pll, (gpPllStateSettings[pll]->LoopFilter.effectiveloopBW / 10u), base);
	}

#if (SUPPORT_BLEED_CAL == 1u)
	/* check if we cal'd sucessfully */
	if (rtnVal == NO_ERROR) {
		/* Run the bleed ramp now */
		if ((pll == PLL_RF0_PLL) || (pll == PLL_RF1_PLL)) {
			rtnVal = pll_bleed_ramp(pll, base);

#if (PHSYNC_ENABLE_PHASE_SYNC == 1u)
			/* init Phase Sync hardware if selected */
			if ((rtnVal == NO_ERROR) && (gpPllStateSettings[pll]->phaseSyncMode != PHSYNC_OFF))
				rtnVal = DrvPhaseSync_InitLeafMcs(pll);

#endif


#if (PHSYNC_ENABLE_GCNT_SYNC == 1u)
			/* check if we cal'd sucessfully */
			if ((rtnVal == NO_ERROR) && (gpPllStateSettings[pll]->doGcntMcs == true)) {
				/* Do GCNT MCS */
				DrvPhaseSync_rfPllMcsEnable(pll);
				rtnVal = DrvPhaseSync_IssueGcntMcs(pll);
				DrvPhaseSync_rfPllMcsDisable(pll);
			} else {
				/* Do internal MCS, ONLY for debug purposes */
				rtnVal = DrvPhaseSync_InitRfMcs(pll);
			}
#endif
		}
	}
#endif  /* SUPPORT_BLEED_CAL */

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_rf_synth_cal_and_locked %d\n", rtnVal);
#endif
	return rtnVal;
}



/**
 *******************************************************************************
 * Function: pll_lock_detect
 * @brief
 *
 * @details  Wait for a lock detect
 *
 * Parameters:
 * @param [in]      pll - which PLL
 * @param [in]      wait - wait time in uSec.
 * @param [in]      base - base address of the PLL
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_lock_detect(PllSelName_e pll, uint32_t waitUsec, const uint64_t base)
{
	int rtnVal;

	uint8_t lockDetected = 0u;
	uint32_t timer;

	rtnVal = ERROR_PLL_SYNTH_LOCK_FAILED_ERROR;
	/* Wait for a lock, up to waitUsec microseconds */
	for (timer = 0; timer != waitUsec; timer++) {
		lockDetected = READ_PLL_MEM_MAP_SYN_LOCK(base);

		if (lockDetected == 1u) {
			rtnVal = NO_ERROR;
			break;
		} else {
			udelay(1u);
		}
	}

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_lock_detect %d\n", rtnVal);
#endif

	return rtnVal;
}


/**
 *******************************************************************************
 * Function: pll_calculate_cp_params
 * @brief
 *
 * @details  Calculates the CP coef to avoind math function
 *
 * Parameters:
 * @param [in]      pll - which PLL
 *
 * @return          cpCalClkDiv
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static float pll_calculate_cp_params(PllSelName_e pll)
{
	return cpCalcValue[0].cpCalClkDiv;
}

/**
 *******************************************************************************
 * Function: pll_run_cp_cal
 * @brief
 *
 * @details  Runs the CP cal, Coarse and Fine
 *
 * Parameters:
 * @param [in]      pll - which PLL
 * @param [in]      base - base address of the PLL
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_run_cp_cal(PllSelName_e pll, const uint64_t base)
{
	int rtnVal = NO_ERROR;
	LoopFilterResult_t lFCoef = { 0 };
	uint8_t bypassC1;
	uint8_t bypassC2;
	float capVal;
	float iChargePump;
	float cpCalClkDiv;
	float tmeasure;

	/* Disable CP aux ADC and Ibleed */
	WRITE_PLL_MEM_MAP_CP_SEL_ADC_TC(base, DISABLE);
	WRITE_PLL_MEM_MAP_CP_BLEED_DN_EN(base, DISABLE);

	/* Write to the charge pump current register */
	WRITE_PLL_MEM_MAP_CP_I(base, gpPllStateSettings[pll]->LoopFilter.ICP);
	WRITE_PLL_MEM_MAP_CP_IBLEED(base, gpPllStateSettings[pll]->LoopFilter.Ibleed);

	WRITE_PLL_MEM_MAP_LF_BYPASS_R1(base, 1u);
	WRITE_PLL_MEM_MAP_LF_BYPASS_R3(base, 1u);
	/* 150uA setting */
	if (gpPllStateSettings[pll]->LoopFilter.ICP == 0u) {
		capVal = 375e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 15u;
		lFCoef.C2 = 4u;
		lFCoef.C3 = 0u;
		bypassC1 = 1u;
		bypassC2 = 1u;
	}
	/* 300uA setting */
	else if (gpPllStateSettings[pll]->LoopFilter.ICP == 1u) {
		capVal = 750e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 31u;
		lFCoef.C2 = 2u;
		lFCoef.C3 = 0u;
		bypassC1 = 1u;
		bypassC2 = 1u;
	}
	/* 450uA setting */
	else if (gpPllStateSettings[pll]->LoopFilter.ICP == 2u) {
		capVal = 1125e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 47u;
		lFCoef.C2 = 0u;
		lFCoef.C3 = 0u;
		bypassC1 = 1u;
		bypassC2 = 1u;
	}
	/* 600uA setting */
	else if (gpPllStateSettings[pll]->LoopFilter.ICP == 3u) {
		capVal = 1500e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 62u;
		lFCoef.C2 = 4u;
		lFCoef.C3 = 0u;
		bypassC1 = 1u;
		bypassC2 = 1u;
	}
	/* 750uA setting */
	else if (gpPllStateSettings[pll]->LoopFilter.ICP == 4u) {
		capVal = 1875e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 48u;
		lFCoef.C2 = 12u;
		lFCoef.C3 = 0u;
		bypassC1 = 0u;
		bypassC2 = 0u;
	}
	/* 900uA setting */
	else if (gpPllStateSettings[pll]->LoopFilter.ICP == 5u) {
		capVal = 2250e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 63u;
		lFCoef.C2 = 16u;
		lFCoef.C3 = 0u;
		bypassC1 = 0u;
		bypassC2 = 0u;
	}
	/* 1050uA setting */
	else if (gpPllStateSettings[pll]->LoopFilter.ICP == 6u) {
		capVal = 2593e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 63u;
		lFCoef.C2 = 63u;
		lFCoef.C3 = 63u;
		bypassC1 = 0u;
		bypassC2 = 0u;
	}
	/* 1500uA+ setting */
	else {
		capVal = 2593e-12f;
		iChargePump = 150e-6f * (1.0f + (float)gpPllStateSettings[pll]->LoopFilter.ICP);
		lFCoef.C1 = 63u;
		lFCoef.C2 = 63u;
		lFCoef.C3 = 63u;
		bypassC1 = 0u;
		bypassC2 = 0u;
	}

	WRITE_PLL_MEM_MAP_LF_BYPASS_C2(base, bypassC1);
	WRITE_PLL_MEM_MAP_LF_BYPASS_C1(base, bypassC2);

	pll_write_loop_RCs(pll, &lFCoef, base);

	/* This is a workaround to avoind FP math functions. */
	tmeasure = PLL_VTARGET * capVal / (iChargePump * 0.002f);
	tmeasure = tmeasure + 1.0f;

	cpCalClkDiv = pll_calculate_cp_params(pll);
	WRITE_PLL_MEM_MAP_CP_CAL_CLK_DIVIDE(base, (uint8_t)cpCalClkDiv);


	/* Start the charge pump cal. */
	WRITE_PLL_MEM_MAP_CP_CAL_EN(base, ENABLE);
	WRITE_PLL_MEM_MAP_CP_CAL_INIT(base, ENABLE);

	/* Wait for Cal to start */
	rtnVal = pll_wait_for_cp_init_complete(base);
	/* Wait for Cal to complete */
	if (rtnVal == NO_ERROR)
		rtnVal = pll_wait_for_cp_valid_complete(base);

	/* Take all CP R&Cs out of Bypass */
	WRITE_PLL_MEM_MAP_LF_BYPASS_R1(base, 0u);
	WRITE_PLL_MEM_MAP_LF_BYPASS_R3(base, 0u);
	WRITE_PLL_MEM_MAP_LF_BYPASS_C2(base, 0u);
	WRITE_PLL_MEM_MAP_LF_BYPASS_C1(base, 0u);

	/* Disable CP cal h/w and enable ADC TC */
	WRITE_PLL_MEM_MAP_CP_SEL_ADC_TC(base, ENABLE);

	WRITE_PLL_MEM_MAP_CP_CAL_EN(base, DISABLE);

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_run_cp_cal  %d\n", rtnVal);
#endif

	return rtnVal;
}


/**
 *******************************************************************************
 * Function: pll_bleed_ramp
 * @brief
 *
 * @details  Runs the Bleed ramp
 *
 * Parameters:
 * @param [in]      pll - PLL instance ID
 * @param [in]      base - base address of the PLL
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
int pll_bleed_ramp(PllSelName_e pll, const uint64_t base)
{
	int rtnVal = NO_ERROR;
	uint8_t brCalBusy = 0u;

	/* The Bleed ramp cal procedure is:
	 * 1) Start the cal (set BLEED_RAMP_INIT bit)
	 * 2) Wait for the cal to start (wait for BLEED_RAMP_INIT bit to clear)
	 * 3) Wait for the cal to complete (wait for BLEED_RAMP_DONE bit to clear)
	 * We must do #2 because it is possible for the hardware to not
	 * "see" our start indication (#1) before we read the busy flag (#3).
	 * #2 forces us to wait until the hardware has acked the start indication.
	 */
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll bleed ramp  %d\n", rtnVal);
#endif

	/* Justin to check setting */
	WRITE_PLL_MEM_MAP_BLEED_RAMP_STEP(base, 1u);
	WRITE_PLL_MEM_MAP_BLEED_RAMP_TIME_STEP(base, 15u);

	/* Start the Bleed Ramp cal */
	WRITE_PLL_MEM_MAP_CP_BLEED_DN_EN(base, gpPllStateSettings[pll]->iBleedEnb);
	WRITE_PLL_MEM_MAP_CP_BLEED_UP_EN(base, 0u);
	WRITE_PLL_MEM_MAP_BLEED_RAMP_EN(base, ENABLE);
	WRITE_PLL_MEM_MAP_BLEED_RAMP_INIT(base, ENABLE);

	/* Wait for bleed ramp cal to complete, up to BLEED_CAL_WAIT_US microseconds */
	if (rtnVal == NO_ERROR) {
		udelay(BLEED_CAL_WAIT_US);
		{
			brCalBusy = READ_PLL_MEM_MAP_BLEED_RAMP_DONE(base);

			if (brCalBusy == 0u)
				rtnVal = NO_ERROR;
			else
				rtnVal = ERROR_PLL_BLEED_CAL_FAILED_ERROR;
		}
	}

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_bleed_ramp  %d\n", rtnVal);
#endif

	return rtnVal;
}


/**
 *******************************************************************************
 * Function: pll_write_loop_RCs
 * @brief
 *
 * @details  Calc the pre-calculated Loopfilter Caps and Resistors for the desired PLL.
 *
 * Parameters:
 * @param [in]  pll     - which PLL.
 * @param [in]  pLFCoef - Pointer tp loopfilter values
 * @param [in]  base    - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_write_loop_RCs(PllSelName_e pll, LoopFilterResult_t *pLf, const uint64_t base)
{
	WRITE_PLL_MEM_MAP_LF_C1(base, pLf->C1);
	WRITE_PLL_MEM_MAP_LF_C2(base, pLf->C2);
	WRITE_PLL_MEM_MAP_LF_C3(base, pLf->C3);
	WRITE_PLL_MEM_MAP_LF_R1(base, pLf->R1);
	WRITE_PLL_MEM_MAP_LF_R3(base, pLf->R3);
	WRITE_PLL_MEM_MAP_LF_BYPASS_C1(base, pLf->C1Bypass);
	WRITE_PLL_MEM_MAP_LF_BYPASS_C2(base, pLf->C2Bypass);
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_write_loop_RCs\n");
#endif
}


/**
 *******************************************************************************
 * Function: pll_set_dut_modulus
 * @brief    Set the DUT modulus
 *
 * @details
 *
 * Parameters:
 * @param [in]    pll - PLL type
 * @param [in]    modulus - value to set
 * @param  [in]   base - Base address
 *
 * @return        None
 *
 * Reference to other related functions
 * @sa  pll_get_dut_modulus
 *
 * Notes:
 *
 *******************************************************************************
 */
void pll_set_dut_modulus(PllSelName_e pll, uint32_t modulus, const uint64_t base)
{
	WRITE_PLL_MEM_MAP_SDM_REGS_MOD0_0_MOD0(base, (uint8_t)((modulus >> 0u) & 0xFFu));
	WRITE_PLL_MEM_MAP_SDM_REGS_MOD0_1_MOD0(base, (uint8_t)((modulus >> 8u) & 0xFFu));
	WRITE_PLL_MEM_MAP_SDM_REGS_MOD0_2_MOD0(base, (uint8_t)((modulus >> 16u) & 0xFFu));
	WRITE_PLL_MEM_MAP_SDM_REGS_MOD0_3_MOD0(base, (uint8_t)((modulus >> 24u) & 0xFFu));
}

/**
 *******************************************************************************
 * Function: pll_get_dut_modulus
 * @brief    Get the DUT modulus
 *
 * @details
 *
 * Parameters:
 * @param [in]    pll - PLL type
 * @param  [in]   base - Base address
 *
 * @return        uint32_t Modulus
 *
 * Reference to other related functions
 * @sa  pll_set_dut_modulus
 *
 *
 * Notes:
 *
 *******************************************************************************
 */
static uint32_t pll_get_dut_modulus(PllSelName_e pll, const uint64_t base)
{
	uint32_t modulus;

	modulus = ((uint32_t)READ_PLL_MEM_MAP_SDM_REGS_MOD0_0_MOD0(base) |
		   ((uint32_t)READ_PLL_MEM_MAP_SDM_REGS_MOD0_1_MOD0(base) << 8u) |
		   ((uint32_t)READ_PLL_MEM_MAP_SDM_REGS_MOD0_2_MOD0(base) << 16u) |
		   ((uint32_t)READ_PLL_MEM_MAP_SDM_REGS_MOD0_3_MOD0(base) << 24u));
	return modulus;
}



/**
 *******************************************************************************
 * Function: pll_write_synth_parameters
 * @brief
 *
 * @details  Writes the pre-calculated PLL values for the desired PLL.
 *
 * Parameters:
 * @param [in]    pll - which PLL.
 * @param  [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_write_synth_parameters(PllSelName_e pll, const uint64_t base)
{
	int rtnVal = NO_ERROR;

	float temp;
	uint8_t val;
	uint8_t byte0;
	uint8_t byte1;

	/* If override the refClk write to the ref clk divider */
	if (gpPllStateSettings[pll]->refClkOverride == 1u)
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(base, (uint8_t)gpPllStateSettings[pll]->refClkOverrideValue);

	/* setup VCO TC wait step 17. I had to split this up due to Misra rule 10.4 */
	temp = ((float)gpPllStateSettings[pll]->refClock / 16384.0f * PLL_TC_UPDATE_RATE);
	val = (uint8_t)utils_floorLog2U32((uint32_t)temp);

	/* Clip if LOG returns 0xFF */
	if (val == 0xFFu)
		val = 0u;
	else
		val = (uint8_t)(val + 1u);


	WRITE_PLL_MEM_MAP_VCO_TC_WAIT(base, val);

	/* System C will generate a bunch of warnigns (non-critical) so want to have the option to enable/disable this */
	/* We need to make sure the VCOcal hardware is reset prior to changing VCO_SEL */
	if (plat_is_sysc() == true)
		pll_reset_dividers(pll, base);

	/* Write the Int/Frac words to the PLL h/w Step 19. NOTE:  integerByte0 must BE LAST */
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE0_SDM_FRAC(base, gpPllStateSettings[pll]->fractionalByte0);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE1_SDM_FRAC(base, gpPllStateSettings[pll]->fractionalByte1);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE2_SDM_FRAC(base, gpPllStateSettings[pll]->fractionalByte2);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_FRAC_BYTE3_SDM_FRAC(base, gpPllStateSettings[pll]->fractionalByte3);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_INT_BYTE1_SDM_INT(base, gpPllStateSettings[pll]->integerByte1);
	WRITE_PLL_MEM_MAP_PLL_BASE_REGS_DIVIDER_INT_BYTE0_SDM_INT(base, gpPllStateSettings[pll]->integerByte0);

	/* Setup the LO divider */
	WRITE_PLL_MEM_MAP_LODIV_RB(base, 1u);

	if (gpPllStateSettings[pll]->rootDivHw == 0u) {
		WRITE_PLL_MEM_MAP_LODIV_FUND(base, 1u);
		WRITE_PLL_MEM_MAP_LODIV_THERMCODE(base, 0x0u);
	} else {
		WRITE_PLL_MEM_MAP_LODIV_FUND(base, 0u);
		WRITE_PLL_MEM_MAP_LODIV_THERMCODE(base, gpPllStateSettings[pll]->rootDivHw);
	}

	/* Need to reset the VCO, and must be done BEFORE VCO_SEL */
	WRITE_PLL_MEM_MAP_VCO_CAL_LOGIC_RESETB(base, 0u);
	/* Stop forcing ALC code from memmap*/
	WRITE_PLL_MEM_MAP_VCO_F_ALC_EN(base, DISABLE);

	/* Wait for ALC to complete */
	udelay(FORCE_ALC_WAIT_US);
	byte0 = READ_PLL_MEM_MAP_PLL_BASE_REGS_F_ALC_BYTE0_VCO_F_ALC(base);
	byte1 = READ_PLL_MEM_MAP_PLL_BASE_REGS_F_ALC_BYTE1_VCO_F_ALC(base);

	if ((byte0 == 0u) && (byte1 == 0u))
		rtnVal = NO_ERROR;
	else
		rtnVal = ERROR_PLL_FORCED_ALC_TIMEOUT;

	WRITE_PLL_MEM_MAP_VCO_CAL_LOGIC_RESETB(base, 1u);

	if (rtnVal == NO_ERROR) {
		/* Set the VCO select based on the desired VCO freq */
		if (gpPllStateSettings[pll]->vcoBand == PLL_VCO_HIGH_BAND)
			WRITE_PLL_MEM_MAP_VCO_SEL(base, VCO_SEL_HB);
		else
			WRITE_PLL_MEM_MAP_VCO_SEL(base, VCO_SEL_LB);

		/* Write in VCO Ptat value */
/*      WRITE_PLL_MEM_MAP_LDO_VPTATCTRL(base, gpPllStateSettings[pll]->vcoPtatCtrl);  */

		/* Write in VCO LUT value */
		WRITE_PLL_MEM_MAP_VCO_VAR_TC(base, gpPllStateSettings[pll]->vcoVarTc);

		/* Set Varactor setting */
		WRITE_PLL_MEM_MAP_VCO_VAR(base, gpPllStateSettings[pll]->vcoVaractor);
	}

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_write_synth_parameters\n");
#endif
	return rtnVal;
}

/**
 *******************************************************************************
 * Function: pll_compute_synth_parameters
 * @brief
 *
 * @details  Calculate the PLL paramters and populate into the selected
 *           PLL config array
 *
 * Parameters:
 * @param [in]  pll - which PLL
 * @param [in]  base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static int pll_compute_synth_parameters(PllSelName_e pll, const uint64_t base)
{
	int rtnVal = NO_ERROR;

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("pll_compute_synth_parameters\n");
#endif

	/* Set the VCO band based on the desired VCO freq */
	if ((gpPllStateSettings[pll]->vcoFreqHz / 1000ull) >= pllBandSelectFreq_kHz[pll])
		gpPllStateSettings[pll]->vcoBand = PLL_VCO_HIGH_BAND;
	else
		gpPllStateSettings[pll]->vcoBand = PLL_VCO_LOW_BAND;
	/* Save the band select freq */
	gpPllStateSettings[pll]->vcoBandSelFreq_kHz = pllBandSelectFreq_kHz[pll];

	/* Set freq to 0 indicating that a re-programming is going to occure */
	gpPllStateSettings[pll]->PllFrequency = 0ul;
	gpPllStateSettings[pll]->rootDivHw = (uint8_t)(gpPllStateSettings[pll]->rootDiv - 1u);

	pll_calc_int_frac_words(pll, base);

	/* Set the VCO LUT as function of VCO Freq step 18 */
	pll_update_vco_tables(pll);
	return rtnVal;
}



/**
 *******************************************************************************
 * Function: pll_update_vco_tables
 * @brief
 *
 * @details  Update the VCO tables based on VCO freq
 *
 * Parameters:
 * @param [in]      which PLL
 *
 * @return          none
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
static void pll_update_vco_tables(PllSelName_e pll)
{
	uint64_t vcoMhz = gpPllStateSettings[pll]->vcoFreqHz / 1000000u;
	uint32_t index;
	uint32_t tabSize = 0u;
	const VcoCoef_t *pVcoTable = NULL;


	pVcoTable = pPLL_TempCompClkPLL;
	tabSize = VCO_COEFF_CLK_TABLE_SIZE;

	/* Scan the table for a match */
	for (index = 0u; index != tabSize; index++)
		if ((vcoMhz >= pVcoTable[index].f1Mhz) && (vcoMhz <= pVcoTable[index].f2Mhz))
			break;
	/* Update the main PLL table */
	gpPllStateSettings[pll]->vcoVarTc = pVcoTable[index].vcoVarTc;
	gpPllStateSettings[pll]->vcoVaractor = pVcoTable[index].vcoVaractor;

	/* This will eventually go into temp comp and sub an equation */
	gpPllStateSettings[pll]->vcoPkDet = pVcoTable[index].vcoPkDet;
	gpPllStateSettings[pll]->vcoPtatCtrl = pVcoTable[index].vcoPtatCtrl;
#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("Update VCO tables\n");
#endif
}



/**
 *******************************************************************************
 * Function: pll_clk_power_init
 * @brief
 *
 * @details  One time Powerup init fo the Clk, setting up tables and
 *           powerup of the h/w.
 *
 * Parameters:
 * @param  [in]   base - Base address
 * @param  [in]   dig_core_base - Digital core base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
extern int pll_clk_power_init(const uint64_t base, const uint64_t dig_core_base, const uint64_t freq, const uint32_t refclk_freq, PllSelName_e pll)
{
	int rtnVal = NO_ERROR;

	/* Initialize global pll settings */
	memset(&pllStateSettings, 0, sizeof(pllStateSettings));

	gpPllStateSettings[pll] = &pllStateSettings[pll];

	assert((freq == CLK_VCO_7G_HZ) || (freq == CLK_VCO_11G_HZ));
	if (freq == CLK_VCO_7G_HZ)
		gpPllStateSettings[pll]->rootDiv = 8U;
	else
		gpPllStateSettings[pll]->rootDiv = 12U;

	if (clkLoDiv[CLK_7G] == gpPllStateSettings[pll]->rootDiv) {
		gpPllStateSettings[pll]->clkFreqIndex = CLK_7G;
		gpPllStateSettings[pll]->PllFrequency = CLK_VCO_7G_HZ;
		gpPllStateSettings[pll]->PllFrequency_kHz = CLK_VCO_7G_KHZ;
		gpPllStateSettings[pll]->vcoFreqHz = CLK_VCO_7G_HZ;
	} else if (clkLoDiv[CLK_11G] == gpPllStateSettings[pll]->rootDiv) {
		gpPllStateSettings[pll]->clkFreqIndex = CLK_11G;
		gpPllStateSettings[pll]->PllFrequency = CLK_VCO_11G_HZ;
		gpPllStateSettings[pll]->PllFrequency_kHz = CLK_VCO_11G_KHZ;
		gpPllStateSettings[pll]->vcoFreqHz = CLK_VCO_11G_HZ;
	} else {
		rtnVal = ERROR_PLL_INVALID_FREQ_ERROR;
	}

	if (rtnVal == NO_ERROR) {
		/* Init the PLL data stuctures */
		pll_init_pll_tables(pll);
		rtnVal = pll_sdm_ref_clk_init(pll, base, refclk_freq);
	}

	if (rtnVal == NO_ERROR) {
		/* Not sure what buffers need to be enabled, check with Jason Fan */


		/* Powerup the PLL */
		pll_power_ctrl(pll, POWERUP, base, dig_core_base);
	}

#ifdef DUMP_PLL_SETTINGS_BL2
	INFO("Power Init, status = %d\n", rtnVal);
#endif

	return rtnVal;
}
/**
 *******************************************************************************
 * Function: pll_program_clock_pll_clock
 * @brief
 *
 * @details: Main API for controlling the System clock.
 *
 *
 * Parameters:
 * @param  [in]   base - Base address
 *
 * @return      Error Code
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
extern int pll_program_clock_pll_clock(const uint64_t base, PllSelName_e pll)
{
	int rtnVal = NO_ERROR;

	rtnVal = pll_compute_synth_parameters(pll, base);

	/* Write the PLL settings to H/W */
	if (rtnVal == NO_ERROR) {
		pll_init_loop_filter(pll, gpPllStateSettings[pll]);
		pll_calc_loop_filter_coef(pll, gpPllStateSettings[pll]);

		rtnVal = pll_write_synth_parameters(pll, base);
	}

	/* Calibrate and wait for lock */
	if (rtnVal == NO_ERROR)
		rtnVal = pll_rf_synth_cal_and_locked(pll, base);

	return rtnVal;
}

/**
 *******************************************************************************
 * Function: pll_configure_vco_test_out
 * @brief
 *
 * @details: Routes the pll at base to its corresponding test output pin
 *
 *
 * Parameters:
 * @param  [in]   base - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
void pll_configure_vco_test_out(const uint64_t base)
{
	WRITE_PLL_MEM_MAP_CLKOUT_BUF_PD(base, 0);
	WRITE_PLL_MEM_MAP_TEST_CLKDIV_RSTB(base, 1);
	WRITE_PLL_MEM_MAP_TEST_CLKDIV_SAMPLE_KILLCLK(base, 0);

	return;
}

/**
 *******************************************************************************
 * Function: pll_configure_vtune_test_out
 * @brief
 *
 * @details: Routes the Vtune output of the pll at base out to its test pin
 *
 *
 * Parameters:
 * @param  [in]   base - Base address
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 * Notes:
 *
 *******************************************************************************
 */
void pll_configure_vtune_test_out(const uint64_t base)
{
	WRITE_PLL_MEM_MAP_VT_FORCE(base, 1);
	WRITE_PLL_MEM_MAP_TCFORCEN(base, 1);
	return;
}
