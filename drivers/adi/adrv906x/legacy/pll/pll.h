/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLL_H
#define PLL_H

#include "../adi_errors/adi_errors.h"
#include "../regmap/pll_regmap.h"
#include "../utils/utils.h"
#include "../hardware_if/hardware_if.h"
#include "../temperature/temperature.h"

#define     PLL_CAL_MAX_CNT_NORMAL                (500000ul)

#define     VCO_COEFF_SERDES_TABLE_SIZE           (9u)
#define     FORCE_ALC_WAIT_US                     (10U)                 /* 10  usec timeout for force ALC wait */
#define     PLL_TC_UPDATE_RATE                    (0.0666e-3f)
#define     VCO_SEL_HB                            (0x1u)                /* VCO_SEL value for 2 Core HB VCO */
#define     VCO_SEL_LB                            (0x2u)                /* VCO_SEL value for 2 Core LB VCO */
#define     PLL_VTARGET                           (42.5e-3f)


#define SYNTH_CAL_WAIT_US                         (10000U)              /* 100 msec timeout for calibration */
#define CP_CAL_WAIT_US                            (100000U)             /* 100 msec timeout for calibration */
#define BLEED_CAL_WAIT_US                         (10000U)              /* 100 usec timeout for bleed cal wait */
#define FORCE_ALC_WAIT_US                         (10U)                 /* 10  usec timeout for force ALC wait */
#define LOCKDET_WAIT_US                           (10000U)              /* 100  msec timeout for force ALC wait */

#define CLK_VCO_7G_HZ                                                     (7864320000LL)
#define CLK_VCO_11G_HZ                                                    (11796480000LL)

#define CLK_VCO_7G_MHZ                                                    (CLK_VCO_7G_HZ / 1000000UL)
#define CLK_VCO_11G_MHZ                                                   (CLK_VCO_11G_HZ / 1000000UL)

#define CLK_VCO_7G_KHZ                                                    (CLK_VCO_7G_HZ / 1000ULL)
#define CLK_VCO_11G_KHZ                                                   (CLK_VCO_11G_HZ / 1000ULL)

/* Frequency at which we switch to the HB VCO */
#define VCO_HB_THRESHOLD_FREQ_CLK_HZ        (10050000000ull)

/* VCO Frequency limits for the ClkPLL */
#define MIN_VCO_FREQ_CLK_HZ                 (7100000000ULL)
#define MAX_VCO_FREQ_CLK_HZ                 (14200000000ULL)

#define MIN_VCO_FREQ_CLK_KHZ                (MIN_VCO_FREQ_CLK_HZ / 1000UUL)
#define MAX_VCO_FREQ_CLK_KHZ                (MAX_VCO_FREQ_CLK_HZ / 1000UUL)

#define VCO_HB_THRESHOLD_FREQ_RF_KHZ        ((10050000000ull) / 1000UL)
#define VCO_HB_THRESHOLD_FREQ_CLK_KHZ       (VCO_HB_THRESHOLD_FREQ_CLK_HZ / 1000UL)

#define MIN_VCO_FREQ_CLK_MHZ                (MIN_VCO_FREQ_CLK_HZ / 1000000UL)
#define MAX_VCO_FREQ_CLK_MHZ                (MAX_VCO_FREQ_CLK_HZ / 1000000UL)
#define VCO_HB_THRESHOLD_FREQ_SERDES_KHZ    1000u

#define VCO_HB_THRESHOLD_FREQ_CLK_MHZ       (VCO_HB_THRESHOLD_FREQ_CLK_HZ / 1000000UL)

#define VCO_COEFF_CLK_TABLE_SIZE            (9u)


#define TEMP_SENSOR_DFL_OFFSET_CODE_VALUE    (16.0f)    /* Default RTL offset code value */
#define KELVIN_TO_CELSIUS                    (273.0f)   /* Kelvin to Celsius conversion */
#define TEMP_SLOPE_VALUE                     (1.00f)    /* Slope part of the Kelvin to Celsius conversion */
#define TCIDAC_125C                          (328.0f)
#define TCIDAC_MAX                           (4095.0f)
#define IDAC_INT_LOWBAND                     (2556.2f)
#define TEMP_TIMEOUT_US                      (10000u)

#define PLL_LOCK_DETECTOR_TIME               (3u)

#define SCHED_TEMPSENS_UPDATE_TIME_MS        (1000u)
#define RAWABSZERO                           (0xD6AE)   /* equiv of -274.0 deg C */
#define TEMP_ROUND                           (5000)     /* half to add for rounding */
#define TEMP_ADC_DITHER_SETTING1             (0xFu)     /* static negative input offset inside the comparator (DS ADC) set to 3u amp
	                                                 */
#define TEMP_ADC_DITHER_SETTING2             (0xBu)     /* 0x01; random dither to output rounding of decimation filter
	                                                 * 0x02; Powersdown_bar the mbias block
	                                                 * 0x08; Adds extra cap to reduce oscillator frequency in temp sensor
	                                                 */

