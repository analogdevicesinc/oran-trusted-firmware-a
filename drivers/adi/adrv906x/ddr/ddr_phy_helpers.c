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
#include <adrv906x_device_profile.h>

#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/ddr/ddr_phy.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "ddr_messages.h"
#include "ddr_phy_helpers.h"
#include "ddr_regmap.h"

/* Debug */
/*#define DDR_DEBUG_ENABLE*/

#ifdef DDR_DEBUG_ENABLE
#define HDTCTRL_ADDR 0x150030
#define DDR_DEBUG(...)          INFO(__VA_ARGS__)
#else
#define DDR_DEBUG(...)
#endif

extern unsigned char adi_dmem0_1d[];
extern unsigned char adi_dmem0_2d[];
extern unsigned char adi_sec_dmem0_1d[];
extern unsigned char adi_sec_dmem0_2d[];
extern unsigned char adi_imem_1d[];
extern unsigned char adi_imem_2d[];
extern unsigned char adi_dmem0_1d_end[];
extern unsigned char adi_dmem0_2d_end[];
extern unsigned char adi_sec_dmem0_1d_end[];
extern unsigned char adi_sec_dmem0_2d_end[];
extern unsigned char adi_imem_1d_end[];
extern unsigned char adi_imem_2d_end[];

static bool ddr_custom_parameters_enable = false;
static ddr_custom_values_t ddr_custom_parameters = {
	.ard_ptr_init_val		= 0,
	.data_tx_impedance_ctrl		= 0,
	.data_tx_odt_drive_strength	= 0,
	.master_cal_rate		= 0
};

typedef struct {
	uint8_t wr2rd;
	uint8_t wr2rd_s;
	uint8_t wr2rd_dr;
	uint8_t rd2wr;
	uint8_t diff_rank_rd_gap;
	uint8_t diff_rank_wr_gap;
	uint8_t wrdata_delay;
} umctl2_timing_registers_t;

typedef enum {
	UMCTL2_RR,
	UMCTL2_RW,
	UMCTL2_WW,
	UMCTL2_WR
} ucmtl2_timing_types_t;

static umctl2_timing_registers_t umctl2TimingBaseValues;
static umctl2_timing_registers_t pstateTimings[4];

static void get_base_umctl2_timing_values(uintptr_t base_addr_ctrl, uint64_t freq);
static uint8_t get_cdd_value(uintptr_t base_addr_phy, ucmtl2_timing_types_t timing_type, uint8_t ranks);
static void get_cdd_array(uintptr_t base_addr_phy, ucmtl2_timing_types_t timing_type, int8_t *array);
static uint8_t get_wrdata_delay(uintptr_t base_addr_phy, ddr_pstate_t pstate, uint8_t ranks);
static int phy_get_mem_info(bool select_imem, ddr_pstate_t pstate, int train_2d, uint16_t **mem_ptr, unsigned int *mem_length, ddr_config_t configuration);
static int phy_get_streaming_message(uintptr_t base_addr_phy, int train_2d);
static void phy_print_streaming_message(const char *message, ...);

/* Check if custom parameters have already been entered
 * Can be used to avoid overwriting existing custom parameters */
bool ddr_check_for_custom_parameters()
{
	return ddr_custom_parameters_enable;
}

/* Set the value of custom parameters and enable them */
void ddr_set_custom_parameters(ddr_custom_values_t *parameters)
{
	if (parameters != NULL) {
		/* Copy argument values into the static struct. Ensure that this function is making a deep copy
		 * if any dynamically allocated members are added to the struct definition */
		ddr_custom_parameters.ard_ptr_init_val = parameters->ard_ptr_init_val;
		ddr_custom_parameters.data_tx_impedance_ctrl = parameters->data_tx_impedance_ctrl;
		ddr_custom_parameters.data_tx_odt_drive_strength = parameters->data_tx_odt_drive_strength;
		ddr_custom_parameters.master_cal_rate = parameters->master_cal_rate;
		ddr_custom_parameters_enable = true;
	}
}

/* Weak defined function expected to be defined in customer board specific code */
#pragma weak ddr_board_custom_pre_training
void ddr_board_custom_pre_training(uintptr_t base_addr_phy)
{
	INFO("Using default values for DDR PHY init.\n");
}

static void ddr_program_custom_parameters(uintptr_t base_addr_phy)
{
	if (ddr_custom_parameters_enable) {
		INFO("Updating DDR PHY configuration values with custom numbers.\n");
		INFO("Setting TXODTDRVSTREN to %x.\n", ddr_custom_parameters.data_tx_odt_drive_strength);
		mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy), ddr_custom_parameters.data_tx_odt_drive_strength);
		mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B1_P0 + base_addr_phy), ddr_custom_parameters.data_tx_odt_drive_strength);
		mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy), ddr_custom_parameters.data_tx_odt_drive_strength);
		mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXODTDRVSTREN_B1_P0 + base_addr_phy), ddr_custom_parameters.data_tx_odt_drive_strength);
		INFO("TXODTDRVSTREN now is set to %x.\n", mmio_read_32(DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy));

		INFO("Setting TXIMPEDANCECTRL to %x.\n", ddr_custom_parameters.data_tx_impedance_ctrl);
		mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXIMPEDANCECTRL1_B0_P0 + base_addr_phy), ddr_custom_parameters.data_tx_impedance_ctrl);
		mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXIMPEDANCECTRL1_B1_P0 + base_addr_phy), ddr_custom_parameters.data_tx_impedance_ctrl);
		mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXIMPEDANCECTRL1_B0_P0 + base_addr_phy), ddr_custom_parameters.data_tx_impedance_ctrl);
		mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXIMPEDANCECTRL1_B1_P0 + base_addr_phy), ddr_custom_parameters.data_tx_impedance_ctrl);
		INFO("TXIMPEDANCECTRL now is set to %x.\n", mmio_read_32(DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXIMPEDANCECTRL1_B0_P0 + base_addr_phy));

		INFO("Setting MASTER0_CALRATE to %x.\n", ddr_custom_parameters.master_cal_rate);
		mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_CALRATE + base_addr_phy), ddr_custom_parameters.master_cal_rate);
		INFO("MASTER0_CALRATE now is set to %x.\n", mmio_read_32(DDRPHYA_MASTER0_P0_MASTER0_P0_CALRATE + base_addr_phy));

		INFO("Setting ARDPTRINITVAL to %x.\n", ddr_custom_parameters.ard_ptr_init_val);
		mmio_write_32((DDRPHYA_MASTER0_P0_MASTER0_P0_ARDPTRINITVAL_P0 + base_addr_phy), ddr_custom_parameters.ard_ptr_init_val);
		INFO("ARDPTRINITVAL now is set to %x.\n", ddr_custom_parameters.ard_ptr_init_val);
	}
}

