/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <common/debug.h>
#include <drivers/adi/adi_qspi.h>
#include <drivers/adi/adi_sdhci.h>
#include <drivers/adi/adi_spu.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/systemc_pl180_mmc.h>
#include <drivers/mmc.h>
#include <drivers/spi_mem.h>
#include <lib/mmio.h>
#include <lib/utils.h>

#include <adrv906x_board.h>
#include <adrv906x_boot.h>
#include <adrv906x_clkrst_def.h>
#include <adrv906x_spu_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_pinmux_source_def.h>
#include <platform_def.h>
#include <plat_boot.h>
#include <plat_pinctrl.h>
#include <plat_setup.h>

#define BOOT_MODE_QSPI          (0)
#define BOOT_MODE_EMMC          (1)
#define BOOT_MODE_SD            (2)
#define BOOT_MODE_HOST          (3)
#define BOOT_MODE_NUM           (4)

static bool is_boot_dev_init = false;

static void init_sysc_mmc(uintptr_t base)
{
	struct mmc_device_info mmc_info;
	struct systemc_pl180_params params;

	/* Technically the MMC model is an SD card, but setting this
	 * to MMC_IS_SD generates a bunch of errors in the driver, none
	 * of which are worth debugging for what we're using this for.
	 * Set to MMC_IS_EMMC to quiet those errors.
	 */
	mmc_info.mmc_dev_type = MMC_IS_EMMC;
	zeromem(&params, sizeof(struct systemc_pl180_params));
	params.reg_base = base;
	params.bus_width = MMC_BUS_WIDTH_1;
	params.max_freq = 400000;
	params.clk_rate = 400000;
	params.device_info = &mmc_info;
	systemc_pl180_mmc_init(&params);
}

static void init_mmc(enum mmc_device_type device)
{
	struct mmc_device_info mmc_info;
	struct adi_mmc_params mmc_params;
	extern const plat_pinctrl_settings sd_pin_grp[];
	extern const size_t sd_pin_grp_members;

	zeromem(&mmc_params, sizeof(struct adi_mmc_params));
	mmc_info.mmc_dev_type = device;

	if (MMC_IS_EMMC == device) {
		mmc_params.reg_base = EMMC_0_BASE;
		mmc_params.clk_rate = EMMC_0_CLK_RATE_HZ;
		mmc_params.bus_width = EMMC_0_BUS_WIDTH;
		/* eMMC interface of dwc_mshc includes a PHY solution */
		mmc_params.phy_config_needed = true;
		mmc_params.phy_reg_base = EMMC_0_PHY_BASE;
	} else { /* MMC_IS_SD */
		mmc_params.reg_base = SD_0_BASE;
		mmc_params.clk_rate = SD_0_CLK_RATE_HZ;
		mmc_params.bus_width = SD_0_BUS_WIDTH;
		/* SD interface of dwc_mshc does not include PHY solution */
		mmc_params.phy_config_needed = false;
		/* As per Spec, Host System should set Voltage support to 3.3V
		 * or 3.0V for SD card. Adrv906x drives 1.8V and level shifter
		 * in the board converts to 3.3V */
		mmc_info.ocr_voltage = OCR_3_3_3_4 | OCR_3_2_3_3;
	}

	mmc_params.src_clk_hz = clk_get_freq(CLK_CTL, CLK_ID_EMMC);
	mmc_params.device_info = &mmc_info;
	mmc_params.use_dma = true;
	mmc_params.flags = MMC_FLAG_CMD23;

	/* If the interface is SD, configure the pinmux to enable these pins*/
	if (device != MMC_IS_EMMC)
		plat_secure_pinctrl_set_group(sd_pin_grp, sd_pin_grp_members, true, PINCTRL_BASE);

	(void)adi_mmc_init(&mmc_params);
}

static void init_qspi(void)
{
	struct adi_qspi_ctrl qspi_params;
	extern const plat_pinctrl_settings qspi_pin_grp[];
	extern const size_t qspi_pin_grp_members;

	plat_secure_pinctrl_set_group(qspi_pin_grp, qspi_pin_grp_members, true, PINCTRL_BASE);

	/* Reset the flash chip before use */
	plat_do_spi_nor_reset();

	qspi_params.reg_base = QSPI_0_BASE;
	qspi_params.tx_dde_reg_base = QSPI_0_TX_DDE_BASE;
	qspi_params.rx_dde_reg_base = QSPI_0_RX_DDE_BASE;
	qspi_params.clock_freq = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
	qspi_params.mode = SPI_TX_QUAD | SPI_RX_QUAD; /* Asking for QUAD support. Clock phase, polarity and LSBF to 0 */
	qspi_params.cs = QSPI_FLASH_CHIP_SELECT;
	qspi_params.spi_clk_freq = QSPI_CLK_FREQ_HZ;
	qspi_params.dma = true;

	adi_qspi_init(&qspi_params);
}