#define PLL_INIT_CP_CURRENT                  (0xffu)
#define NUM_REF_CLOCK_SETTINGS               (2)

typedef enum {
	PLL_VCO_LOW_BAND,
	PLL_VCO_HIGH_BAND,
}
PllVcoBand_e;

typedef struct {
	float vcoampIntLow;
	float vcoampSlpLow;
	float vcoampIntHigh;
	float vcoampSlpHigh;
} PeakTempCoef_t;

typedef struct {
	float idacSlpLow;
	float idacIntLow;
	float idacSlpHigh;
	float idacIntHigh;
} TempCoef_t;

/* Peak Detector seach table */
typedef struct {
	float vAmp;
	uint8_t cmSel;
	uint8_t refSel;
	uint8_t bypass;
} PeakDet_t;

typedef struct {
	uint32_t vcoFreq_mHz;
	uint8_t C1;                             /*!< Loopfilter C1 value */
	uint8_t C2;                             /*!< Loopfilter C2 value */
	uint8_t C1Bypass;                       /*!< Loopfilter C1 Bypass value */
	uint8_t C2Bypass;                       /*!< Loopfilter C2 Bypass value */
	uint8_t C3;                             /*!< Loopfilter C3 value */
	uint8_t R1;                             /*!< Loopfilter R1 value */
	uint8_t R3;                             /*!< Loopfilter R3 value */
	uint8_t ICP;                            /*!< Loopfilter ICP value */
	uint8_t Ibleed;                         /*!< Loopfilter I Bleed value */
	uint8_t oldIcp;                         /*!< Old Loopfilter ICP value */
	uint32_t effectiveloopBW;               /*!< Loopfilter Effective bw from calculated R/Cs */
} LoopFilterResult_t;

typedef struct {
	float loopBW;                                   /*!< PLL's Loopfilter bandwidth */
	float phaseMargin;                              /*!< PLL's Loopfilter phase margin */
	uint8_t voutLvl;                                /*!< PLL's Loopfilter output level */
} LoopFilterParam_t;


/* Enumeration for PLLs */
typedef enum {
	PLL_CLKGEN_PLL,         /*!< Ref for the Clock Gen PLL */
	PLL_SEC_CLKGEN_PLL,     /*!< Ref for the Secondary Clock Gen PLL */
	PLL_LAST_PLL            /*!< Ref for last COMMON PLL  */
}
PllSelName_e;
typedef enum {
	CLK_7G,         /*!< Clk Vco running at 7G */
	CLK_11G,        /*!< Clk Vco running at 11G */
	NUM_CLK_SPD
} clkFreq_e;


/* VCO coef data structure def'n */
typedef struct {
	uint16_t f1Mhz;
	uint16_t f2Mhz;
	uint8_t vcoVarTc;
	uint8_t vcoVaractor;
	uint8_t vcoPkDet;
	uint8_t vcoPtatCtrl;
} VcoCoef_t;

typedef struct {
	uint64_t vcoFreqHz;
	uint32_t refClock;
	uint8_t ICP;
	float cpCalClkDiv;
} CpCalcs_t;


/* This structure contains the state of the PLL settings */
typedef struct {
	uint64_t PllFrequency;
	uint32_t PllFrequency_kHz;
	uint64_t vcoFreqHz;
	uint64_t vcoScaledFreqHz;
	clkFreq_e clkFreqIndex;
	uint8_t refClkDiv;
	uint8_t k1ClkDiv;
	uint32_t refClock;
	uint8_t rootDiv;
	uint8_t rootDivHw;
	uint8_t vcoClkDiv;
	uint8_t fractionalByte0;
	uint8_t fractionalByte1;
	uint8_t fractionalByte2;
	uint8_t fractionalByte3;
	uint8_t integerByte0;
	uint8_t integerByte1;
	uint8_t vcoVaractor;
	uint8_t vcoVarTc;
	uint8_t vcoPtatCtrl;
	uint8_t vOut;
	uint8_t vcoPkDet;
	uint8_t vcoVrefSel;
	uint8_t cmSel;
	uint8_t bypasDet;
	uint8_t iBleedEnb;
	uint32_t cpCalEnb;
	bool doGcntMcs;
	float loopBW;
	float phaseMargin;
	float kVco;
	float vcoAmp;
	float vcoAmpOverrideValue;
	uint32_t vcoAmpOverride;
	float pfdOverrideValue;
	uint32_t refClkOverride;
	uint32_t refClkOverrideValue;
	uint32_t refClkSave;
	uint32_t pfdOverride;
	uint32_t vcoBandSelFreq_kHz;
	PllVcoBand_e vcoBand;
	LoopFilterResult_t LoopFilter;
} PllSynthParam_t;

#endif /* PLL_H */