/**
 *******************************************************************************
 * Function: phy_override_user_input
 *
 * @brief      Overrides certain user inputs before Phy init sequence
 *
 * @details    This function can be modified to overwrite any of the default user inputs
 * 		if needed
 *
 * Parameters:
 * @param [in]    None
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_override_user_input(void)
{
	return 0;
}

/**
 *******************************************************************************
 * Function: phy_enable_power_and_clocks
 *
 * @brief      Enable/check power/clocks to the PHY before init sequence
 *
 * @details    Enable/check power/clocks to the PHY before init sequence
 *
 * Parameters:
 * @param [in]    base_addr_adi_interface - Base addr for the DDR perf regmap
 * @param [in]    base_addr_clk - Base addr for the clk
 * @param [in]    freq - Default frequency for clock to run at when first enabled
 *
 * @return 	Status indicating if PHY was able to initialize its clock
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_enable_power_and_clocks(uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, uint64_t freq)
{
	int i;
	int return_val = ERROR_DDR_NO_ERROR;
	uint64_t clk_freq;

	/* Set clk freq */
	clk_disable_clock(base_addr_clk, CLK_ID_DDR);
	clk_set_freq(base_addr_clk, CLK_ID_DDR, (freq * DDR_MHZ_TO_HZ));
	clk_freq = clk_get_freq(base_addr_clk, CLK_ID_DDR);
	if ((clk_freq / DDR_MHZ_TO_HZ) != freq)
		return ERROR_DDR_PHY_INIT_FAILED;

	DDR_DEBUG("Programming DDR clock to %ld MHz.\n", freq);
	clk_enable_clock(base_addr_clk, CLK_ID_DDR);
	/* Check if DDR components are out of reset, and pull them out if not */
	if (mmio_read_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface) != 0x00000000) {
		mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x2f);
		/* Need to wait at minimum 128 cycles of core DDR clock after clearing APB reset to allow clocks to sync, Synopsys recommends 1us for standard */
		udelay(1);

		mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x28);
		mmio_write_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface, 0x00);
		for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
			if (mmio_read_32(DDR_FUNCTIONAL_CONTROLLER_DDR_RESET + base_addr_adi_interface) == 0x00000000)
				break;
			else
				mdelay(1);
		}

		if (i == ADI_DDR_CTRL_TIMEOUT)
			return_val = ERROR_DDR_PHY_INIT_FAILED;
		/* Synopsys documentation requires reset bit to remain low for at least 200 us before attempting the init sequence */
		udelay(200);
	}

	return return_val;
}

/**
 *******************************************************************************
 * Function: phy_run_pre_training
 *
 * @brief      Run training commands before training sequence begins
 *
 * @details    This function can override/write to custom PHY registers before training begins
 *
 * Parameters:
 * @param [in]    base_addr_ctrl - Base address of the DDR controller
 *
 * Reference to other related functions
 *
 * Notes: This function should not be used to override any registers that depend on values in userInput.Basic/Advanced or the message block. These should be done in PhyOverrideUserInput instead
 *
 *******************************************************************************/
