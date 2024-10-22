/*
 * Copyright(c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <drivers/adi/adrv906x/ddr/ddr_phy.h>
#include <lib/mmio.h>

#include "adrv906x_device_profile.h"
#include "../../ddr_config.h"
#include "../../ddr_regmap.h"
#include "../../ddr_phy_helpers.h"

/**
 *******************************************************************************
 * Function: ddr_phy_init
 *
 * @brief      Initializes the DDR PHY
 *
 * @details    Initializes the DDR PHY
 *
 * @return	Status of DDR phy init sequence
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************
 */

/* Start of PhyInit */
ddr_error_t ddr_2gb_1rank_x16_1gbx16_3200_phy_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_init_stages_t stage, ddr_config_t configuration)
{
	ddr_error_t result = ERROR_DDR_NO_ERROR;
	ddr_pstate_data_t pstate = { DDR_PSTATE0, 1600 };
	int train_2d = 0;


/*##############################################################
*
* PhyOverrideUserInput is a user-editable function.
*
* See PhyInit App Note for detailed description and function usage
*
##############################################################*/

	result = phy_override_user_input();

/* Enable power/clocks to the DDR Phy */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_enable_power_and_clocks(base_addr_adi_interface, base_addr_clk, pstate.freq);




/*##############################################################
*
* Step (C) Initialize PHY Configuration
*
* Load the required PHY configuration registers for the appropriate mode and memory configuration
*
##############################################################*/



/*##############################################################
 * TxPreDrvMode[2] = 0
 ############################################################## */
/* Pstate=0, Memclk=1600MHz, Programming TxSlewRate::TxPreDrvMode to 0x2*/
/* Pstate=0, Memclk=1600MHz, Programming TxSlewRate::TxPreP to 0xf*/
/* Pstate=0, Memclk=1600MHz, Programming TxSlewRate::TxPreN to 0xf*/
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXSLEWRATE_B0_P0 + base_addr_phy), 0x2ff);
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXSLEWRATE_B1_P0 + base_addr_phy), 0x2ff);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXSLEWRATE_B0_P0 + base_addr_phy), 0x2ff);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXSLEWRATE_B1_P0 + base_addr_phy), 0x2ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=0*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=0*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=0*/
	mmio_write_32((DDRPHYA_ANIB0_P0_ANIB0_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=1*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=1*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=1*/
	mmio_write_32((DDRPHYA_ANIB1_P0_ANIB1_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=2*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=2*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=2*/
	mmio_write_32((DDRPHYA_ANIB2_P0_ANIB2_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=3*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=3*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=3*/
	mmio_write_32((DDRPHYA_ANIB3_P0_ANIB3_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x0, ANIB=4*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=4*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=4*/
	mmio_write_32((DDRPHYA_ANIB4_P0_ANIB4_P0_ATXSLEWRATE + base_addr_phy), 0xff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x0, ANIB=5*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=5*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=5*/
	mmio_write_32((DDRPHYA_ANIB5_P0_ANIB5_P0_ATXSLEWRATE + base_addr_phy), 0xff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=6*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=6*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=6*/
	mmio_write_32((DDRPHYA_ANIB6_P0_ANIB6_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=7*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=7*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=7*/
	mmio_write_32((DDRPHYA_ANIB7_P0_ANIB7_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=8*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=8*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=8*/
	mmio_write_32((DDRPHYA_ANIB8_P0_ANIB8_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=9*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=9*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=9*/
	mmio_write_32((DDRPHYA_ANIB9_P0_ANIB9_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=10*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=10*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=10*/
	mmio_write_32((DDRPHYA_ANIB10_P0_ANIB10_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Programming ATxSlewRate::ATxPreDrvMode to 0x3, ANIB=11*/
/* Programming ATxSlewRate::ATxPreP to 0xf, ANIB=11*/
/* Programming ATxSlewRate::ATxPreN to 0xf, ANIB=11*/
	mmio_write_32((DDRPHYA_ANIB11_P0_ANIB11_P0_ATXSLEWRATE + base_addr_phy), 0x3ff);
/* Pstate=0,  Memclk=1600MHz, Programming PllCtrl2 to 19 based on DfiClk frequency = 800.*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_PLLCTRL2_P0 + base_addr_phy), 0x19);

/*##############################################################
*
* Program ARdPtrInitVal based on Frequency and PLL Bypass inputs
* The values programmed here assume ideal properties of DfiClk
* and Pclk including:
* - DfiClk skew
* - DfiClk jitter
* - DfiClk PVT variations
* - Pclk skew
* - Pclk jitter
*
* PLL Bypassed mode:
*     For MemClk frequency > 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 2-5
*     For MemClk frequency < 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 1-5
*
* PLL Enabled mode:
*     For MemClk frequency > 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 1-5
*     For MemClk frequency < 933MHz, the valid range of ARdPtrInitVal_p0[3:0] is: 0-5
*
##############################################################*/
/* Pstate=0, Memclk=1600MHz, Programming ARdPtrInitVal to 0x2*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_ARDPTRINITVAL_P0 + base_addr_phy), 0x2);
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::TwoTckRxDqsPre to 0x0*/
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::TwoTckTxDqsPre to 0x0*/
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::PositionDfeInit to 0x2*/
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::LP4TglTwoTckTxDqsPre to 0x0*/
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::LP4PostambleExt to 0x0*/
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl::LP4SttcPreBridgeRxEn to 0x0*/
/* Pstate=0, Memclk=1600MHz, Programming DqsPreambleControl to 0x8*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DQSPREAMBLECONTROL_P0 + base_addr_phy), 0x8);
/* Pstate=0, Memclk=1600MHz, Programming DbyteDllModeCntrl to 0x2*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DBYTEDLLMODECNTRL + base_addr_phy), 0x2);
/* Pstate=0, Memclk=1600MHz, Programming DllLockParam to 0x212*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DLLLOCKPARAM_P0 + base_addr_phy), 0x212);
/* Pstate=0, Memclk=1600MHz, Programming DllGainCtl to 0x61*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DLLGAINCTL_P0 + base_addr_phy), 0x61);
/* Pstate=0, Memclk=1600MHz, Programming ProcOdtTimeCtl to 0x7*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_PROCODTTIMECTL_P0 + base_addr_phy), 0x7);
/* Pstate=0, Memclk=1600MHz, Programming TxOdtDrvStren::ODTStrenP to 0x18*/
/* Pstate=0, Memclk=1600MHz, Programming TxOdtDrvStren::ODTStrenN to 0x0*/
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B1_P0 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXODTDRVSTREN_B1_P0 + base_addr_phy), 0x18);
/* Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqP to 0x18*/
/* Pstate=0, Memclk=1600MHz, Programming TxImpedanceCtrl1::DrvStrenFSDqN to 0x18*/
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXIMPEDANCECTRL1_B0_P0 + base_addr_phy), 0x659);
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXIMPEDANCECTRL1_B1_P0 + base_addr_phy), 0x659);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXIMPEDANCECTRL1_B0_P0 + base_addr_phy), 0x659);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXIMPEDANCECTRL1_B1_P0 + base_addr_phy), 0x659);
/* Programming ATxImpedance::ADrvStrenP to 0x1f*/
/* Programming ATxImpedance::ADrvStrenN to 0x1f*/
	mmio_write_32((DDRPHYA_ANIB0_P0_ANIB0_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB1_P0_ANIB1_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB2_P0_ANIB2_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB3_P0_ANIB3_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB4_P0_ANIB4_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB5_P0_ANIB5_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB6_P0_ANIB6_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB7_P0_ANIB7_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB8_P0_ANIB8_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB9_P0_ANIB9_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB10_P0_ANIB10_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
	mmio_write_32((DDRPHYA_ANIB11_P0_ANIB11_P0_ATXIMPEDANCE + base_addr_phy), 0x3ff);
/* Programming DfiMode to 0x1*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIMODE + base_addr_phy), 0x1);
/* Programming DfiCAMode to 0x2*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFICAMODE + base_addr_phy), 0x2);
/* Programming CalDrvStr0::CalDrvStrPd50 to 0x0*/
/* Programming CalDrvStr0::CalDrvStrPu50 to 0x0*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_CALDRVSTR0 + base_addr_phy), 0x0);
/* Pstate=0, Memclk=1600MHz, Programming CalUclkInfo::CalUClkTicksPer1uS to 0x320*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_CALUCLKINFO_P0 + base_addr_phy), 0x320);
/* Programming CalRate::CalInterval to 0x9*/
/* Programming CalRate::CalOnce to 0x0*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_CALRATE + base_addr_phy), 0x9);
/* Pstate=0, Programming VrefInGlobal::GlobalVrefInSel to 0x0*/
/* Pstate=0, Programming VrefInGlobal::GlobalVrefInDAC to 0x41*/
/* Pstate=0, Programming VrefInGlobal to 0x208*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_VREFINGLOBAL_P0 + base_addr_phy), 0x208);
/* Pstate=0, Programming DqDqsRcvCntrl::MajorModeDbyte to 0x3*/
/* Pstate=0, Programming DqDqsRcvCntrl to 0x5b1*/
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_DQDQSRCVCNTRL_B0_P0 + base_addr_phy), 0x5b1);
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_DQDQSRCVCNTRL_B1_P0 + base_addr_phy), 0x5b1);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_DQDQSRCVCNTRL_B0_P0 + base_addr_phy), 0x5b1);
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_DQDQSRCVCNTRL_B1_P0 + base_addr_phy), 0x5b1);
/* Pstate=0, Memclk=1600MHz, Programming DfiFreqRatio_p0 to 0x1*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQRATIO_P0 + base_addr_phy), 0x1);
/* Pstate=0, Memclk=1600MHz, Programming TristateModeCA::DisDynAdrTri_p0 to 0x1*/
/* Pstate=0, Memclk=1600MHz, Programming TristateModeCA::DDR2TMode_p0 to 0x0*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_TRISTATEMODECA_P0 + base_addr_phy), 0x5);
/* Programming DfiFreqXlat* */
	/* Protium and Palladium models use pll workaround clock source, configure to use pll workaround clock frequency */
	if (plat_is_protium() || plat_is_palladium())
		mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT0 + base_addr_phy), 0x6666);
	else
		mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT0 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT1 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT2 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT3 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT4 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT5 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT6 + base_addr_phy), 0x5555);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQXLAT7 + base_addr_phy), 0xf000);
