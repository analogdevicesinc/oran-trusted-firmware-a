/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <adrv906x_pinctrl.h>
#include <adrv906x_pinmux_source_def.h>
#include <plat_pinctrl.h>

#define ADI_ADRV906X_UNUSED_CONFIG        0U

/** The following defines the possible ADRV906X source mux selections*/
#define ADI_PINMUX_SRC_SEL_0    0U
#define ADI_PINMUX_SRC_SEL_1    1U
#define ADI_PINMUX_SRC_SEL_2    2U
#define ADI_PINMUX_SRC_SEL_3    3U
#define ADI_PINMUX_SRC_SEL_4    4U
#define ADI_PINMUX_SRC_NONE             0xFFFFFFFFU

/*
 * QSPI PINCTRL GROUP, A55_GPIO_S_84 used for flash reset
 */
const plat_pinctrl_settings qspi_pin_grp[] = {
	/*     pin#                  SRCMUX                   DS           ST     P_EN   PU        extendedOptions*/
	{ QSPI_FLASH_CLK_PIN,  QSPI_FLASH_CLK_MUX_SEL,	CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ QSPI_FLASH_D0_PIN,   QSPI_FLASH_D0_MUX_SEL,	CMOS_PAD_DS_0100, true,	 false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ QSPI_FLASH_D1_PIN,   QSPI_FLASH_D1_MUX_SEL,	CMOS_PAD_DS_0100, true,	 false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ QSPI_FLASH_D2_PIN,   QSPI_FLASH_D2_MUX_SEL,	CMOS_PAD_DS_0100, true,	 false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ QSPI_FLASH_D3_PIN,   QSPI_FLASH_D3_MUX_SEL,	CMOS_PAD_DS_0100, true,	 false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ QSPI_FLASH_SELB_PIN, QSPI_FLASH_SELB_MUX_SEL, CMOS_PAD_DS_0100, false, true,	true,  ADI_ADRV906X_UNUSED_CONFIG },
	{ A55_GPIO_S_84_PIN,   A55_GPIO_S_84_MUX_SEL,	CMOS_PAD_DS_0100, false, true,	true,  ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t qspi_pin_grp_members = sizeof(qspi_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * SPI MASTER0 PINCTRL GROUP, used with external sysref ZL30732 on CS0
 */
const plat_pinctrl_settings spimaster0_pin_grp[] = {
	/*pin#                        SRCMUX              DS                ST     P_EN   PU     extendedOptions*/
	{ SPI_MASTER0_MISO_DIO_PIN,   0 /*Dedicated IO*/, CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SPI_MASTER0_MOSI_DIO_PIN,   0 /*Dedicated IO*/, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SPI_MASTER0_CLK_DIO_PIN,    0 /*Dedicated IO*/, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SPI_MASTER0_SELB_0_DIO_PIN, 0 /*Dedicated IO*/, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t spimaster0_pin_grp_members = sizeof(spimaster0_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * UART0 PINCTRL GROUP
 */
const plat_pinctrl_settings uart0_pin_grp[] = {
	/*	pin#				SRCMUX						DS					ST		P_EN	PU		extendedOptions*/
	{ UART0_RXSIN_PIN,  UART0_RXSIN_MUX_SEL,  CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ UART0_TXSOUT_PIN, UART0_TXSOUT_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t uart0_pin_grp_members = sizeof(uart0_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * UART1 PINCTRL GROUP
 */
const plat_pinctrl_settings uart1_pin_grp[] = {
	/*	pin#				SRCMUX						DS					ST		P_EN	PU		extendedOptions*/
	{ UART1_RXSIN_PIN,  UART1_RXSIN_MUX_SEL,  CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ UART1_TXSOUT_PIN, UART1_TXSOUT_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t uart1_pin_grp_members = sizeof(uart1_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * SD PINCTRL GROUP
 */
const plat_pinctrl_settings sd_pin_grp[] = {
	/*	pin#				SRCMUX						DS					ST		P_EN	PU		extendedOptions*/
	{ SD_CLK_SEL_PIN,    SD_CLK_SEL_MUX_SEL,    CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SD_CMD_PIN,	     SD_CMD_MUX_SEL,	    CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SD_DATA0_PIN,	     SD_DATA0_MUX_SEL,	    CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SD_DATA1_PIN,	     SD_DATA1_MUX_SEL,	    CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SD_DATA2_PIN,	     SD_DATA2_MUX_SEL,	    CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SD_DATA3_PIN,	     SD_DATA3_MUX_SEL,	    CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ SD_CARDDETECT_PIN, SD_CARDDETECT_MUX_SEL, CMOS_PAD_DS_0100, true,  false, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t sd_pin_grp_members = sizeof(sd_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * GPINT0 PINCTRL GROUP
 */
const plat_pinctrl_settings gpint0_pin_grp[] = {
	/*	pin#			SRCMUX				DS					ST		P_EN	PU		extendedOptions*/
	{ GPINT_OUTPUT_0_PIN, GPINT_OUTPUT_0_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t gpint0_pin_grp_members = sizeof(gpint0_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * SECONDARY TO PRIMARY PINCTRL GROUP
 */
const plat_pinctrl_settings secondary_to_primary_pin_grp[] = {
	/*	pin#			SRCMUX				DS					ST		P_EN	PU		extendedOptions*/
	{ GPINT_INTERRUPT_INPUT_SECONDARY_TO_PRIMARY_PIN, GPINT_INTERRUPT_INPUT_SECONDARY_TO_PRIMARY_MUX_SEL, CMOS_PAD_DS_0100, true, true, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t secondary_to_primary_pin_grp_members = sizeof(secondary_to_primary_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * DEBUG_GPIOS PINCTRL GROUP
 */
const plat_pinctrl_settings debug_gpios_pin_grp[] = {
	/*	pin#			SRCMUX				DS					ST		P_EN	PU		extendedOptions*/
	{ GPIO_DEBUG_0_PIN, GPIO_DEBUG_0_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_1_PIN, GPIO_DEBUG_1_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_2_PIN, GPIO_DEBUG_2_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_3_PIN, GPIO_DEBUG_3_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_4_PIN, GPIO_DEBUG_4_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_5_PIN, GPIO_DEBUG_5_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_6_PIN, GPIO_DEBUG_6_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG },
	{ GPIO_DEBUG_7_PIN, GPIO_DEBUG_7_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, ADI_ADRV906X_UNUSED_CONFIG }
};
const size_t debug_gpios_pin_grp_members = sizeof(debug_gpios_pin_grp) / sizeof(plat_pinctrl_settings);