ddr_error_t phy_run_pre_training(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uint64_t freq)
{
	DDR_DEBUG("Running DDR phy pre training tasks.\n");
	get_base_umctl2_timing_values(base_addr_ctrl, freq);
	/* Manual PLLCtrl overrides that must be done before training */
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_PLLTESTMODE_P0, 0x24);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_PLLCTRL1_P0, 0x21);
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_PLLCTRL4_P0, 0x17f);

	/* Run board specific pre training */
	ddr_board_custom_pre_training(base_addr_phy);
	/* Program custom parameters into PHY registers */
	ddr_program_custom_parameters(base_addr_phy);

	return 0;
};
/**
 *******************************************************************************
 * Function: phy_load_imem
 *
 * @brief      Loads the instruction memory for training sequence
 *
 * @details    Loads the instruction memory for training sequence
 *
 * Parameters:
 * @param [in]    train_2d - Chooses between 1D and 2D training. 0=1D, 1=2D
 * @param [in]	  pstate - Pstate selection for IMEM
 * @param [in]	  base_addr_phy - Base address of the DDR PHY
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_load_imem(int train_2d, ddr_pstate_t pstate, uintptr_t base_addr_phy)
{
	uintptr_t dest_ptr = (base_addr_phy + DDR_PHY_IP_ICCM_INDEX);
	uint16_t *mem_ptr = (uint16_t *)dest_ptr;
	unsigned int mem_length = 0;
	unsigned int i;

	DDR_DEBUG("Loading DDR IMEM for pstate %d.\n", pstate);
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_STALLTOMICRO_MASK);
	/* IMEM is the same for all configurations, so we assume primary. */
	phy_get_mem_info(1, pstate, train_2d, &mem_ptr, &mem_length, DDR_PRIMARY_CONFIGURATION);
	/* According to design team, for address between Synopsys space and our space to align, two bytes of the .bin are written every four addresses */
	for (i = 0; i < (mem_length >> 1); i++) {
		mmio_write_16(dest_ptr, *mem_ptr);
		dest_ptr += 4;
		mem_ptr++;
	}
	return 0;
};
/**
 *******************************************************************************
 * Function: phy_set_dfi_clock
 *
 * @brief      Sets DfiClock to desired frequency before training
 *
 * @details    Sets DfiClock to desired frequency before training
 *
 * Parameters:
 * @param [in]    base_addr_ctrl - Base address for DDR controller
 * @param [in]    base_addr_phy - Base address for DDR phy
 * @param [in]    base_addr_clk - Base address for the input clock
 * @param [in]    pstate_data - pstate data for desired frequency
 *
 * @return 	Status indicating if PHY successfully set DFI clock
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_set_dfi_clock(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_clk, ddr_pstate_data_t pstate_data)
{
	ddr_error_t return_val = ERROR_DDR_NO_ERROR;
	uint32_t dfimisc_ctrl;
	uint32_t dbgreg;
	uint32_t pwrctl;
	uint64_t clk_freq;
	int i;

	DDR_DEBUG("Setting DDR DFI clock to %d MHz.\n", pstate_data.freq);
	switch (pstate_data.pstate) {
	case DDR_PSTATE0:
		/* Set DfiFreqRatio, always 1:2 with current HW implementation */
		mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_DFIFREQRATIO_P0, DDR_DFIFREQRATIO);
		break;
	default:
		break;
	}
	;
	/* dfi_freq in DFIMISC is a quasi-dynamic register, so we must follow the protocol in Figure 6-4 of the handbook to program it */
	dbgreg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1);
	/* Stop any DFI transaction from being queued */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1, (dbgreg | DBG1_DIS_DQ_MASK));
	/* Make sure that the pipeline is empty before writing */
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		dbgreg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBGCAM);
		if ((dbgreg & (DBGCAM_WR_DATA_PIPELINE_EMPTY_MASK | DBGCAM_RD_DATA_PIPELINE_EMPTY_MASK)) == DDR_DFI_PIPELINE_EMPTY)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return_val = ERROR_DDR_PHY_INIT_FAILED;


	if (return_val == ERROR_DDR_NO_ERROR) {
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);
		/* Tell PHY what pstate frequency we are running at */
		dfimisc_ctrl = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC);
		dfimisc_ctrl &= ~DFIMISC_DFI_FREQUENCY_MASK;
		dfimisc_ctrl |= pstate_data.pstate << DFIMISC_DFI_FREQUENCY_SHIFT;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC, dfimisc_ctrl);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
		for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
			if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWSTAT) == 0x00000001)
				break;
			else
				mdelay(1);
		}

		dbgreg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1);
		dbgreg &= ~DBG1_DIS_DQ_MASK;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1, dbgreg); /* Allow queueing transactions again */
		if (i == ADI_DDR_CTRL_TIMEOUT)
			return ERROR_DDR_PHY_INIT_FAILED;

		pwrctl = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL);
		pwrctl |= PWRCTL_SELFREF_SW_MASK;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, pwrctl);

		clk_disable_clock(base_addr_clk, CLK_ID_DDR);

		/* Set clk freq */
		clk_set_freq(base_addr_clk, CLK_ID_DDR, (pstate_data.freq * DDR_MHZ_TO_HZ));
		clk_freq = clk_get_freq(base_addr_clk, CLK_ID_DDR);
		if ((clk_freq / DDR_MHZ_TO_HZ) != (uint64_t)pstate_data.freq)
			return_val = ERROR_DDR_PHY_INIT_FAILED;

		clk_enable_clock(base_addr_clk, CLK_ID_DDR);

		pwrctl = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL);
		pwrctl &= ~PWRCTL_SELFREF_SW_MASK;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, pwrctl);
	}
	return return_val;
};

/**
 *******************************************************************************
 * Function: phy_load_dmem
 *
 * @brief      Loads data memory to PHY before training
 *
 * @details    Loads data memory to PHY before training
 *
 * Parameters:
 * @param [in]    train_2d - Chooses between 1D and 2D training, 0=1D,1=2D
 * @param [in]	  pstate - Select which pstate to load the DMEM for
 * @param [in]	  base_addr_phy - Base address of the DDR PHY
 * @param [in]	  configuration - Selects the primary or secondary configuration of the DDR
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_load_dmem(int train_2d, ddr_pstate_t pstate, uintptr_t base_addr_phy, ddr_config_t configuration)
{
	uintptr_t dest_ptr = (base_addr_phy + DDR_PHY_IP_DCCM_INDEX);
	uint16_t *mem_ptr = (uint16_t *)dest_ptr;
	unsigned int mem_length = 0;
	unsigned int i;

	DDR_DEBUG("Loading DDR DMEM for pstate %d.\n", pstate);
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_STALLTOMICRO_MASK);
	phy_get_mem_info(0, pstate, train_2d, &mem_ptr, &mem_length, configuration);
	if (mem_ptr == NULL)
		return ERROR_DDR_PHY_INIT_FAILED;
	/* According to design team, for address between Synopsys space and our space to align, two bytes of the .bin are written every four addresses */
	for (i = 0; i < (mem_length >> 1); i++) {
		mmio_write_16(dest_ptr, *mem_ptr);
		mem_ptr++;
		dest_ptr += 4;
	}

#ifdef DDR_DEBUG_ENABLE
	/* Enable DDR PHY streaming messages */
	mmio_write_8(base_addr_phy + HDTCTRL_ADDR, 0x04);
