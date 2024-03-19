/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Register offset */
/* DWC_mshc_phy_block */
#define SDHCI_PHY_CNFG_R_OFF               (0x00U)
#define SDHCI_PHY_CMDPAD_CNFG_R_OFF        (0x04U)
#define SDHCI_PHY_DATPAD_CNFG_R_OFF        (0x06U)
#define SDHCI_PHY_STBPAD_CNFG_R_OFF        (0x0AU)
#define SDHCI_PHY_RSTNPAD_CNFG_R_OFF       (0x0CU)
#define SDHCI_PHY_COMMDL_CNFG_R_OFF        (0x1CU)
#define SDHCI_PHY_SDCLKDL_CNFG_R_OFF       (0x1DU)
#define SDHCI_PHY_SDCLKDL_DC_R_OFF         (0x1EU)
#define SDHCI_PHY_SMPLDL_CNFG_R_OFF        (0x20U)
#define SDHCI_PHY_ATDL_CNFG_R_OFF          (0x21U)

/* Bit mask */
/* PHY_CNFG */
#define PHY_RSTN_BM                        BIT(0)
#define PHY_POWERGOOD_BM                   BIT(1)
/* *PAD_CNFG */
#define RXSEL_BM                           GENMASK(2, 0)
#define WEAKPULL_EN_BM                     GENMASK(4, 3)
/* SMPLDL_CNFG/SDCLKDL_CNFG/ATDL_CNFG/COMMDL_CNFG */
#define INPSEL_CNFG_BM                     GENMASK(3, 2)
#define UPDATE_DC_BM                       BIT(4)
#define DLSTEP_SEL_BM                      BIT(0)
/* SDCLKDL_DC */
#define CCLK_DC_BM                         GENMASK(6, 0)

/* Bit field values */
#define PHY_PAD_SN                         (0x8U)
#define PHY_PAD_SP                         (0x8U)
#define RXSEL_CMD_PAD                      (0x1U)
#define RXSEL_DAT_PAD                      (0x1U)
#define RXSEL_RST_N_PAD                    (0x1U)
#define RXSEL_STB_N_PAD                    (0x1U)
#define WEAKPULL_EN_CMD_PAD                (0x1U)
#define WEAKPULL_EN_DAT_PAD                (0x1U)
#define WEAKPULL_EN_RST_N_PAD              (0x1U)
#define WEAKPULL_EN_STB_PAD                (0x2U)
#define INPSEL_CNFG_SDCLKDL                (0x0U)
#define INPSEL_CNFG_SMPLDL                 (0x2U)
#define INPSEL_CNFG_ATDL                   (0x2U)
#define UPDATE_DC_SDCLKDL                  (0x1U)
#define DLSTEP_SEL                         (0x1U)
#define CCLK_DC                            (0x78U)

/* Bit positions */
#define PAD_SP_POS                         (16U)
#define PAD_SN_POS                         (20U)
#define WEAKPULL_EN_POS                    (3U)
#define INPSEL_CNFG_POS                    (2U)
#define UPDATE_DC_POS                      (4U)
#define DLSTEP_SEL_POS                     (0U)
#define CCLK_DC_POS                        (0U)
