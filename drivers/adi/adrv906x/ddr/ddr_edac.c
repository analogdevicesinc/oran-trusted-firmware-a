/*
 * Copyright (c) 2026, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>

#include <drivers/adi/adrv906x/ddr/ddr.h>

#include <lib/mmio.h>

#include "ddr_regmap.h"

/* This table comes directly from section 2.39.5 in the DDR controller databook */
#define DDR_CORRECTED_BIT_NUM_ENCODING_MAX      72
static uint8_t ddr_corrected_bit_num_encoding[DDR_CORRECTED_BIT_NUM_ENCODING_MAX] = {
	64, 65, 66, 0,	67, 1,	2,  3,
	68, 4,	5,  6,	7,  8,	9,  10,
	69, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25,
	70, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56,
	71, 57, 58, 59, 60, 61, 62, 63
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

bool ddr_get_ecc_enabled_state(uintptr_t base_addr_ctrl)
{
	return ((mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG0) & ECCCFG0_ECC_MODE_MASK) >> ECCCFG0_ECC_MODE_SHIFT) != 0;
}

uint32_t ddr_get_address_map_offset(uintptr_t base_addr_ctrl, unsigned int bank)
{
	return mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP0 + (bank * 4));
}

ddr_dtype_t ddr_get_dtype(uintptr_t base_addr_ctrl)
{
	unsigned short data_width = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_MSTR) & MSTR_DATA_BUS_WIDTH_MASK) >> MSTR_DATA_BUS_WIDTH_SHIFT;

	/* if halved; 8 bit data width */
	if (data_width)
		return DEV_X1;
	/* if full; 16 bit data width */
	return DEV_X2;
}

static bool ddr_is_ddr4(uintptr_t base_addr_ctrl)
{
	return (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_MSTR) & MSTR_DDR4_MASK) >> MSTR_DDR4_SHIFT != 0;
}

static bool ddr_is_ddr3(uintptr_t base_addr_ctrl)
{
	return (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_MSTR) & MSTR_DDR3_MASK) >> MSTR_DDR3_SHIFT != 0;
}

ddr_mtype_t ddr_get_mtype(uintptr_t base_addr_ctrl)
{
	if (ddr_is_ddr4(base_addr_ctrl))
		return MEM_DDR4;
	else if (ddr_is_ddr3(base_addr_ctrl))
		return MEM_DDR3;
	else
		return MEM_DDR2;
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

bool ddr_get_ecc_syndrome_mask(uintptr_t base_addr_ctrl, uint32_t *syndrome)
{
	uint32_t mask = 0;

	if (!syndrome)
		return false;

	syndrome[0] = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCSYN0) & ECCCSYN0_ECC_CORR_SYNDROMES_31_0_MASK) >> ECCCSYN0_ECC_CORR_SYNDROMES_31_0_SHIFT;
	syndrome[1] = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCBITMASK0) & ECCBITMASK0_ECC_CORR_BIT_MASK_31_0_MASK) >> ECCBITMASK0_ECC_CORR_BIT_MASK_31_0_SHIFT;
	syndrome[2] = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCSYN1) & ECCCSYN1_ECC_CORR_SYNDROMES_63_32_MASK) >> ECCCSYN1_ECC_CORR_SYNDROMES_63_32_SHIFT;
	syndrome[3] = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCBITMASK1) & ECCBITMASK1_ECC_CORR_BIT_MASK_63_32_MASK) >> ECCBITMASK1_ECC_CORR_BIT_MASK_63_32_SHIFT;
	syndrome[4] = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCSYN2) & ECCCSYN2_ECC_CORR_SYNDROMES_71_64_MASK) >> ECCCSYN2_ECC_CORR_SYNDROMES_71_64_SHIFT;
	syndrome[5] = (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCBITMASK2) & ECCBITMASK2_ECC_CORR_BIT_MASK_71_64_MASK) >> ECCBITMASK2_ECC_CORR_BIT_MASK_71_64_SHIFT;

	return true;
}

void ddr_set_data_inject_poison(uintptr_t base_addr_ctrl, unsigned int rank, unsigned int col, unsigned int bank_grp, unsigned int bank, unsigned int row)
{
	uint32_t cfg;

	cfg = (rank << ECCPOISONADDR0_ECC_POISON_RANK_SHIFT) & ECCPOISONADDR0_ECC_POISON_RANK_MASK;
	cfg |= (col << ECCPOISONADDR0_ECC_POISON_COL_SHIFT) & ECCPOISONADDR0_ECC_POISON_COL_MASK;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONADDR0, cfg);
	cfg = (bank << ECCPOISONADDR1_ECC_POISON_BANK_SHIFT) & ECCPOISONADDR1_ECC_POISON_BANK_MASK;
	cfg |= (bank_grp << ECCPOISONADDR1_ECC_POISON_BG_SHIFT) & ECCPOISONADDR1_ECC_POISON_BG_MASK;
	cfg |= (row << ECCPOISONADDR1_ECC_POISON_ROW_SHIFT) & ECCPOISONADDR1_ECC_POISON_ROW_MASK;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONADDR1, cfg);
}

void ddr_get_data_inject_poison(uintptr_t base_addr_ctrl, unsigned int *rank, unsigned int *col, unsigned int *bank_grp, unsigned int *bank, unsigned int *row)
{
	uint32_t cfg;

	cfg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONADDR0);
	if (rank)
		*rank = (cfg & ECCPOISONADDR0_ECC_POISON_RANK_MASK) >> ECCPOISONADDR0_ECC_POISON_RANK_SHIFT;
	if (col)
		*col = (cfg & ECCPOISONADDR0_ECC_POISON_COL_MASK) >> ECCPOISONADDR0_ECC_POISON_COL_SHIFT;

	cfg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONADDR1);
	if (bank_grp)
		*bank_grp = (cfg & ECCPOISONADDR1_ECC_POISON_BG_MASK) >> ECCPOISONADDR1_ECC_POISON_BG_SHIFT;
	if (bank)
		*bank = (cfg & ECCPOISONADDR1_ECC_POISON_BANK_MASK) >> ECCPOISONADDR1_ECC_POISON_BANK_SHIFT;
	if (row)
		*row = (cfg & ECCPOISONADDR1_ECC_POISON_ROW_MASK) >> ECCPOISONADDR1_ECC_POISON_ROW_SHIFT;
}

void ddr_set_data_poison_config(uintptr_t base_addr_ctrl, bool enable, bool double_error)
{
	uint32_t orig;
	uint32_t cfg;

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000000);
	cfg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG1) | ~(ECCCFG1_DATA_POISON_EN_MASK | ECCCFG1_DATA_POISON_BIT_MASK);
	orig = cfg;
	if (enable)
		cfg |= ECCCFG1_DATA_POISON_EN_MASK;
	else
		cfg &= ~ECCCFG1_DATA_POISON_EN_MASK;
	if (double_error)
		cfg |= ECCCFG1_DATA_POISON_BIT_MASK;
	else
		cfg &= ~ECCCFG1_DATA_POISON_BIT_MASK;
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG1, cfg);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
}

void ddr_get_data_poison_config(uintptr_t base_addr_ctrl, bool *enable, bool *double_error)
{
	uint32_t cfg;

	cfg = mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG1);
	if (enable)
		*enable = (cfg & ECCCFG1_DATA_POISON_EN_MASK) >> ECCCFG1_DATA_POISON_EN_SHIFT;
	if (double_error)
		*double_error = (cfg & ECCCFG1_DATA_POISON_BIT_MASK) >> ECCCFG1_DATA_POISON_BIT_SHIFT;
}
