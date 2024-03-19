/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "adi_sdhci_phy_hw.h"

/* PHY Powergood timeout value */
#define SDHCI_PHY_PGGOOD_TIMEOUT_US_100_MS          (100000U)

int adi_sdhci_phy_init(uintptr_t base)
{
	uint64_t timeout;
	uint16_t u16_reg_data;
	uint8_t u8_reg_data;
	uint8_t u8_aux;

	/* eMMC PHY general configuration */
	mmio_setbits_32(base + SDHCI_PHY_CNFG_R_OFF, ((PHY_PAD_SP << PAD_SP_POS) |
						      (PHY_PAD_SN << PAD_SN_POS)));

	/* eMMC PHY's command/response PAD settings */
	u16_reg_data = mmio_read_16(base + SDHCI_PHY_CMDPAD_CNFG_R_OFF) & ~(RXSEL_BM | WEAKPULL_EN_BM);
	u16_reg_data |= (RXSEL_CMD_PAD | (WEAKPULL_EN_CMD_PAD << WEAKPULL_EN_POS));
	mmio_write_16(base + SDHCI_PHY_CMDPAD_CNFG_R_OFF, u16_reg_data);

	/* eMMC PHY's Data PAD settings */
	u16_reg_data = mmio_read_16(base + SDHCI_PHY_DATPAD_CNFG_R_OFF) & ~(RXSEL_BM | WEAKPULL_EN_BM);
	u16_reg_data |= (RXSEL_DAT_PAD | (WEAKPULL_EN_DAT_PAD << WEAKPULL_EN_POS));
	mmio_write_16(base + SDHCI_PHY_DATPAD_CNFG_R_OFF, u16_reg_data);

	/* eMMC PHY's RSTN PAD settings */
	u16_reg_data = mmio_read_16(base + SDHCI_PHY_RSTNPAD_CNFG_R_OFF) & ~(RXSEL_BM | WEAKPULL_EN_BM);
	u16_reg_data |= (RXSEL_RST_N_PAD | (WEAKPULL_EN_RST_N_PAD << WEAKPULL_EN_POS));
	mmio_write_16(base + SDHCI_PHY_RSTNPAD_CNFG_R_OFF, u16_reg_data);

	/* eMMC PHY's Strobe PAD settings */
	u16_reg_data = mmio_read_16(base + SDHCI_PHY_STBPAD_CNFG_R_OFF) & ~(RXSEL_BM | WEAKPULL_EN_BM);
	u16_reg_data |= (RXSEL_STB_N_PAD | (WEAKPULL_EN_STB_PAD << WEAKPULL_EN_POS));
	mmio_write_16(base + SDHCI_PHY_STBPAD_CNFG_R_OFF, u16_reg_data);

	/* eMMC cclk_rx DelayLine input source selection */
	u8_reg_data = mmio_read_8(base + SDHCI_PHY_SMPLDL_CNFG_R_OFF) & ~INPSEL_CNFG_BM;
	u8_reg_data |= (INPSEL_CNFG_SMPLDL << INPSEL_CNFG_POS);
	mmio_write_8(base + SDHCI_PHY_SMPLDL_CNFG_R_OFF, u8_reg_data);

	/* eMMC clk_tx DelayLine input source selection */
	u8_reg_data = mmio_read_8(base + SDHCI_PHY_SDCLKDL_CNFG_R_OFF) & ~INPSEL_CNFG_BM;
	u8_reg_data |= (INPSEL_CNFG_SDCLKDL << INPSEL_CNFG_POS);
	mmio_write_8(base + SDHCI_PHY_SDCLKDL_CNFG_R_OFF, u8_reg_data);

	/* eMMC clk_tx DelayLine value settings (for default speed)
	 *   Note: Card clock must be disabled (it is during this initialization phase)
	 */
	u8_reg_data |= (UPDATE_DC_SDCLKDL << UPDATE_DC_POS);
	mmio_write_8(base + SDHCI_PHY_SDCLKDL_CNFG_R_OFF, u8_reg_data);
	u8_aux = mmio_read_8(base + SDHCI_PHY_SDCLKDL_DC_R_OFF) & ~CCLK_DC_BM;
	u8_aux |= (CCLK_DC << CCLK_DC_POS);
	mmio_write_8(base + SDHCI_PHY_SDCLKDL_DC_R_OFF, u8_aux);
	u8_reg_data &= ~(UPDATE_DC_SDCLKDL << UPDATE_DC_POS);
	mmio_write_8(base + SDHCI_PHY_SDCLKDL_CNFG_R_OFF, u8_reg_data);

	/* eMMC drift_cclk_rx DelayLine input source selection */
	u8_reg_data = mmio_read_8(base + SDHCI_PHY_ATDL_CNFG_R_OFF) & ~INPSEL_CNFG_BM;
	u8_reg_data |= (INPSEL_CNFG_ATDL << INPSEL_CNFG_POS);
	mmio_write_8(base + SDHCI_PHY_ATDL_CNFG_R_OFF, u8_reg_data);

	/* eMMC DelayLine's per step delay selection */
	u8_reg_data = mmio_read_8(base + SDHCI_PHY_COMMDL_CNFG_R_OFF) & ~DLSTEP_SEL_BM;
	u8_reg_data |= (DLSTEP_SEL << DLSTEP_SEL_POS);
	mmio_write_8(base + SDHCI_PHY_COMMDL_CNFG_R_OFF, u8_reg_data);

	/* Wait max 100ms for the PHY Powergood to be 1. As per JEDEC Spec v5.1,
	 * supply power-up time for eMMC operating at 1.8V is 25ms, but we give
	 * more time for the PHY to powerup. */
	timeout = timeout_init_us(SDHCI_PHY_PGGOOD_TIMEOUT_US_100_MS);
	while (0U == (mmio_read_32(base + SDHCI_PHY_CNFG_R_OFF) & PHY_POWERGOOD_BM)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: PHY Powergood status never asserted.\n", __func__);
			return -ETIMEDOUT;
		}
	}

	/* De-assert PHY Reset */
	mmio_setbits_32(base + SDHCI_PHY_CNFG_R_OFF, PHY_RSTN_BM);

	return 0;
}