/* Disabling Lane 8 Receiver to save power.0*/
	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_DQDQSRCVCNTRL1 + base_addr_phy), 0x500);
/* Disabling Lane 8 Receiver to save power.1*/
	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_DQDQSRCVCNTRL1 + base_addr_phy), 0x500);
/* Programming MasterX4Config::X4TG to 0x0*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_MASTERX4CONFIG + base_addr_phy), 0x0);
/* Pstate=0, Memclk=1600MHz, Programming DMIPinPresent::RdDbiEnabled to 0x0*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_DMIPINPRESENT_P0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_ACX4ANIBDIS + base_addr_phy), 0x0);

/* Run pre-training command; */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_run_pre_training(base_addr_ctrl, base_addr_phy, pstate.freq);

// 1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.
//       This allows the memory controller unrestricted access to the configuration CSRs.
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);

	if (stage == DDR_CUSTOM_TRAINING) {
		result = phy_set_dfi_clock(base_addr_ctrl, base_addr_phy, base_addr_clk, pstate);
		return result;
	}

/* Load the Imem */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_load_imem(train_2d, DDR_PSTATE0, base_addr_phy);

/* Set DFI clock to desired runtime frequency */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_set_dfi_clock(base_addr_ctrl, base_addr_phy, base_addr_clk, pstate);

