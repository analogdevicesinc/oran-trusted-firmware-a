/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>

#include <drivers/adi/adrv906x/ddr/ddr.h>

#include <lib/mmio.h>

#include "ddr_regmap.h"

/* This table comes directly from section 2.39.5 in the DDR controller databook */
#define DDR_CORRECTED_BIT_NUM_ENCODING_MAX      22
static uint8_t ddr_corrected_bit_num_encoding[DDR_CORRECTED_BIT_NUM_ENCODING_MAX] = {
	16, 17, 18, 0, 19, 1, 2, 3, 20, 4, 5, 6, 7, 8, 9, 10, 21, 11, 12, 13, 14, 15
};

/* Returns the current count of correctable errors */
static uint16_t ddr_get_correctable_error_count(uintptr_t base_addr_ctrl)
{
	uint16_t ecc_err_cnt = 0;

	ecc_err_cnt = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCERRCNT) & ECCERRCNT_ECC_CORR_ERR_CNT_MASK) >> ECCERRCNT_ECC_CORR_ERR_CNT_SHIFT;
	return ecc_err_cnt;
}

/* Returns the current count of uncorrectable error */
static uint16_t ddr_get_uncorrectable_error_count(uintptr_t base_addr_ctrl)
{
	uint16_t ecc_err_cnt = 0;

	ecc_err_cnt = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCERRCNT) & ECCERRCNT_ECC_UNCORR_ERR_CNT_MASK) >> ECCERRCNT_ECC_UNCORR_ERR_CNT_SHIFT;
	return ecc_err_cnt;
}

static bool ddr_check_ecc_error_status(uintptr_t base_addr_ctrl, bool correctable)
{
	if (correctable)
		return ((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCSTAT) & ECCSTAT_ECC_CORRECTED_ERR_MASK) >> ECCSTAT_ECC_CORRECTED_ERR_SHIFT) == 1;
	else
		return ((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCSTAT) & ECCSTAT_ECC_UNCORRECTED_ERR_MASK) >> ECCSTAT_ECC_UNCORRECTED_ERR_SHIFT) == 1;
}

/* Clears the AP error from the controller side so interrupt line is not continuously triggered */
void ddr_clear_ap_error(uintptr_t base_addr_ctrl)
{
	uint32_t register_data;

	register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL);
	register_data |= ECCCTL_ECC_AP_ERR_INTR_CLR_MASK;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL, register_data);
}

/* Retrieves the info(row, bank, etc.) of an ECC error */
bool ddr_get_ecc_error_info(uintptr_t base_addr_ctrl, bool correctable, ddr_ecc_error_data_t *data)
{
	uint32_t register_data;

	if (!ddr_check_ecc_error_status(base_addr_ctrl, correctable))
		return false;

	if (correctable) {
		/* Retrieve the rank and row from ECCCADDR0 */
		register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCADDR0);
		data->rank = (register_data & ECCCADDR0_ECC_CORR_RANK_MASK) >> ECCCADDR0_ECC_CORR_RANK_SHIFT;
		data->row = (register_data & ECCCADDR0_ECC_CORR_ROW_MASK) >> ECCCADDR0_ECC_CORR_ROW_SHIFT;

		/* Retrieve the bank group, bank num, and block num from ECCCADDR1 */
		register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCADDR1);
		data->bank_group = (register_data & ECCCADDR1_ECC_CORR_BG_MASK) >> ECCCADDR1_ECC_CORR_BG_SHIFT;
		data->bank = (register_data & ECCCADDR1_ECC_CORR_BANK_MASK) >> ECCCADDR1_ECC_CORR_BANK_SHIFT;
		data->block = (register_data & ECCCADDR1_ECC_CORR_COL_MASK) >> ECCCADDR1_ECC_CORR_COL_SHIFT;

		/* Decode the corrected bit number */
		register_data = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCSTAT) & ECCSTAT_ECC_CORRECTED_BIT_NUM_MASK) >> ECCSTAT_ECC_CORRECTED_BIT_NUM_SHIFT;
		if (register_data >= DDR_CORRECTED_BIT_NUM_ENCODING_MAX) {
			WARN("ECCSTAT returned corrected bit number outside of allowed range, setting number to 255\n");
			data->corrected_bit_num = 255;
		} else {
			data->corrected_bit_num = ddr_corrected_bit_num_encoding[register_data];
		}

		data->error_count = ddr_get_correctable_error_count(base_addr_ctrl);

		register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL);
		register_data |= ECCCTL_ECC_CORRECTED_ERR_CLR_MASK;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL, register_data);
	} else {
		/* Retrieve the rank and row from ECCUADDR0 */
		register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCUADDR0);
		data->rank = (register_data & ECCUADDR0_ECC_UNCORR_RANK_MASK) >> ECCUADDR0_ECC_UNCORR_RANK_SHIFT;
		data->row = (register_data & ECCUADDR0_ECC_UNCORR_ROW_MASK) >> ECCUADDR0_ECC_UNCORR_ROW_SHIFT;

		/* Retrieve the bank group, bank num, and block num from ECCUADDR1 */
		register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCADDR1);
		data->bank_group = (register_data & ECCUADDR1_ECC_UNCORR_BG_MASK) >> ECCUADDR1_ECC_UNCORR_BG_SHIFT;
		data->bank = (register_data & ECCUADDR1_ECC_UNCORR_BANK_MASK) >> ECCUADDR1_ECC_UNCORR_BANK_SHIFT;
		data->block = (register_data & ECCUADDR1_ECC_UNCORR_COL_MASK) >> ECCUADDR1_ECC_UNCORR_COL_SHIFT;

		data->error_count = ddr_get_uncorrectable_error_count(base_addr_ctrl);
		register_data = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL);
		register_data |= ECCCTL_ECC_UNCORRECTED_ERR_CLR_MASK;
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL, register_data);
	}

	return true;
}