#endif

	return 0;
};

/**
 *******************************************************************************
 * Function: phy_enable_micro_ctrl
 *
 * @brief      Enables microcontroller on the PHY to start training firmware
 *
 * @details    Pulls the microcontroller out of reset to start loaded training firmware
 *
 * Parameters:
 * @param [in]    base_addr_phy - Base address for DDR phy
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
void phy_enable_micro_ctrl(uintptr_t base_addr_phy)
{
	DDR_DEBUG("Starting the DDR PHY training.\n");
	/* Reset the firmware MCU by writing ResettoMicro and StalltoMicro to 1, then rewrite only StalltoMicro to 1 to pull out of reset but keep stalled */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICROCONTMUXSEL + base_addr_phy), 0x1);
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_RESETTOMICRO_MASK | DDR_STALLTOMICRO_MASK);
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_STALLTOMICRO_MASK);

	/* Begin execution of the training firmware by setting Microreset to 0 */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), 0x00000000);
};

/**
 *******************************************************************************
 * Function: phy_wait_for_done
 *
 * @brief      Blocks continuation of PHY init sequence until training firmware is done
 *
 * @details    Polls the PHY mailbox for firmware complete message, then allows init sequence to continue
 *
 * Parameters:
 * @param [in]    base_addr_phy - Base address for DDR Phy
 * @param [in]	  train_2d - Signals whether current training is for 1D or 2D
 *
 * @return 	Status indicating if PHY training firmware executed successfully
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_wait_for_done(uintptr_t base_addr_phy, int train_2d)
{
	int training_status = DDR_PHY_MAILBOX_TRAINING_RUNNING;
	uint64_t timeout;

	timeout = timeout_init_us(DDR_PHY_TRAINING_TIMEOUT_US);
	while (training_status != DDR_PHY_MAILBOX_TRAINING_DONE) {
		if (timeout_elapsed(timeout)) {
			ERROR("DDR training firmware failed to complete in time.");
			return ERROR_DDR_PHY_FW_FAILED;
		}
		training_status = phy_get_mailbox_message(base_addr_phy, train_2d);
		switch (training_status) {
		case DDR_PHY_MAILBOX_ACK_FAILED:
			mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_RESETTOMICRO_MASK | DDR_STALLTOMICRO_MASK);
			ERROR("DDR PHY Mailbox acknowledgement failed.\n");
			return ERROR_DDR_PHY_MAILBOX_FAILED;
		case DDR_PHY_MAILBOX_NO_STREAMING_MESSAGE:
			mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_RESETTOMICRO_MASK | DDR_STALLTOMICRO_MASK);
			ERROR("No streaming message received from DDR PHY mailbox after streaming message indicator triggered.\n");
			return ERROR_DDR_PHY_MAILBOX_FAILED;
		case DDR_PHY_MAILBOX_TOO_MANY_ARGUMENTS:
			mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_RESETTOMICRO_MASK | DDR_STALLTOMICRO_MASK);
			ERROR("Too many arguments to streaming message.\n");
			return ERROR_DDR_PHY_MAILBOX_FAILED;
		case DDR_PHY_MAILBOX_TRAINING_FAILED:
			ERROR("DDR training firmware returned an error message.\n");
			return ERROR_DDR_PHY_FW_FAILED;
		default:
			continue;
		}
	}
	/* If we make it here the training has passed, put the PHY MCU back into reset */
	mmio_write_32((DDRPHYA_APBONLY0_APBONLY0_MICRORESET + base_addr_phy), DDR_STALLTOMICRO_MASK);
	return ERROR_DDR_NO_ERROR;
};

/**
 *******************************************************************************
 * Function: phy_read_msg_block
 *
 * @brief      Reads results of training firmware out of the PHY
 *
 * @details    Reads results of training firmware out of the PHY
 *
 * Parameters:
 * @param [in]    base_addr_phy - Base address of the DDR PHY
 * @param [in]	  train_2d - Selects 1D or 2D results
 * @param [in]	  ranks - Number of ranks in the DDR
 * @param [in]	  pstate - Pstate to retrieve results for
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_read_msg_block(uintptr_t base_addr_phy, int train_2d, int ranks, ddr_pstate_t pstate)
{
	DDR_DEBUG("Reading PHY training results out of message block.\n");
	if (train_2d) {
		/* Fill out if we decide to use any of the 2D training results */
	} else {
		int8_t cddValue = 0;
		int8_t wrdata_delay;
		cddValue = get_cdd_value(base_addr_phy, UMCTL2_RW, ranks);
		/* Read to write timing within and across ranks */
		pstateTimings[pstate].rd2wr = umctl2TimingBaseValues.rd2wr + cddValue;

		cddValue = get_cdd_value(base_addr_phy, UMCTL2_WW, ranks);
		/* Write to read timing within and accross ranks */
		pstateTimings[pstate].wr2rd = umctl2TimingBaseValues.wr2rd + cddValue;
		pstateTimings[pstate].wr2rd_s = umctl2TimingBaseValues.wr2rd_s + cddValue;
		pstateTimings[pstate].wr2rd_dr = umctl2TimingBaseValues.wr2rd_dr + cddValue;

		/* Write to write timings across ranks, there is no delay for consecutive reads in the same rank */
		pstateTimings[pstate].diff_rank_wr_gap = umctl2TimingBaseValues.diff_rank_rd_gap + cddValue;

		cddValue = get_cdd_value(base_addr_phy, UMCTL2_RR, ranks);
		/* Read to read timings across ranks, there is no delay for consecutive reads in the same rank */
		pstateTimings[pstate].diff_rank_rd_gap = umctl2TimingBaseValues.diff_rank_rd_gap + cddValue;

		wrdata_delay = get_wrdata_delay(base_addr_phy, pstate, ranks);
		pstateTimings[pstate].wrdata_delay = umctl2TimingBaseValues.wrdata_delay + wrdata_delay;
	}
	return ERROR_DDR_NO_ERROR;
};