bool plat_get_test_enable(void)
{
	return (mmio_read_32(A55_SYS_CFG + BOOT_INFO) & TEST_ENABLE_MASK) >> TEST_ENABLE_SHIFT;
}

int plat_get_test_control(void)
{
	return (mmio_read_32(A55_SYS_CFG + BOOT_INFO) & TEST_CONTROL_MASK) >> TEST_CONTROL_SHIFT;
}

plat_boot_device_t plat_get_boot_device(void)
{
	uint32_t boot_mode;

	/* Only the bottom two bits of the Boot Mode section of the BOOT_INFO register are used for the boot device.
	 *  Shifting the mask so that the TE bootrom bypass pin (or the other pin) doesn't cause the boot_mode to always be invalid if it is active.
	 */
	boot_mode = (mmio_read_32(A55_SYS_CFG + BOOT_INFO) & (BOOT_MODE_MASK >> 2));

	if (BOOT_MODE_QSPI == boot_mode)
		return PLAT_BOOT_DEVICE_QSPI_0;
	else if (BOOT_MODE_EMMC == boot_mode)
		return PLAT_BOOT_DEVICE_EMMC_0;
	else if (BOOT_MODE_SD == boot_mode)
		return PLAT_BOOT_DEVICE_SD_0;
	else if (BOOT_MODE_HOST == boot_mode)
		return PLAT_BOOT_DEVICE_HOST;
	else
		return PLAT_BOOT_DEVICE_NUM;
}

void plat_init_boot_device(void)
{
	plat_boot_device_t boot_dev = plat_get_boot_device();

	switch (boot_dev) {
	case PLAT_BOOT_DEVICE_SD_0:
		// BL1 and BL2 use DMA on the boot device, so MSEC must be enabled.
		// this permission gets removed during BL31 setup.
		adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_EMMC1SLV);

		if (plat_is_sysc() == true)
			init_sysc_mmc(ADRV906X_SYSC_SD_BASE);
		else
			init_mmc(MMC_IS_SD);
		break;
	case PLAT_BOOT_DEVICE_EMMC_0:
		adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_EMMC0SLV);

		if (plat_is_sysc() == true)
			init_sysc_mmc(ADRV906X_SYSC_EMMC_BASE);
		else
			init_mmc(MMC_IS_EMMC);
		break;
	case PLAT_BOOT_DEVICE_QSPI_0:
		adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_QUAD_SPI_DMA_0);
		adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_QUAD_SPI_DMA_1);

		init_qspi();
		break;
	default:
		break;
	}

	is_boot_dev_init = true;
}

void plat_deinit_boot_device(void)
{
	plat_boot_device_t boot_device = plat_get_boot_device();

	if ((boot_device == PLAT_BOOT_DEVICE_EMMC_0) && (plat_is_sysc() == false))
		adi_mmc_deinit(EMMC_0_BASE);
	else if ((boot_device == PLAT_BOOT_DEVICE_SD_0) && (plat_is_sysc() == false))
		adi_mmc_deinit(SD_0_BASE);
	else if (boot_device == PLAT_BOOT_DEVICE_QSPI_0)
		adi_qspi_deinit(QSPI_0_BASE, QSPI_0_TX_DDE_BASE, QSPI_0_RX_DDE_BASE);

	is_boot_dev_init = false;
}

uintptr_t plat_get_host_boot_addr(void)
{
	return NS_SRAM_BASE;
}

bool plat_is_boot_dev_initialized(void)
{
	return is_boot_dev_init;
}

boot_info_clk_sel_t plat_get_boot_clk_sel(void)
{
	uint32_t clk_sel;

	clk_sel = mmio_read_32(A55_SYS_CFG + BOOT_INFO) & CLK_SEL_MASK;

	if (clk_sel == 0)
		return CLK_SEL_DEV_CLK;
	else
		return CLK_SEL_ROSC_CLK;
}

bool plat_is_devclk_available(void)
{
	uint32_t devclk_good;

	devclk_good = mmio_read_32(A55_SYS_CFG + BOOT_INFO) & DEVCLK_GOOD_MASK;

	if (devclk_good == DEVCLK_GOOD_MASK)
		return true;
	else
		return false;
}
