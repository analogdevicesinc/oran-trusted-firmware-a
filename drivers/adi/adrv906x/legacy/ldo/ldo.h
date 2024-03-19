/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LDO_H
#define LDO_H

#include "../utils/utils.h"
#include "../hardware_if/hardware_if.h"
#include "../adi_errors/adi_errors.h"
#include "../regmap/pll_regmap.h"


#define     LDO_DEFAULT_PWR_UP_DEL                (2000u)       /* Default Power-up delay: 2 mSecs */
#define     LDO_DEFAULT_ENB_OUT_DEL               (8000u)       /* Default EnableLDO output delay: 8 mSecs */
#define     LDO_DEFAULT_PWR_UP_SETTLE             (9000u)       /* Default Power-up delay: 9 mSecs */
#define     LDO_PWRGOOD_MAX_WAIT_TIME             (5000u)       /* Maximum wait time for powergood signal to be set: 5 ms */
#define     LDO_NOMINAL_VOL_SEL                   (0x8u)        /* Nominal LDO DC Output Voltage Selection: 1.000V */
#define     VCOLCR_WAIT_TIME                      (15u)         /* Time for VCO LCR to power up. 12.5us in user guide. We wait 15us. */
#define     SHUNT_LDO_MAX_WAIT_TIME               (5u)          /* Maximum wait time for shunt LDO powerup: 5us */

typedef enum {
	LDO_VCO_SHUNT,  /*!< VCO shunt LDO  */
	LDO_SDM_SHUNT,  /*!< SDM shunt LDO  */
	LDO_ALL_SHUNT,  /*!< All shunt LDOs */
} Ldo_ShuntLdoMask_e;




#endif /* LDO_H */