/**
 *******************************************************************************
 * Function: phy_run_post_training
 *
 * @brief      Runs user custom commands after Phy training is done
 *
 * @details    Function can be used to update/override PHY registers after training i.e. alter trained delays for testing purposes
 *
 * Parameters:
 * @param [in]    None
 *
 * Reference to other related functions
 *
 * Notes: Function should not be used to alter any registers that depend on values in userInput.Basic/Advanced
 *
 *******************************************************************************/
ddr_error_t phy_run_post_training(void)
{
	DDR_DEBUG("Running DDR post training tasks.\n");
	return 0;
}

/**
 *******************************************************************************
 * Function: phy_enter_mission_mode
 *
 * @brief      Switches the PHY from init/training mode to mission mode
 *
 * @details    Switches the PHY from init/training mode to mission mode
 *
 * Parameters:
 * @param [in]    base_addr_ctrl - Base address for DDR controller
 * @param [in]    base_addr_phy - Base address for DDR phy
 * @param [in]    base_addr_clk - Base address for the input clock
 * @param [in]    train_2d - Indicates whether 1D or 2D training was used
 * @param [in]    last_trained - Last pstate that was trained with firmware
 * @param [in]    default_pstate - Default pstate to use if 2D training was used
 *
 * @return 	Status indicating if PHY successfully entered mission mode
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
ddr_error_t phy_enter_mission_mode(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_clk, int train_2d, ddr_pstate_data_t last_trained, ddr_pstate_data_t default_pstate)
{
	ddr_error_t return_val = ERROR_DDR_NO_ERROR;

	/* The setting of DFI init bits is already implemented in the DDR ctrl code, so this function just ensures the clocks are set correctly for mission mode */
	DDR_DEBUG("Entering DDR normal mission mode.\n");
	/* If 2D training was used, we have to enter mission mode in pstate0 no matter what pstate we want to normally run at */
	if (train_2d) {
		/* Program the controller with the timing parameters of the selected pstate */
		if (return_val == ERROR_DDR_NO_ERROR)
			return_val = phy_set_dfi_clock(base_addr_ctrl, base_addr_phy, base_addr_clk, default_pstate);
	} else {
		/* If no 2D training used, enter mission mode in the last pstate trained */
		if (return_val == ERROR_DDR_NO_ERROR)
			return_val = phy_set_dfi_clock(base_addr_ctrl, base_addr_phy, base_addr_clk, last_trained);
	}

	/* Enable the RX Fifo Check interrupt for the PHY, while disabling all other PHY interrupt sources */
	mmio_write_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_PHYINTERRUPTENABLE, PHY_RX_FIFO_CHECK_ENABLE_MASK);
	return return_val;
};

/**
 *******************************************************************************
 * Function: phy_get_mailbox_message
 *
 * @brief      Retrieves message from DDR Phy mailbox
 *
 * @details    Checks if there is a message in the DDR Phy mailbox, and returns it
 *
 * Parameters:
 * @param [in]    base_addr_phy - Base address for the DDR Phy
 * @param [in]	  train_2d - Signals whether current training is for 1D or 2D
 *
 * @return 	Mailbox message value, or -1 if no message ready
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************/
int phy_get_mailbox_message(uintptr_t base_addr_phy, int train_2d)
{
	int i;
	int result = DDR_PHY_MAILBOX_MAILBOX_EMPTY;
	uint32_t mailbox_msg = DDR_PHY_TRAINING_RUNNING;

	/* SystemC/Protium/Palladium cannot run the actual training firmware, so return the TRAINING_DONE message so the rest of the init can be tested untrained */
	if (!plat_is_hardware())
		return DDR_PHY_MAILBOX_TRAINING_DONE;

	if ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) == 0x0) {
		/* Check if mailbox has a message ready */
		mailbox_msg = mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTWRITEONLYSHADOW);
		mmio_write_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_DCTWRITEPROT, 0x0);
		/* Wait for ACK from PHY */
		for (i = 0; i < DDR_PHY_MAILBOX_TIMEOUT_US; i++) {
			/* ACK the message and wait for DDR Phy ack back */
			if ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) == 0x1) {
				mmio_write_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_DCTWRITEPROT, 0x1);
				break;
			} else {
				udelay(1);
			}
		}

		/* Error with the ACK */
		if (i == DDR_PHY_MAILBOX_TIMEOUT_US)
			return DDR_PHY_MAILBOX_ACK_FAILED;
	} else {
		/* There was no message in the mailbox, so return early to let caller know. */
		return result;
	}

	if (mailbox_msg == DDR_PHY_TRAINING_DONE)
		result = DDR_PHY_MAILBOX_TRAINING_DONE;
	else if (mailbox_msg == DDR_PHY_STREAMING_MESSAGE)
		result = phy_get_streaming_message(base_addr_phy, train_2d);
	else if (mailbox_msg == DDR_PHY_TRAINING_FAILED)
		result = DDR_PHY_MAILBOX_TRAINING_FAILED;
	else
		result = DDR_PHY_MAILBOX_TRAINING_RUNNING;



	return result;
}

