/*
 * Copyright (c) 2024, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include "adrv906x_pinmux_source_def.h"

CASSERT(ADRV906X_PINMUX_NUM_SRCS <= UINT16_MAX, pinmux_source_t_larger_than_uint16_t);
uint16_t pinmux_config[][ADRV906X_PINMUX_SRC_PER_PIN] = {
	{ SD_CMD,				      SPI_MASTER3_MISO,	     RFFE_13,	    A55_GPIO_S_0,    A55_GPIO_NS_0,   PWM_1	      },        /* Pin 0 */
	{ SD_CLK_SEL,				      SPI_MASTER3_MOSI,	     RFFE_14,	    A55_GPIO_S_1,    A55_GPIO_NS_1,   PWM_2	      },        /* Pin 1 */
	{ SD_DATA0,				      SPI_MASTER3_CLK,	     RFFE_15,	    A55_GPIO_S_2,    A55_GPIO_NS_2,   PWM_3	      },        /* Pin 2 */
	{ SD_DATA1,				      SPI_MASTER3_SELB,	     RFFE_16,	    A55_GPIO_S_3,    A55_GPIO_NS_3,   PWM_4	      },        /* Pin 3 */
	{ SD_DATA2,				      I2C3_SCL,		     RFFE_17,	    A55_GPIO_S_4,    A55_GPIO_NS_4,   PWM_5	      },        /* Pin 4 */
	{ SD_DATA3,				      I2C3_SDA,		     RFFE_18,	    A55_GPIO_S_5,    A55_GPIO_NS_5,   PWM_6	      },        /* Pin 5 */
	{ SD_CARDDETECT,			      PWM_0,		     PPI16_PDI_0,   A55_GPIO_S_6,    A55_GPIO_NS_6,   NO_SIGNAL	      },        /* Pin 6 */
	{ RFFE_0,				      NO_SIGNAL,	     PPI16_PDI_1,   A55_GPIO_S_7,    A55_GPIO_NS_7,   NO_SIGNAL	      },        /* Pin 7 */
	{ RFFE_1,				      NO_SIGNAL,	     PPI16_PDI_2,   A55_GPIO_S_8,    A55_GPIO_NS_8,   NO_SIGNAL	      },        /* Pin 8 */
	{ RFFE_2,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_9,    A55_GPIO_NS_9,   NO_SIGNAL	      },        /* Pin 9 */
	{ RFFE_3,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_10,   A55_GPIO_NS_10,  NO_SIGNAL	      },        /* Pin 10 */
	{ RFFE_4,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_11,   A55_GPIO_NS_11,  NO_SIGNAL	      },        /* Pin 11 */
	{ RFFE_5,				      NO_SIGNAL,	     PPI16_PDI_4,   A55_GPIO_S_12,   A55_GPIO_NS_12,  NO_SIGNAL	      },        /* Pin 12 */
	{ RFFE_6,				      NO_SIGNAL,	     PPI16_PDI_5,   A55_GPIO_S_13,   A55_GPIO_NS_13,  NO_SIGNAL	      },        /* Pin 13 */
	{ RFFE_7,				      NO_SIGNAL,	     PPI16_PDI_6,   A55_GPIO_S_14,   A55_GPIO_NS_14,  NO_SIGNAL	      },        /* Pin 14 */
	{ RFFE_8,				      NO_SIGNAL,	     PPI16_PDI_7,   A55_GPIO_S_15,   A55_GPIO_NS_15,  NO_SIGNAL	      },        /* Pin 15 */
	{ RFFE_9,				      NO_SIGNAL,	     PPI16_PDI_8,   A55_GPIO_S_16,   A55_GPIO_NS_16,  NO_SIGNAL	      },        /* Pin 16 */
	{ RFFE_10,				      NO_SIGNAL,	     PPI16_PDI_9,   A55_GPIO_S_17,   A55_GPIO_NS_17,  NO_SIGNAL	      },        /* Pin 17 */
	{ RFFE_11,				      NO_SIGNAL,	     PPI16_PDI_11,  A55_GPIO_S_18,   A55_GPIO_NS_18,  NO_SIGNAL	      },        /* Pin 18 */
	{ RFFE_12,				      NO_SIGNAL,	     PPI16_PDI_12,  A55_GPIO_S_19,   A55_GPIO_NS_19,  NO_SIGNAL	      },        /* Pin 19 */
	{ I2C1_SCL,				      NO_SIGNAL,	     PPI16_PDI_13,  A55_GPIO_S_20,   A55_GPIO_NS_20,  NO_SIGNAL	      },        /* Pin 20 */
	{ I2C1_SDA,				      NO_SIGNAL,	     PPI16_PDI_14,  A55_GPIO_S_21,   A55_GPIO_NS_21,  NO_SIGNAL	      },        /* Pin 21 */
	{ TEMP_SENSOR_INT0,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_22,   A55_GPIO_NS_22,  NO_SIGNAL	      },        /* Pin 22 */
	{ TEMP_SENSOR_INT1,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_23,   A55_GPIO_NS_23,  NO_SIGNAL	      },        /* Pin 23 */
	{ UART0_RXSIN,				      TE_UART_RXD,	     NO_SIGNAL,	    A55_GPIO_S_24,   A55_GPIO_NS_24,  NO_SIGNAL	      },        /* Pin 24 */
	{ UART0_RTSOUT,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_25,   A55_GPIO_NS_25,  NO_SIGNAL	      },        /* Pin 25 */
	{ UART0_TXSOUT,				      TE_UART_TXD,	     NO_SIGNAL,	    A55_GPIO_S_26,   A55_GPIO_NS_26,  NO_SIGNAL	      },        /* Pin 26 */
	{ UART0_CTSIN,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_27,   A55_GPIO_NS_27,  NO_SIGNAL	      },        /* Pin 27 */
	{ UART1_RXSIN,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_28,   A55_GPIO_NS_28,  NO_SIGNAL	      },        /* Pin 28 */
	{ UART1_TXSOUT,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_29,   A55_GPIO_NS_29,  NO_SIGNAL	      },        /* Pin 29 */
	{ UART2_RXSIN,				      UART4_RXSIN,	     PPI16_PDI_15,  A55_GPIO_S_30,   A55_GPIO_NS_30,  NO_SIGNAL	      },        /* Pin 30 */
	{ UART2_TXSOUT,				      UART4_TXSOUT,	     PPI16_CLK,	    A55_GPIO_S_31,   A55_GPIO_NS_31,  NO_SIGNAL	      },        /* Pin 31 */
	{ A55_GPIO_S_32,			      NO_SIGNAL,	     NO_SIGNAL,	    GPINT_OUTPUT_0,  A55_GPIO_NS_32,  NO_SIGNAL	      },        /* Pin 32 */
	{ A55_GPIO_S_33,			      UART1_RTSOUT,	     NO_SIGNAL,	    DEBUG_CLK_INPUT, A55_GPIO_NS_33,  RFFE_30	      },        /* Pin 33 */
	{ A55_GPIO_S_34,			      SPI_MASTER4_MISO,	     PPI16_PEB,	    JTAG_TDO_M4,     A55_GPIO_NS_34,  RFFE_31	      },        /* Pin 34 */
	{ A55_GPIO_S_35,			      SPI_MASTER4_MOSI,	     I2C2_SCL,	    JTAG_TDI_M4,     A55_GPIO_NS_35,  RFFE_32	      },        /* Pin 35 */
	{ A55_GPIO_S_36,			      SPI_MASTER4_CLK,	     I2C2_SDA,	    JTAG_TMS_M4,     A55_GPIO_NS_36,  RFFE_33	      },        /* Pin 36 */
	{ A55_GPIO_S_37,			      SPI_MASTER4_SELB,	     PWM_7,	    JTAG_TCK_M4,     A55_GPIO_NS_37,  RFFE_34	      },        /* Pin 37 */
	{ A55_GPIO_S_38,			      I2C7_SCL,		     PWM_8,	    JTAG_TRSTB_M4,   A55_GPIO_NS_38,  RFFE_35	      },        /* Pin 38 */
	{ A55_GPIO_S_39,			      I2C7_SDA,		     PWM_9,	    OTP_CLK_OUTPUT,  A55_GPIO_NS_39,  RFFE_36	      },        /* Pin 39 */
	{ A55_GPIO_S_40,			      UART3_RXSIN,	     PWM_10,	    GPINT_OUTPUT_1,  A55_GPIO_NS_40,  RFFE_37	      },        /* Pin 40 */
	{ NO_SIGNAL,				      UART3_TXSOUT,	     PWM_11,	    A55_GPIO_S_41,   A55_GPIO_NS_41,  RFFE_38	      },        /* Pin 41 */
	{ SPI_MASTER0_SELB_1,			      NO_SIGNAL,	     TRACE_D0,	    A55_GPIO_S_42,   A55_GPIO_NS_42,  NO_SIGNAL	      },        /* Pin 42 */
	{ SPI_MASTER0_SELB_2,			      UART2_RTSOUT,	     TRACE_D1,	    A55_GPIO_S_43,   A55_GPIO_NS_43,  RFFE_39	      },        /* Pin 43 */
	{ SPI_MASTER0_SELB_3,			      UART2_CTSIN,	     TRACE_D2,	    A55_GPIO_S_44,   A55_GPIO_NS_44,  SFP1_RS0	      },        /* Pin 44 */
	{ SPI_MASTER1_MISO,			      ONE_PPS_CLK_OUTPUT_SE, PPI16_PDI_10,  A55_GPIO_S_45,   A55_GPIO_NS_45,  SFP1_RS1	      },        /* Pin 45 */
	{ SPI_MASTER1_MOSI,			      PWM_12,		     TRACE_D3,	    A55_GPIO_S_46,   A55_GPIO_NS_46,  REBOOTB	      },        /* Pin 46 */
	{ SPI_MASTER1_CLK,			      PWM_13,		     TRACE_D4,	    A55_GPIO_S_47,   A55_GPIO_NS_47,  RFFE_40	      },        /* Pin 47 */
	{ SPI_MASTER1_SELB_0,			      PWM_14,		     TRACE_D5,	    A55_GPIO_S_48,   A55_GPIO_NS_48,  RFFE_41	      },        /* Pin 48 */
	{ SPI_MASTER1_SELB_1,			      PWM_15,		     TRACE_D6,	    A55_GPIO_S_49,   A55_GPIO_NS_49,  RFFE_42	      },        /* Pin 49 */
	{ SPI_MASTER1_SELB_2,			      UART4_RTSOUT,	     TRACE_D7,	    A55_GPIO_S_50,   A55_GPIO_NS_50,  RFFE_43	      },        /* Pin 50 */
	{ SPI_MASTER1_SELB_3,			      UART4_CTSIN,	     TRACE_CLK,	    A55_GPIO_S_51,   A55_GPIO_NS_51,  POWERDOWN	      },        /* Pin 51 */
	{ SPI_MASTER2_MISO,			      UART3_RTSOUT,	     QSFP_MODSEL_1, A55_GPIO_S_52,   A55_GPIO_NS_52,  SFP0_RS0	      },        /* Pin 52 */
	{ SPI_MASTER2_MOSI,			      UART3_CTSIN,	     QSFP_MODPRS_1, A55_GPIO_S_53,   A55_GPIO_NS_53,  SFP0_RS1	      },        /* Pin 53 */
	{ SPI_MASTER2_CLK,			      GNSS_INTERRUPT,	     NO_SIGNAL,	    A55_GPIO_S_54,   A55_GPIO_NS_54,  RFFE_44	      },        /* Pin 54 */
	{ SPI_MASTER2_SELB,			      RTC_INT,		     NO_SIGNAL,	    A55_GPIO_S_55,   A55_GPIO_NS_55,  RFFE_45	      },        /* Pin 55 */
	{ QSFP_RESET,				      SFP0_TX_DISABLE,	     NO_SIGNAL,	    A55_GPIO_S_56,   A55_GPIO_NS_56,  NO_SIGNAL	      },        /* Pin 56 */
	{ QSFP_INTERRUPT,			      SFP0_TXFAULT,	     NO_SIGNAL,	    A55_GPIO_S_57,   A55_GPIO_NS_57,  NO_SIGNAL	      },        /* Pin 57 */
	{ QSFP_MODSEL_0,			      SFP0_RX_LOS,	     NO_SIGNAL,	    A55_GPIO_S_58,   A55_GPIO_NS_58,  NO_SIGNAL	      },        /* Pin 58 */
	{ QSFP_MODPRS_0,			      SFP0_MOD_ABS,	     NO_SIGNAL,	    A55_GPIO_S_59,   A55_GPIO_NS_59,  NO_SIGNAL	      },        /* Pin 59 */
	{ A55_GPIO_S_60,			      NO_SIGNAL,	     NO_SIGNAL,	    JTAG_TDO_A55,    A55_GPIO_NS_60,  NO_SIGNAL	      },        /* Pin 60 */
	{ A55_GPIO_S_61,			      NO_SIGNAL,	     NO_SIGNAL,	    JTAG_TDI_A55,    A55_GPIO_NS_61,  NO_SIGNAL	      },        /* Pin 61 */
	{ A55_GPIO_S_62,			      NO_SIGNAL,	     NO_SIGNAL,	    JTAG_TMS_A55,    A55_GPIO_NS_62,  NO_SIGNAL	      },        /* Pin 62 */
	{ A55_GPIO_S_63,			      NO_SIGNAL,	     NO_SIGNAL,	    JTAG_TCK_A55,    A55_GPIO_NS_63,  NO_SIGNAL	      },        /* Pin 63 */
	{ A55_GPIO_S_64,			      NO_SIGNAL,	     NO_SIGNAL,	    JTAG_TRSTB_A55,  A55_GPIO_NS_64,  NO_SIGNAL	      },        /* Pin 64 */
	{ RFFE_46,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_65,   A55_GPIO_NS_65,  NO_SIGNAL	      },        /* Pin 65 */
	{ GPIO_DEBUG_0,				      SPI_MASTER5_MISO,	     RFFE_19,	    A55_GPIO_S_66,   A55_GPIO_NS_66,  NO_SIGNAL	      },        /* Pin 66 */
	{ GPIO_DEBUG_1,				      SPI_MASTER5_MOSI,	     RFFE_20,	    A55_GPIO_S_67,   A55_GPIO_NS_67,  NO_SIGNAL	      },        /* Pin 67 */
	{ GPIO_DEBUG_2,				      SPI_MASTER5_CLK,	     RFFE_21,	    A55_GPIO_S_68,   A55_GPIO_NS_68,  NO_SIGNAL	      },        /* Pin 68 */
	{ GPIO_DEBUG_3,				      SPI_MASTER5_SELB,	     RFFE_22,	    A55_GPIO_S_69,   A55_GPIO_NS_69,  NO_SIGNAL	      },        /* Pin 69 */
	{ GPIO_DEBUG_4,				      I2C4_SCL,		     RFFE_23,	    A55_GPIO_S_70,   A55_GPIO_NS_70,  SFP1_TX_DISABLE },        /* Pin 70 */
	{ GPIO_DEBUG_5,				      I2C4_SDA,		     RFFE_24,	    A55_GPIO_S_71,   A55_GPIO_NS_71,  SFP1_TXFAULT    },        /* Pin 71 */
	{ GPIO_DEBUG_6,				      I2C5_SCL,		     RFFE_25,	    A55_GPIO_S_72,   A55_GPIO_NS_72,  SFP1_RX_LOS     },        /* Pin 72 */
	{ GPIO_DEBUG_7,				      I2C5_SDA,		     RFFE_26,	    A55_GPIO_S_73,   A55_GPIO_NS_73,  SFP1_MOD_ABS    },        /* Pin 73 */
	{ A55_GPIO_S_74,			      I2C6_SCL,		     PPI16_PDI_3,   SPI_SLAVE_MISO,  A55_GPIO_NS_74,  LED_STATUS_0    },        /* Pin 74 */
	{ A55_GPIO_S_75,			      I2C6_SDA,		     RFFE_27,	    SPI_SLAVE_MOSI,  A55_GPIO_NS_75,  LED_STATUS_1    },        /* Pin 75 */
	{ A55_GPIO_S_76,			      QSPI_FLOW_READY,	     RFFE_28,	    SPI_SLAVE_CLK,   A55_GPIO_NS_76,  LED_STATUS_2    },        /* Pin 76 */
	{ A55_GPIO_S_77,			      UART1_CTSIN,	     RFFE_29,	    SPI_SLAVE_SELB,  A55_GPIO_NS_77,  LED_STATUS_3    },        /* Pin 77 */
	{ QSPI_FLASH_D0,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_78,   A55_GPIO_NS_78,  NO_SIGNAL	      },        /* Pin 78 */
	{ QSPI_FLASH_D1,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_79,   A55_GPIO_NS_79,  NO_SIGNAL	      },        /* Pin 79 */
	{ QSPI_FLASH_D2,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_80,   A55_GPIO_NS_80,  NO_SIGNAL	      },        /* Pin 80 */
	{ QSPI_FLASH_D3,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_81,   A55_GPIO_NS_81,  NO_SIGNAL	      },        /* Pin 81 */
	{ QSPI_FLASH_CLK,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_82,   A55_GPIO_NS_82,  NO_SIGNAL	      },        /* Pin 82 */
	{ QSPI_FLASH_SELB,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_83,   A55_GPIO_NS_83,  NO_SIGNAL	      },        /* Pin 83 */
	{ QSPI_FLASH_RESETN,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_84,   A55_GPIO_NS_84,  NO_SIGNAL	      },        /* Pin 84 */
	{ EMAC_GMII_MDIO,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_85,   A55_GPIO_NS_85,  NO_SIGNAL	      },        /* Pin 85 */
	{ EMAC_GMII_MDC,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_86,   A55_GPIO_NS_86,  NO_SIGNAL	      },        /* Pin 86 */
	{ EMAC_PHY_INTR,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_87,   A55_GPIO_NS_87,  NO_SIGNAL	      },        /* Pin 87 */
	{ EMAC_RESETN,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_88,   A55_GPIO_NS_88,  NO_SIGNAL	      },        /* Pin 88 */
	{ EMAC_PHY_RX_DV,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_89,   A55_GPIO_NS_89,  NO_SIGNAL	      },        /* Pin 89 */
	{ EMAC_CLK_TX,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_90,   A55_GPIO_NS_90,  NO_SIGNAL	      },        /* Pin 90 */
	{ EMAC_PHY_TXEN,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_91,   A55_GPIO_NS_91,  NO_SIGNAL	      },        /* Pin 91 */
	{ EMAC_PHY_TXD_3,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_92,   A55_GPIO_NS_92,  NO_SIGNAL	      },        /* Pin 92 */
	{ EMAC_PHY_TXD_2,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_93,   A55_GPIO_NS_93,  NO_SIGNAL	      },        /* Pin 93 */
	{ EMAC_PHY_TXD_1,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_94,   A55_GPIO_NS_94,  NO_SIGNAL	      },        /* Pin 94 */
	{ EMAC_PHY_TXD_0,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_95,   A55_GPIO_NS_95,  NO_SIGNAL	      },        /* Pin 95 */
	{ EMAC_CLK_RX,				      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_96,   A55_GPIO_NS_96,  NO_SIGNAL	      },        /* Pin 96 */
	{ EMAC_PHY_RXD_3,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_97,   A55_GPIO_NS_97,  NO_SIGNAL	      },        /* Pin 97 */
	{ EMAC_PHY_RXD_2,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_98,   A55_GPIO_NS_98,  NO_SIGNAL	      },        /* Pin 98 */
	{ EMAC_PHY_RXD_1,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_99,   A55_GPIO_NS_99,  NO_SIGNAL	      },        /* Pin 99 */
	{ EMAC_PHY_RXD_0,			      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_100,  A55_GPIO_NS_100, NO_SIGNAL	      },        /* Pin 100 */
	{ GPINT_INTERRUPT_INPUT_SECONDARY_TO_PRIMARY, NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_101,  A55_GPIO_NS_101, NO_SIGNAL	      },        /* Pin 101 */
	{ POWERREBOOTREGULATOR_ENABLE_PULLDOWN,	      NO_SIGNAL,	     NO_SIGNAL,	    A55_GPIO_S_102,  A55_GPIO_NS_102, NO_SIGNAL	      },        /* Pin 102 */
};
