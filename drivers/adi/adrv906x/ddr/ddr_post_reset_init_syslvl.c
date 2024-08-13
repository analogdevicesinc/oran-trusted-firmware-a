/*
 * Copyright(c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX - License - Identifier: BSD - 3 - Clause
 */
#include <adrv906x_device_profile.h>

#include <drivers/adi/adrv906x/ddr/ddr.h>
#include <drivers/adi/adrv906x/ddr/ddr_phy.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "ddr_config.h"
#include "ddr_regmap.h"

/**
 *******************************************************************************
 * Function: ddr_post_reset_init
 *
 * @brief      Writes dynamic registers to DDR controller
 *
 * @details    Writes dynamic registers to DDR controller
 *
 * Parameters:
 * @param [in]  None
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************
 */
ddr_error_t ddr_post_reset_init(uintptr_t base_addr_ctrl, uintptr_t base_addr_phy, uintptr_t base_addr_adi_interface, uintptr_t base_addr_clk, ddr_init_stages_t stage, ddr_config_t configuration)
{
	ddr_error_t return_val = ERROR_DDR_NO_ERROR;
	int i;

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1, 0x00000000);

	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1) == 0x00000000)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	/* Step 4 in table 6-8 of controller databook, keeps SDRAM in self-refresh per the application note */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x00000020);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL) == 0x00000020)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_MSTR) == 0x81040210)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	/* Step 5 in table 6-8, no reading of ACK needed since UMCTL2_OCCAP_EN=0 */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);


	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC, 0x00000050);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC) == 0x00000050)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;


/* Added this according to step 7 of the ddr_umctl2 document. This was not part of the test vector */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWSTAT) == 0x00000001)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	/* Steps 8-13 in table 6-8 will be handled in this function */
	return_val = ddr_function_configurations[configuration].phy_function(base_addr_ctrl, base_addr_phy, base_addr_adi_interface, base_addr_clk, stage, configuration);
	if (return_val != ERROR_DDR_NO_ERROR || (stage == DDR_CUSTOM_TRAINING))
		return return_val;


	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_phy + DDRPHYA_MASTER0_P0_MASTER0_P0_CALBUSY) == 0x00000000)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);


	/* Start DFI init and wait for it to be done */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC, 0x00000070);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);

	/* Poll DFI stat register for DFI init complete signal */
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFISTAT) == 0x00000001)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);

	/* Set DFI init start to 0 */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC, 0x00000050);
	/* Set dfi_init_complete_en to 1 based on table 6-8 */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC, 0x00000051);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC) == 0x00000051)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	/* Step 23 in table 6-8 */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x00000000);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL) == 0x00000000)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	/* Step 24 in table 6-8 */
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWSTAT) == 0x00000001)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;
	/* Wait for controller to enter normal operating mode */
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_STAT) == 0x00000001)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;


	return return_val;
}