/* Gets a streaming message from the mailbox once the streaming message indicator major message is seen */
static int phy_get_streaming_message(uintptr_t base_addr_phy, int train_2d)
{
	uint32_t arguments[DDR_PHY_STREAMING_MESSAGE_MAX_ARGUMENTS];
	bool found_message = false;
	char *message = "No matching streaming message found for 0x%x\n";
	uint32_t mailbox_msg;
	int i, j;
	int arg_count;
	uint64_t timeout;

	timeout = timeout_init_us(DDR_PHY_MAILBOX_TIMEOUT_US);
	while ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) != 0x0) {
		if (timeout_elapsed(timeout))
			/* If the streaming message doesn't show up in the mailbox, error out */
			return DDR_PHY_MAILBOX_NO_STREAMING_MESSAGE;
	}

	/* Get the streaming message */
	mailbox_msg = mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTWRITEONLYSHADOW);
	mailbox_msg |= (mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTDATWRITEONLYSHADOW) << 16);
	mmio_write_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_DCTWRITEPROT, 0x0);
	/* Wait for ACK from PHY */
	for (j = 0; j < DDR_PHY_MAILBOX_TIMEOUT_US; j++) {
		/* ACK the message and wait for DDR Phy ack back */
		if ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) == 0x1) {
			mmio_write_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_DCTWRITEPROT, 0x1);
			break;
		} else {
			udelay(1);
		}
	}

	/* Error with the ACK */
	if (j == DDR_PHY_MAILBOX_TIMEOUT_US)
		return DDR_PHY_MAILBOX_ACK_FAILED;

	/* Decode the first part of the streaming message to figure out which format string to use */
	if (train_2d) {
		/* Figure out which message to print */
		for (i = 0; i < TWOD_TRAINING_MESSAGE_STRING_COUNT; i++) {
			if (mailbox_msg == ddr_2d_log_messages[i].id) {
				message = ddr_2d_log_messages[i].message;
				found_message = true;
				break;
			}
		}
	} else {
		/* Figure out which message to print */
		for (i = 0; i < ONED_TRAINING_MESSAGE_STRING_COUNT; i++) {
			if (mailbox_msg == ddr_1d_log_messages[i].id) {
				message = ddr_1d_log_messages[i].message;
				found_message = true;
				break;
			}
		}
	}

	if (found_message) {
		/* Get any remaining arguments for the print formatting */
		arg_count = mailbox_msg & DDR_PHY_STREAMING_MESSAGE_MASK;
		for (i = 0; i < arg_count; i++) {
			timeout = timeout_init_us(DDR_PHY_MAILBOX_TIMEOUT_US);
			while ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) != 0x0) {
				if (timeout_elapsed(timeout))
					/* If the streaming message doesn't show up in the mailbox, error out */
					return DDR_PHY_MAILBOX_NO_STREAMING_MESSAGE;
			}

			if (i >= DDR_PHY_STREAMING_MESSAGE_MAX_ARGUMENTS)
				return DDR_PHY_MAILBOX_TOO_MANY_ARGUMENTS;
			/* Get argument value and save it */
			mailbox_msg = mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTWRITEONLYSHADOW);
			mailbox_msg |= (mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTDATWRITEONLYSHADOW) << 16);
			arguments[i] = mailbox_msg;

			/* Tell PHY we are ready for the next argument by ACK'ing the current one */
			mmio_write_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_DCTWRITEPROT, 0x0);
			/* Wait for ACK from PHY */
			for (j = 0; j < DDR_PHY_MAILBOX_TIMEOUT_US; j++) {
				/* ACK the message and wait for DDR Phy ack back */
				if ((mmio_read_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_UCTSHADOWREGS) & DDR_PHY_UCTWRITEPROTSHADOWMASK) == 0x1) {
					mmio_write_32(base_addr_phy + DDRPHYA_APBONLY0_APBONLY0_DCTWRITEPROT, 0x1);
					break;
				} else {
					udelay(1);
				}
			}

			/* Error with the ACK */
			if (j == DDR_PHY_MAILBOX_TIMEOUT_US)
				return DDR_PHY_MAILBOX_ACK_FAILED;
		}
	} else {
		/* Set the first argument to the value of the streaming message ID for printing out to the log */
		arguments[0] = mailbox_msg;
	}

	/* Print the message. If no matching string was found, print the ID of the streaming message to the log */
	phy_print_streaming_message(message, arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], arguments[7], arguments[8], arguments[9], arguments[10], arguments[11], arguments[12], arguments[13], arguments[14], arguments[15], arguments[16], \
				    arguments[17], arguments[18], arguments[19], arguments[20], arguments[21], arguments[22], arguments[23], arguments[24], arguments[25], arguments[26], arguments[27], arguments[28], arguments[29], arguments[30], arguments[31]);
	return DDR_PHY_MAILBOX_STREAMING_MESSAGE;
}

/* Prints the message using the input string and arguments */
static void phy_print_streaming_message(const char *message, ...)
{
	va_list arg_ptr;

	va_start(arg_ptr, message);
	vprintf(message, arg_ptr);
	va_end(arg_ptr);
	return;
}

/* Gets the length and location of the desired IMEM/DMEM for loading */
static int phy_get_mem_info(bool select_imem, ddr_pstate_t pstate, int train_2d, uint16_t **mem_ptr, unsigned int *mem_length, ddr_config_t configuration)
{
	if (select_imem) {
		if (train_2d) {
			*mem_ptr = (uint16_t *)&adi_imem_2d;
			*mem_length = adi_imem_2d_end - adi_imem_2d;
		} else {
			*mem_ptr = (uint16_t *)&adi_imem_1d;
			*mem_length = adi_imem_1d_end - adi_imem_1d;
		}
	} else {
		switch (pstate) {
		case DDR_PSTATE0:
			if (train_2d) {
				if (configuration == DDR_PRIMARY_CONFIGURATION) {
					*mem_ptr = (uint16_t *)&adi_dmem0_2d;
					*mem_length = adi_dmem0_2d_end - adi_dmem0_2d;
				} else {
					*mem_ptr = (uint16_t *)&adi_sec_dmem0_2d;
					*mem_length = adi_sec_dmem0_2d_end - adi_sec_dmem0_2d;
				}
			} else {
				if (configuration == DDR_PRIMARY_CONFIGURATION) {
					*mem_ptr = (uint16_t *)&adi_dmem0_1d;
					*mem_length = adi_dmem0_1d_end - adi_dmem0_1d;
				} else {
					*mem_ptr = (uint16_t *)&adi_sec_dmem0_1d;
					*mem_length = adi_sec_dmem0_1d_end - adi_sec_dmem0_1d;
				}
			}
			break;
		default:
			ERROR("Invalid PSTATE for memory info.\n");
			*mem_ptr = NULL;
			break;
		}
		;
	}
	return 0;
}

