/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Register offset */
/* DWC_mshc_block */
#define SDHCI_SDMASA_R_OFF                 (0x00U)
#define SDHCI_32BIT_BLK_CNT_R_OFF          (SDHCI_SDMASA_R_OFF)
#define SDHCI_BLOCK_SIZE_R_OFF             (0x04U)
#define SDHCI_BLOCK_COUNT_R_OFF            (0x06U)
#define SDHCI_ARGUMENT_R_OFF               (0x08U)
#define SDHCI_XFER_MODE_R_OFF              (0x0CU)
#define SDHCI_CMD_R_OFF                    (0x0EU)
#define SDHCI_RESP01_R_OFF                 (0x10U)
#define SDHCI_RESP23_R_OFF                 (0x14U)
#define SDHCI_RESP45_R_OFF                 (0x18U)
#define SDHCI_RESP67_R_OFF                 (0x1CU)
#define SDHCI_BUF_DATA_R_OFF               (0x20U)
#define SDHCI_PSTATE_REG_R_OFF             (0x24U)
#define SDHCI_HOST_CTRL1_R_OFF             (0x28U)
#define SDHCI_PWR_CTRL_R_OFF               (0x29U)
#define SDHCI_BGAP_CTRL_R_OFF              (0x2AU)
#define SDHCI_CLK_CTRL_R_OFF               (0x2CU)
#define SDHCI_TOUT_CTRL_R_OFF              (0x2EU)
#define SDHCI_SW_RST_R_OFF                 (0x2FU)
#define SDHCI_NORMAL_INT_STAT_R_OFF        (0x30U)
#define SDHCI_ERROR_INT_STAT_R_OFF         (0x32U)
#define SDHCI_NORMAL_INT_STAT_EN_R_OFF     (0x34U)
#define SDHCI_ERROR_INT_STAT_EN_R_OFF      (0x36U)
#define SDHCI_NORMAL_INT_SIGNAL_EN_R_OFF   (0x38U)
#define SDHCI_ERROR_INT_SIGNAL_EN_R_OFF    (0x3AU)
#define SDHCI_HOST_CTRL2_R_OFF             (0x3EU)
#define SDHCI_CAPABILITIES1_R_OFF          (0x40U)
#define SDHCI_ADMA_SA_LOW_R_OFF            (0x58U)
#define SDHCI_EMMC_CTRL_R_OFF              (0x52CU)
#define SDHCI_AT_CTRL_R_OFF                (0x540U)

/* Bit mask */
/* BLOCKSIZE_R */
#define XFER_BLOCK_SIZE_BM                 GENMASK(11, 0)
#define SDMA_BUF_BDARY_BM                  GENMASK(14, 12)
/* XFER_MODE_R */
#define DMA_ENABLE_BM                      BIT(0)
#define BLOCK_COUNT_ENABLE_BM              BIT(1)
#define AUTO_CMD_ENABLE_BM                 GENMASK(3, 2)
#define DATA_XFER_DIR_BM                   BIT(4)
#define MULTI_BLK_SEL_BM                   BIT(5)
#define RESPONSE_TYPE_BM                   BIT(6)
#define RESP_ERR_CHK_ENABLE_BM             BIT(7)
#define RESP_INT_DISABLE_BM                BIT(8)
/* CMD_R */
#define CMD_CRC_CHK_ENABLE_BM              BIT(3)
#define CMD_IDX_CHK_ENABLE_BM              BIT(4)
#define DATA_PRESENT_SEL_BM                BIT(5)
/* PSTATE_REG */
#define CMD_INHIBIT_BM                     BIT(0)
#define CMD_INHIBIT_DAT_BM                 BIT(1)
#define BUF_WR_ENABLE_BM                   BIT(10)
#define BUF_RD_ENABLE_BM                   BIT(11)
#define CARD_INSERTED_BM                   BIT(16)
/* HOST_CTRL1_R */
#define DAT_XFER_WIDTH_BM                  BIT(1)
#define DMA_SEL_BM                         GENMASK(4, 3)
#define EXT_DAT_XFER_BM                    BIT(5)
/* PWR_CTRL_R */
#define SD_BUS_PWR_VDD1_BM                 BIT(0)
#define SD_BUS_VOL_VDD1_BM                 GENMASK(3, 1)
/* BGAP_CTRL_R */
#define STOP_BG_REQ_BM                     BIT(0)
/* CLK_CTRL_R */
#define INTERNAL_CLK_EN_BM                 BIT(0)
#define INTERNAL_CLK_STABLE_BM             BIT(1)
#define SD_CLK_EN_BM                       BIT(2)
#define PLL_ENABLE_BM                      BIT(3)
#define CLK_GEN_SELECT_BM                  BIT(5)
#define UPPER_FREQ_SEL_BM                  GENMASK(7, 6)
#define FREQ_SEL_BM                        GENMASK(15, 8)
/* TOUT_CTRL_R */
#define TOUT_CNT_BM                        GENMASK(3, 0)
/* SW_RST_R */
#define SW_RST_ALL_BM                      BIT(0)
#define SW_RST_CMD_BM                      BIT(1)
#define SW_RST_DAT_BM                      BIT(2)
/* NORMAL_INT_STAT_R/NORMAL_INT_STAT_EN_R */
#define CMD_COMPLETE_BM                    BIT(0)
#define XFER_COMPLETE_BM                   BIT(1)
#define DMA_INTERRUPT_BM                   BIT(3)
#define BUF_WR_READY_STAT_EN_BM            BIT(4)
#define BUF_RD_READY_STAT_EN_BM            BIT(5)
#define ERR_INTERRUPT_BM                   BIT(15)
/* ERROR_INT_STAT_R/ERROR_INT_STAT_EN_R*/
#define CMD_TOUT_ERR_STAT_EN_BM            BIT(0)
#define CMD_CRC_ERR_STAT_EN_BM             BIT(1)
#define CMD_END_BIT_ERR_STAT_EN_BM         BIT(2)
#define CMD_IDX_ERR_STAT_EN_BM             BIT(3)
#define DATA_TOUT_ERR_STAT_EN_BM           BIT(4)
#define DATA_CRC_ERR_STAT_EN_BM            BIT(5)
#define DATA_END_BIT_ERR_STAT_EN_BM        BIT(6)
#define ADMA_ERROR_STATUS_EN_BM            BIT(9)
#define CMD_LINE_ERR_INTR_BM               (CMD_TOUT_ERR_STAT_EN_BM | CMD_CRC_ERR_STAT_EN_BM | \
					    CMD_END_BIT_ERR_STAT_EN_BM | CMD_IDX_ERR_STAT_EN_BM)