// 1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.
//       This allows the memory controller unrestricted access to the configuration CSRs.
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);

/* Load the Dmem */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_load_dmem(train_2d, DDR_PSTATE0, base_addr_phy, configuration);

/* Enable uCtrl and wait for DONE message */
	if (result == ERROR_DDR_NO_ERROR) {
		phy_enable_micro_ctrl(base_addr_phy);
		result = phy_wait_for_done(base_addr_phy, train_2d);
	}

/*Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.*/
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);

/* Read the message block */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_read_msg_block(base_addr_phy, train_2d, 1, DDR_PSTATE0);

/*Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x1);

	train_2d = 1;

/* Set DFI clock to desired runtime frequency */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_set_dfi_clock(base_addr_ctrl, base_addr_phy, base_addr_clk, pstate);

// 1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.
//       This allows the memory controller unrestricted access to the configuration CSRs.
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);

/* Load the Imem */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_load_imem(train_2d, DDR_PSTATE0, base_addr_phy);

// 1.	Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.
//       This allows the memory controller unrestricted access to the configuration CSRs.
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);

/* Load the Dmem */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_load_dmem(train_2d, DDR_PSTATE0, base_addr_phy, configuration);

/* Enable uCtrl and wait for DONE message */
	if (result == ERROR_DDR_NO_ERROR) {
		phy_enable_micro_ctrl(base_addr_phy);
		result = phy_wait_for_done(base_addr_phy, train_2d);
	}

/*Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0.*/
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);

/* Read the message block */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_read_msg_block(base_addr_phy, train_2d, 1, DDR_PSTATE0);