/* Gets the base timing values for the controller */
static void get_base_umctl2_timing_values(uintptr_t base_addr_ctrl, uint64_t freq)
{
	/* Base timing values from the Synopsys PHY databook*/
	if (freq == 800) {
		umctl2TimingBaseValues.wr2rd = 0xE;
		umctl2TimingBaseValues.rd2wr = 0x6;
		umctl2TimingBaseValues.wr2rd_dr = 0xE;
		umctl2TimingBaseValues.wr2rd_s = 0x10;
		umctl2TimingBaseValues.diff_rank_rd_gap = 0x1;
		umctl2TimingBaseValues.diff_rank_wr_gap = 0x2;
		umctl2TimingBaseValues.wrdata_delay = 0x8;
	} else {
		umctl2TimingBaseValues.wr2rd = 0xE;
		umctl2TimingBaseValues.rd2wr = 0x6;
		umctl2TimingBaseValues.wr2rd_dr = 0xE;
		umctl2TimingBaseValues.wr2rd_s = 0x1C;
		umctl2TimingBaseValues.diff_rank_rd_gap = 0x1;
		umctl2TimingBaseValues.diff_rank_wr_gap = 0x2;
		umctl2TimingBaseValues.wrdata_delay = 0xC;
	}
}

/* Retrieves the training firmware timing results from the DDR PHY */
static void get_cdd_array(uintptr_t base_addr_phy, ucmtl2_timing_types_t timing_type, int8_t *array)
{
	uintptr_t address;
	uint16_t timings;
	int i;
	int index = 0;
	int entries;

	switch (timing_type) {
	case UMCTL2_RR:
		address = CDD_RR_START_ADDR;
		break;
	case UMCTL2_RW:
		address = CDD_RW_START_ADDR;
		break;
	case UMCTL2_WR:
		address = CDD_WR_START_ADDR;
		break;
	case UMCTL2_WW:
		address = CDD_WW_START_ADDR;
		break;
	}
	;

	/* Pull out the 0-0 timing, which is by itself in the message block */
	timings = mmio_read_16(base_addr_phy + address);
	array[index] = timings & 0xFF;
	index += 1;
	address -= 4;

	/* RR and WW have 4 fewer measurements, since there is no delay for consecutive reads or writes to the same rank, which means two fewer address in the mem block to read */
	if (timing_type == UMCTL2_RR || timing_type == UMCTL2_WW)
		entries = 5;
	else
		entries = 7;

	/* Loop over the each pair of values until the last value in the set, which will be by itself */
	for (i = 0; i < entries; i++) {
		timings = mmio_read_16(base_addr_phy + address);
		array[index] = timings & 0xFF;
		array[index + 1] = (timings & 0xFF00) >> 8;
		index += 2;
		/* Synopsys defined address are left shifted by 2 to match hardware, so a decrement of 1 in the message block address is 4 in ours */
		address -= 4;
	}

	/* Extract the last value from the upper 8 bits of the memory register */
	timings = mmio_read_16(base_addr_phy + address);
	array[index] = (timings & 0xFF00) >> 8;
	return;
}

/* Gets the value of a certain timing to update the controller with */
static uint8_t get_cdd_value(uintptr_t base_addr_phy, ucmtl2_timing_types_t timing_type, uint8_t ranks)
{
	int8_t cdd_array[16];
	int i, j, index;
	int8_t returnVal = 0;

	get_cdd_array(base_addr_phy, timing_type, cdd_array);
	switch (timing_type) {
	case UMCTL2_RR:
	case UMCTL2_WW:
		for (i = 0; i < ranks; i++) {
			/* Jump to each rank's place in the array, starting with 0 and going to 3 at the maximum */
			index = i * 3;
			/* RR and WW don't have a 0-0, 1-1, 2-2, or 3-3 measurement, so one fewer read per rank set */
			for (j = 0; j < (ranks - 1); j++) {
				/* We have to use the absolute value of the measurements */
				if (cdd_array[index] < 0)
					cdd_array[index] = cdd_array[index] * -1;
				if (cdd_array[index] > returnVal)
					returnVal = cdd_array[index];
				index += 1;
			}
		}
		break;
	case UMCTL2_RW:
	case UMCTL2_WR:
		for (i = 0; i < ranks; i++) {
			/* Jump to each rank's place in the array, starting with 0 and going to 3 at the maximum */
			index = i * 4;
			for (j = 0; j < ranks; j++) {
				/* We have to use the absolute value of the measurements */
				if (cdd_array[index] < 0)
					cdd_array[index] = cdd_array[index] * -1;
				if (cdd_array[index] > returnVal)
					returnVal = cdd_array[index];
				index += 1;
			}
		}
		break;
	default:
		return 0;
	}
	/* The controller uses the return value in terms of DFI clock instead of MEMCLK, so we need to divide by 2 or controller values will be double what is expected */
	return (uint8_t)returnVal / 2;
}