#define DATA_LINE_ERR_INTR_BM              (DATA_TOUT_ERR_STAT_EN_BM | DATA_CRC_ERR_STAT_EN_BM | \
					    DATA_END_BIT_ERR_STAT_EN_BM)
/* HOST_CTRL2_R */
#define UHS_MODE_SEL_BM                    GENMASK(2, 0)
#define SIGNALING_EN                       BIT(3)
#define UHS2_IF_ENABLE_BM                  BIT(8)
#define HOST_VER4_ENABLE_BM                BIT(12)
#define ADDRESSING_BM                      BIT(13)
#define ASYNC_INT_ENABLE_BM                BIT(14)
#define PRESET_VAL_ENABLE_BM               BIT(15)
/* CAPABILITIES1_R */
#define BASE_CLK_FREQ_BM                   GENMASK(15, 8)
#define SYS_ADDR_64_V4_BM                  BIT(27)
#define ASYNC_INT_SUPPORT_BM               BIT(29)
/* EMMC_CTRL_R */
#define CARD_IS_EMMC_BM                    BIT(0)

/* Bit field values */
#define SD_BUS_VOL_VDD1_1V8                (0x0000000AU)
#define HOST_VER4_ENABLE_V4                (1U)
#define HOST_VER4_ENABLE_V3                (0U)
#define RESP_INT_DISABLED                  (1U)
#define RESP_INT_ENABLED                   (0U)
#define TOUT_CNT_VALUE14                   (0x0EU)
/* - AT_CTRL_R */
#define POST_CHANGE_DLY_LESS_4_CYCLES      (0x03U)
#define TUNE_CLK_STOP_EN                   (1U)

/* Bit positions */
#define HOST_VER4_ENABLE_POS               (12U)
#define BASE_CLK_FREQ_POS                  (8U)
#define FREQ_SEL_POS                       (8U)
#define UPPER_FREQ_SEL_POS                 (6U)
#define CMD_TYPE_POS                       (6U)
#define DMA_SEL_POS                        (3U)
#define SDMA_BUF_BDARY_POS                 (12U)
/* - AT_CTRL_R */
#define POST_CHANGE_DLY_OFF                (19U)
#define TUNE_CLK_STOP_EN_OFF               (16U)

/* Upper Frequency divider select mask */
#define UPPER_FREQ_SEL                     (0x300U)

/* Lower Frequency divider select mask */
#define LOWER_FREQ_SEL                     (0xFFU)

/* RESP_TYPE_SELECT values in CMD_R register */
#define  SDHCI_CMD_RESP_NONE               (0x00U)
#define  SDHCI_CMD_RESP_LONG               (0x01U)
#define  SDHCI_CMD_RESP_SHORT              (0x02U)
#define  SDHCI_CMD_RESP_SHORT_BUSY         (0x03U)

/* CMD_TYPE values in CMD_R register */
#define CMD_TYPE_NORMAL                    (0x00U)
#define CMD_TYPE_SUSPEND                   (0x01U)
#define CMD_TYPE_RESUME                    (0x02U)
#define CMD_TYPE_ABORT                     (0x03U)

/* Mask value for interrupt to clear all the interrupts */
#define NORMAL_INT_STAT_MASK               (0x40FFU)
#define ERROR_INT_STAT_MASK                (0xFFFFU)

/* DMA_SEL values in HOST_CTRL1_R */
#define DMA_SEL_SDMA                       (0x00U)
#define DMA_SEL_RESERVED                   (0x01U)
#define DMA_SEL_ADMA2                      (0x02U)
#define DMA_SEL_ADMA2_OR_ADMA3             (0x03U)

/* SDMA Buffer Boundary value */
#define SDMA_BUF_BDARY_512K                (7U)