/*Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x1);
/* Start of dwc_ddrphy_phyinit_I_loadPIEImage()*/


/*##############################################################
*
* (I) Load PHY Init Engine Image
*
* Load the PHY Initialization Engine memory with the provided initialization sequence.
* See PhyInit App Note for detailed description and function usage
*
*
##############################################################*/


/* Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0. */
/* This allows the memory controller unrestricted access to the configuration CSRs. */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x0);
/* Programming PIE Production Code*/
// [dwc_ddrphy_phyinit_LoadPieProdCode] Load PIE Production code: userInputBasic.DramDataWidth=16, userInputAdvanced.EnableHighClkSkewFix=1
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_PRESEQUENCEREG0B0S0 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_PRESEQUENCEREG0B0S1 + base_addr_phy), 0x400);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_PRESEQUENCEREG0B0S2 + base_addr_phy), 0x10e);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_PRESEQUENCEREG0B1S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_PRESEQUENCEREG0B1S1 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_PRESEQUENCEREG0B1S2 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B0S0 + base_addr_phy), 0xb);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B0S1 + base_addr_phy), 0x480);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B0S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B1S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B1S1 + base_addr_phy), 0x448);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B1S2 + base_addr_phy), 0x139);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B2S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B2S1 + base_addr_phy), 0x478);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B2S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B3S0 + base_addr_phy), 0x2);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B3S1 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B3S2 + base_addr_phy), 0x139);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B4S0 + base_addr_phy), 0xb);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B4S1 + base_addr_phy), 0x7c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B4S2 + base_addr_phy), 0x139);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B5S0 + base_addr_phy), 0x44);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B5S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B5S2 + base_addr_phy), 0x159);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B6S0 + base_addr_phy), 0x14f);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B6S1 + base_addr_phy), 0x630);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B6S2 + base_addr_phy), 0x159);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B7S0 + base_addr_phy), 0x47);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B7S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B7S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B8S0 + base_addr_phy), 0x4f);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B8S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B8S2 + base_addr_phy), 0x179);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B9S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B9S1 + base_addr_phy), 0xe0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B9S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B10S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B10S1 + base_addr_phy), 0x7c8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B10S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B11S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B11S1 + base_addr_phy), 0x1);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B11S2 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B12S0 + base_addr_phy), 0x30);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B12S1 + base_addr_phy), 0x65a);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B12S2 + base_addr_phy), 0x9);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B13S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B13S1 + base_addr_phy), 0x45a);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B13S2 + base_addr_phy), 0x9);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B14S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B14S1 + base_addr_phy), 0x448);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B14S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B15S0 + base_addr_phy), 0x40);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B15S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B15S2 + base_addr_phy), 0x179);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B16S0 + base_addr_phy), 0x1);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B16S1 + base_addr_phy), 0x618);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B16S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B17S0 + base_addr_phy), 0x40c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B17S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B17S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B18S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B18S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B18S2 + base_addr_phy), 0x48);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B19S0 + base_addr_phy), 0x4040);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B19S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B19S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B20S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B20S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B20S2 + base_addr_phy), 0x48);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B21S0 + base_addr_phy), 0x40);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B21S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B21S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B22S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B22S1 + base_addr_phy), 0x658);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B22S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B23S0 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B23S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B23S2 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B24S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B24S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B24S2 + base_addr_phy), 0x78);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B25S0 + base_addr_phy), 0x549);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B25S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B25S2 + base_addr_phy), 0x159);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B26S0 + base_addr_phy), 0xd49);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B26S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B26S2 + base_addr_phy), 0x159);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B27S0 + base_addr_phy), 0x94a);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B27S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B27S2 + base_addr_phy), 0x159);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B28S0 + base_addr_phy), 0x441);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B28S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B28S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B29S0 + base_addr_phy), 0x42);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B29S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B29S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B30S0 + base_addr_phy), 0x1);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B30S1 + base_addr_phy), 0x633);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B30S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B31S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B31S1 + base_addr_phy), 0xe0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B31S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B32S0 + base_addr_phy), 0xa);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B32S1 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B32S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B33S0 + base_addr_phy), 0x9);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B33S1 + base_addr_phy), 0x3c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B33S2 + base_addr_phy), 0x149);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B34S0 + base_addr_phy), 0x9);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B34S1 + base_addr_phy), 0x3c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B34S2 + base_addr_phy), 0x159);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B35S0 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B35S1 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B35S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B36S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B36S1 + base_addr_phy), 0x3c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B36S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B37S0 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B37S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B37S2 + base_addr_phy), 0x48);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B38S0 + base_addr_phy), 0x18);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B38S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B38S2 + base_addr_phy), 0x58);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B39S0 + base_addr_phy), 0xb);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B39S1 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B39S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B40S0 + base_addr_phy), 0x1);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B40S1 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B40S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B41S0 + base_addr_phy), 0x5);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B41S1 + base_addr_phy), 0x7c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B41S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B42S0 + base_addr_phy), 0xe);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B42S1 + base_addr_phy), 0x7c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B42S2 + base_addr_phy), 0x189);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B43S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B43S1 + base_addr_phy), 0x8140);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B43S2 + base_addr_phy), 0x10c);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B44S0 + base_addr_phy), 0x10);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B44S1 + base_addr_phy), 0x8138);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B44S2 + base_addr_phy), 0x104);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B45S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B45S1 + base_addr_phy), 0x448);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B45S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B46S0 + base_addr_phy), 0xf);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B46S1 + base_addr_phy), 0x7c0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B46S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B47S0 + base_addr_phy), 0x47);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B47S1 + base_addr_phy), 0x630);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B47S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B48S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B48S1 + base_addr_phy), 0x618);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B48S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B49S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B49S1 + base_addr_phy), 0xe0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B49S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B50S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B50S1 + base_addr_phy), 0x7c8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B50S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B51S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B51S1 + base_addr_phy), 0x8140);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B51S2 + base_addr_phy), 0x10c);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B52S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B52S1 + base_addr_phy), 0x478);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B52S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B53S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B53S1 + base_addr_phy), 0x1);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B53S2 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B54S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B54S1 + base_addr_phy), 0x4);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQUENCEREG0B54S2 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_POSTSEQUENCEREG0B0S0 + base_addr_phy), 0x8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_POSTSEQUENCEREG0B0S1 + base_addr_phy), 0x7c8);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_POSTSEQUENCEREG0B0S2 + base_addr_phy), 0x109);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_POSTSEQUENCEREG0B1S0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_POSTSEQUENCEREG0B1S1 + base_addr_phy), 0x400);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_POSTSEQUENCEREG0B1S2 + base_addr_phy), 0x106);
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_SEQUENCEROVERRIDE + base_addr_phy), 0x400);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_STARTVECTOR0B0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_STARTVECTOR0B15 + base_addr_phy), 0x2d);
/* Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY0 to 0x64*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_SEQ0BDLY0_P0 + base_addr_phy), 0x64);
/* Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY1 to 0xc8*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_SEQ0BDLY1_P0 + base_addr_phy), 0xc8);
/* Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY2 to 0x7d0*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_SEQ0BDLY2_P0 + base_addr_phy), 0x7d0);
/* Pstate=0,  Memclk=1600MHz, Programming Seq0BDLY3 to 0x2c*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_SEQ0BDLY3_P0 + base_addr_phy), 0x2c);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG0 + base_addr_phy), 0x0);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG1 + base_addr_phy), 0x173);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG2 + base_addr_phy), 0x60);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG3 + base_addr_phy), 0x6110);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG4 + base_addr_phy), 0x2152);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG5 + base_addr_phy), 0xdfbd);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG6 + base_addr_phy), 0xffff);
	mmio_write_32((DDRPHYA_INITENG0_P0_INITENG0_P0_SEQ0BDISABLEFLAG7 + base_addr_phy), 0x6152);
/* Turn on calibration and hold idle until dfi_init_start is asserted sequence is triggered.*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_CALZAP + base_addr_phy), 0x1);
/* Programming CalRate::CalInterval to 0x9*/
/* Programming CalRate::CalOnce to 0x0*/
/* Programming CalRate::CalRun to 0x1*/
	mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_CALRATE + base_addr_phy), 0x19);
/* Disabling Ucclk (PMU) and Hclk (training hardware)*/
	mmio_write_32((DDRPHYA_DRTUB0_DRTUB0_UCCLKHCLKENABLES + base_addr_phy), 0x0);
/* Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1. */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x1);
/* End of dwc_ddrphy_phyinit_I_loadPIEImage()*/

/* Run post-training command; */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_run_post_training();


/* Enter Mission Mode */
	if (result == ERROR_DDR_NO_ERROR)
		result = phy_enter_mission_mode(base_addr_ctrl, base_addr_phy, base_addr_clk, 1, pstate, pstate);


/* End of PhyInit */
	return result;
}