/* Updates the timing parameters in the controller when we switch pstates from the stored values in the global variable pstateTimings */
ddr_error_t update_umctl2_timing_values(uintptr_t base_addr_ctrl, ddr_pstate_t pstate)
{
	uint32_t timing_register;
	int i;

	/* The Timing registers are in quasi-dynamic group 2, so we must enter self-refresh before written values will be accepted.
	 * Step 1. Signal the software entry into self refresh */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x00000020);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		/* Check that self-refresh mode has been entered */
		if ((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_STAT) & STAT_OPERATING_MODE_MASK) == 0x00000003)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		/* Make sure self refresh was entered before of software request only, and not any other reason */
		if (((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_STAT) & STAT_SELFREF_TYPE_MASK) >> STAT_SELFREF_TYPE_SHIFT) == 0x00000002)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);

	DDR_DEBUG("Updating DDR controller values with results of training.\n");
	/* WR2RD Parameter */
	timing_register = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG2);
	timing_register &= ~DRAMTMG2_WR2RD_MASK;
	timing_register |= (pstateTimings[pstate].wr2rd) << DRAMTMG2_WR2RD_SHIFT;
	/* RD2WR Parameter */
	timing_register &= ~DRAMTMG2_RD2WR_MASK;
	timing_register |= (pstateTimings[pstate].rd2wr) << DRAMTMG2_RD2WR_SHIFT;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG2, timing_register);

	/* WR2RD_DR Parameter */
	timing_register = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_RANKCTL1);
	timing_register &= ~RANKCTL1_WR2RD_DR_MASK;
	timing_register |= (pstateTimings[pstate].wr2rd_dr) << RANKCTL1_WR2RD_DR_SHIFT;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RANKCTL1, timing_register);

	/* WR2RD_s Parameter */
	timing_register = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG9);
	timing_register &= ~DRAMTMG9_WR2RD_S_MASK;
	timing_register |= (pstateTimings[pstate].wr2rd_s) << DRAMTMG9_WR2RD_S_SHIFT;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG9, timing_register);

	/* DIFF_RANK_RD_GAP */
	timing_register = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_RANKCTL);
	timing_register &= ~RANKCTL_DIFF_RANK_RD_GAP_MASK;
	timing_register |= (pstateTimings[pstate].diff_rank_rd_gap) << RANKCTL_DIFF_RANK_RD_GAP_SHIFT;
	/* If value of diff_rank_rd_gap is over 0xF, we have to set the MSB bit in the register to 1 */
	if (pstateTimings[pstate].diff_rank_rd_gap > 0xF)
		timing_register |= RANKCTL_DIFF_RANK_RD_GAP_MSB_MASK;
	else
		timing_register &= ~RANKCTL_DIFF_RANK_RD_GAP_MSB_MASK;
	/* DIFF_RANK_WR_GAP */
	timing_register &= ~RANKCTL_DIFF_RANK_WR_GAP_MASK;
	timing_register |= (pstateTimings[pstate].diff_rank_wr_gap) << RANKCTL_DIFF_RANK_WR_GAP_SHIFT;
	/* If value of diff_rank_rd_gap is over 0xF, we have to set the MSB bit in the register to 1 */
	if (pstateTimings[pstate].diff_rank_rd_gap > 0xF)
		timing_register |= RANKCTL_DIFF_RANK_WR_GAP_MSB_MASK;
	else
		timing_register &= ~RANKCTL_DIFF_RANK_WR_GAP_MSB_MASK;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RANKCTL, timing_register);

	/* WRDATA_DELAY */
	timing_register = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFITMG1);
	timing_register &= ~DFITMG1_DFI_T_WRDATA_DELAY_MASK;
	timing_register |= (pstateTimings[pstate].wrdata_delay) << DFITMG1_DFI_T_WRDATA_DELAY_SHIFT;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFITMG1, timing_register);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x00000000);

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWSTAT) == 0x00000001)
			break;
		else
			mdelay(1);
	}

	return ERROR_DDR_NO_ERROR;
}

/* Gets the value of the wrdata delay by combining the coarse and fine delay into one value */
static uint8_t get_wrdata_delay(uintptr_t base_addr_phy, ddr_pstate_t pstate, uint8_t ranks)
{
	uint8_t return_val = 0;
	uint32_t timing_register;
	uint8_t coarse_delay, fine_delay;
	uintptr_t u0_address, u1_address;
	int i;

	switch (pstate) {
	case DDR_PSTATE0:
		u0_address = DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXDQSDLYTG0_U0_P0;
		u1_address = DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXDQSDLYTG0_U1_P0;
		break;
	default:
		return -1;
		break;
	}

	for (i = 0; i < ranks; i++) {
		timing_register = mmio_read_32(base_addr_phy + u0_address);
		/* Divide value by 32 since every increment of 1 in fine delay is 1/32 of actual timing, rounding to the nearest integer value */
		fine_delay = (timing_register & FINEDELAYMASK) >> 5;
		coarse_delay = (timing_register & COARSEDELAYMASK) >> 6;
		if (fine_delay > 0xF)
			coarse_delay += 1;
		if (coarse_delay > return_val)
			return_val = coarse_delay;
		u0_address += 4;
	}

	for (i = 0; i < ranks; i++) {
		timing_register = mmio_read_32(base_addr_phy + u1_address);
		/* Divide value by 32 since every increment of 1 in fine delay is 1/32 of actual timing, rounding to the nearest integer value */
		fine_delay = (timing_register & FINEDELAYMASK) >> 5;
		coarse_delay = (timing_register & COARSEDELAYMASK) >> 6;
		if (fine_delay > 0xF)
			coarse_delay += 1;
		if (coarse_delay > return_val)
			return_val = coarse_delay;
		u1_address += 4;
	}

	return return_val;
}
