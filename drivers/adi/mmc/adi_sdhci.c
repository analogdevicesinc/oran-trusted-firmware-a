/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/mmc.h>
#include <drivers/adi/adi_sdhci.h>
#include <drivers/adi/adi_sdhci_phy.h>
#include <lib/mmio.h>
#include <lib/utils.h>

#include "adi_sdhci_hw.h"

/* Macro to denote maximum clock frequency in SDHCI legacy mode */
#define SDHCI_LEGACY_MODE_MAX_FREQ_HZ         (26U * 1000U * 1000U)

/* Macros to denote various timeout values */
#define SDHCI_CARD_DETECT_TIMEOUT_US_1_MS     (1000U)
#define SDHCI_STOP_CLK_TIMEOUT_US_20_MS       (20000U)
#define SDHCI_CMD_TIMEOUT_US_100_MS           (100000U)
#define SDHCI_CLK_STABLE_TIMEOUT_US_150_MS    (150000U)
#define SDHCI_DATA_XFER_TIMEOUT_US_1_S        (1000000U)

/* Standard SDHCI Commands */
/* eMMC Commands */
#define SDHCI_CMD_GO_IDLE_STATE               MMC_CMD(0)
#define SDHCI_CMD_SELECT_CARD                 MMC_CMD(7)
#define SDHCI_CMD_SEND_EXT_CSD                MMC_CMD(8)
#define SDHCI_CMD_SEND_CSD                    MMC_CMD(9)
#define SDHCI_CMD_STOP_TRANSMISSION           MMC_CMD(12)
#define SDHCI_CMD_READ_SINGLE_BLOCK           MMC_CMD(17)
#define SDHCI_CMD_READ_MULTIPLE_BLOCK         MMC_CMD(18)
#define SDHCI_CMD_WRITE_SINGLE_BLOCK          MMC_CMD(24)
#define SDHCI_CMD_WRITE_MULTIPLE_BLOCK        MMC_CMD(25)
/* SD Commands */
#define SDHCI_CMD_SEND_SCR                    MMC_ACMD(51)

/* Macro to form SDHCI Command */
#define SDHCI_MAKE_CMD(idx, flags) (((idx & 0x3FU) << 8U) | (flags & 0xFFU))

/* Macros that can be passed as arguments to adi_sdhci_control_card_clk() */
#define SDHCI_STOP_CARD_CLK                   (0U)
#define SDHCI_SUPPLY_CARD_CLK                 (1U)

/* Macros that can be passed to 'dir' argument of adi_sdhci_non_dma_xfer() */
#define SDHCI_DATA_XFER_WRITE                 (0U)
#define SDHCI_DATA_XFER_READ                  (1U)

/* Host SDMA Buffer Boundary - 512K */
#define SDHCI_SDMA_BOUNDARY_SIZE              (512U * 1024U)

/****************** Internal Function Prototypes ******************/
static bool adi_sdhci_use_dma_for_data_xfer(void);
static enum mmc_device_type adi_sdhci_get_dev_type(void);
static int adi_sdhci_set_clk(const uint32_t clk_freq_hz);
static bool adi_sdhci_is_data_xfr_cmd(const unsigned int cmd_idx);
static int adi_sdhci_control_card_clk(const uint8_t clk_ctl);
static int adi_sdhci_change_clk_freq(const uint32_t clk_freq_hz);
static int adi_sdhci_sw_reset(const uint8_t mask);
static int adi_sdhci_set_bus_width(const unsigned int width);
static void adi_sdhci_initialize_host_controller(void);
static int adi_sdhci_non_dma_xfer(int lba, uintptr_t buf, const size_t size, const bool dir);
static int adi_sdhci_dma_xfer(uintptr_t buf);
static int adi_sdhci_handle_bus_errors(void);

/***************** External Function Prototypes ******************/
static void adi_sdhci_init(void);
static int adi_sdhci_send_cmd(struct mmc_cmd *cmd);
static int adi_sdhci_set_ios(unsigned int clk, unsigned int width);
static int adi_sdhci_prepare(int lba, uintptr_t buf, size_t size);
static int adi_sdhci_read(int lba, uintptr_t buf, size_t size);
static int adi_sdhci_write(int lba, uintptr_t buf, size_t size);

/*********************** Global Variables *************************/
static const struct mmc_ops adi_sdhci_ops = {
	.init		= adi_sdhci_init,
	.send_cmd	= adi_sdhci_send_cmd,
	.set_ios	= adi_sdhci_set_ios,
	.prepare	= adi_sdhci_prepare,
	.read		= adi_sdhci_read,
	.write		= adi_sdhci_write,
};

static struct adi_mmc_params adi_sdhci_params;

static bool abort_cmd_issued = false;

static bool card_initialized = false;

static bool use_dma_mode = false;

static struct mmc_device_info sdhci_dev_info;

/****************** Internal Function Definitions ******************/
static bool adi_sdhci_use_dma_for_data_xfer(void)
{
	return use_dma_mode;
}

static enum mmc_device_type adi_sdhci_get_dev_type(void)
{
	return adi_sdhci_params.device_info->mmc_dev_type;
}

static bool adi_sdhci_is_data_xfr_cmd(const unsigned int cmd_idx)
{
	bool ret = false;
	enum mmc_device_type dev = adi_sdhci_get_dev_type();

	if ((SDHCI_CMD_READ_SINGLE_BLOCK == cmd_idx) ||
	    (SDHCI_CMD_READ_MULTIPLE_BLOCK == cmd_idx) ||
	    (SDHCI_CMD_WRITE_SINGLE_BLOCK == cmd_idx) ||
	    (SDHCI_CMD_WRITE_MULTIPLE_BLOCK == cmd_idx)) {
		ret = true;
	} else if (SDHCI_CMD_SEND_EXT_CSD == cmd_idx) {
		if (MMC_IS_EMMC == dev)
			ret = true;
	} else if (SDHCI_CMD_SEND_SCR == cmd_idx) {
		if (dev != MMC_IS_EMMC) /* dev can be MMC_IS_SD or MMC_IS_SD_HC */
			ret = true;
	} else {
		/* Do Noting. ret holds false */
	}

	return ret;
}

static int adi_sdhci_sw_reset(const uint8_t mask)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;

	/* Trigger the respective reset */
	mmio_write_8(base + SDHCI_SW_RST_R_OFF, mask);

	/* Wait max 100ms for the reset to complete */
	timeout = timeout_init_us(SDHCI_CMD_TIMEOUT_US_100_MS);
	while (mmio_read_8(base + SDHCI_SW_RST_R_OFF) & mask) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: Reset 0x%x never completed.\n",
			      __func__, (int)mask);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int adi_sdhci_control_card_clk(const uint8_t clk_ctl)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;
	uint16_t u16_reg_data;

	if (SDHCI_SUPPLY_CARD_CLK == clk_ctl) {
		u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) | SD_CLK_EN_BM);
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);
	} else if (SDHCI_STOP_CARD_CLK == clk_ctl) {
		/* Wait for the host controller to become idle before stopping card clock.
		 * Wait max 20ms */
		timeout = timeout_init_us(SDHCI_STOP_CLK_TIMEOUT_US_20_MS);

		while (mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) &
		       (CMD_INHIBIT_BM | CMD_INHIBIT_DAT_BM)) {
			if (timeout_elapsed(timeout)) {
				ERROR("%s: Failed to stop card clock. CMD/DAT line busy.\n",
				      __func__);
				return -ETIMEDOUT;
			}
		}

		/* Stop card clock */
		u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~SD_CLK_EN_BM);
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);
	} else {
		return -EINVAL;
	}
	return 0;
}

static int adi_sdhci_set_clk(const uint32_t clk_freq_hz)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;
	uint32_t divisor;
	uint16_t u16_reg_data;

	if ((0U == clk_freq_hz) ||
	    (0U == adi_sdhci_params.src_clk_hz) ||
	    (clk_freq_hz > SDHCI_LEGACY_MODE_MAX_FREQ_HZ))
		return -EINVAL;

	/* Calculate divisor for SD Clock Frequency.
	 * divisor = (hsdigclk_freq / card_clk) - 1
	 * eMMC is sourced from hsdig_clk
	 * CAPABILITIES1_R.BASE_CLK_FREQ is not used in design */
	divisor = (adi_sdhci_params.src_clk_hz / clk_freq_hz) - 1U;

	/* When card clock doesn't evenly divide src clk, increment
	 * divider by 1 to get it under target frequency */
	if ((adi_sdhci_params.src_clk_hz % clk_freq_hz) != 0U)
		++divisor;

	/* Selecting divided clock mode for Operation */
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~CLK_GEN_SELECT_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	/* Configuring divider value */
	/* Clearing FREQ_SEL field before setting it */
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~FREQ_SEL_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) |
			((divisor & LOWER_FREQ_SEL) << FREQ_SEL_POS));
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~UPPER_FREQ_SEL_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) |
			(((divisor & UPPER_FREQ_SEL) >> 8U) << UPPER_FREQ_SEL_POS));
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	/* Enable the internal clock signal */
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) | INTERNAL_CLK_EN_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	/* Wait for Clock Synchronization. On timeout of 150ms, return */
	timeout = timeout_init_us(SDHCI_CLK_STABLE_TIMEOUT_US_150_MS);

	while (0U == (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & INTERNAL_CLK_STABLE_BM)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: Internal clock never stabilised.\n",
			      __func__);
			return -ETIMEDOUT;
		}
	}

	/* Activate the PLL */
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) | PLL_ENABLE_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	/* Wait for Clock Synchronization. On timeout of 150ms, return */
	timeout = timeout_init_us(SDHCI_CLK_STABLE_TIMEOUT_US_150_MS);

	while (0U == (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & INTERNAL_CLK_STABLE_BM)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: Internal clock never stabilised after enabling PLL.\n",
			      __func__);
			return -ETIMEDOUT;
		}
	}

	VERBOSE("%s: clk divisor=0x%x\n", __func__, divisor);

	return 0;
}

static int adi_sdhci_change_clk_freq(const uint32_t clk_freq_hz)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;
	uint32_t divisor;
	uint16_t u16_reg_data;
	int err;

	if ((clk_freq_hz > SDHCI_LEGACY_MODE_MAX_FREQ_HZ) ||
	    (0U == adi_sdhci_params.src_clk_hz))
		return -EINVAL;

	/* Stop the Clock to Card before changing the clock frequency. */
	err = adi_sdhci_control_card_clk(SDHCI_STOP_CARD_CLK);
	if (err != 0)
		return err;

	if (0U == clk_freq_hz) {
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, 0U);
		return 0;
	}

	/* Deactivate the PLL */
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~PLL_ENABLE_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	/* If Preset Value Enable is set to 0, host driver changes clock parameters. Else, preset
	 * clock is selected according to bus speed mode */
	if (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) & PRESET_VAL_ENABLE_BM) {
		/* Select UHS Speed mode. Value of 0 means Legacy mode */
		u16_reg_data = (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) & ~UHS_MODE_SEL_BM);
		mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);
	} else {
		/* Calculate divisor for SD Clock Frequency.
		 * divisor = (hsdigclk_freq / card_clk) - 1
		 * eMMC is sourced from hsdig_clk
		 * CAPABILITIES1_R.BASE_CLK_FREQ is not used in design */
		divisor = (adi_sdhci_params.src_clk_hz / clk_freq_hz) - 1U;

		/* When card clock doesn't evenly divide src clk, increment
		 *      divider by 1 to get it under target frequency */
		if ((adi_sdhci_params.src_clk_hz % clk_freq_hz) != 0U)
			++divisor;

		/* Configure new divider value */
		/* Clearing FREQ_SEL field before setting it */
		u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~FREQ_SEL_BM);
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);
		u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) |
				((divisor & LOWER_FREQ_SEL) << FREQ_SEL_POS));
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

		u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & ~UPPER_FREQ_SEL_BM);
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);
		u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) |
				(((divisor & UPPER_FREQ_SEL) >> 8U) << UPPER_FREQ_SEL_POS));
		mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

		VERBOSE("%s: clk divisor=0x%x\n", __func__, divisor);
	}

	/* Activate the PLL */
	u16_reg_data = (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) | PLL_ENABLE_BM);
	mmio_write_16(base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	/* Wait for Clock Synchronization. On timeout of 150ms, return */
	timeout = timeout_init_us(SDHCI_CLK_STABLE_TIMEOUT_US_150_MS);

	while (0U == (mmio_read_16(base + SDHCI_CLK_CTRL_R_OFF) & INTERNAL_CLK_STABLE_BM)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: Internal clock never stabilised.\n",
			      __func__);
			return -ETIMEDOUT;
		}
	}

	/* Reset the Data and Command lines to avoid the effect of any glitch on sampling clock. */
	err = adi_sdhci_sw_reset(SW_RST_CMD_BM);
	if (err != 0)
		return err;
	err = adi_sdhci_sw_reset(SW_RST_DAT_BM);
	if (err != 0)
		return err;

	/* Enable the clock to card after clock frequency is changed */
	err = adi_sdhci_control_card_clk(SDHCI_SUPPLY_CARD_CLK);
	if (err != 0)
		return err;

	return 0;
}

static int adi_sdhci_set_bus_width(const unsigned int width)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint8_t host_ctrl1_r = mmio_read_8(base + SDHCI_HOST_CTRL1_R_OFF);
	int err = 0;

	/* Set Data Transfer Width and Extended Data Transfer Width as per the input */
	switch (width) {
	case MMC_BUS_WIDTH_1:
		host_ctrl1_r &= ~EXT_DAT_XFER_BM;
		host_ctrl1_r &= ~DAT_XFER_WIDTH_BM;
		break;
	case MMC_BUS_WIDTH_4:
	case MMC_BUS_WIDTH_DDR_4:
		host_ctrl1_r &= ~EXT_DAT_XFER_BM;
		host_ctrl1_r |= DAT_XFER_WIDTH_BM;
		break;
	case MMC_BUS_WIDTH_8:
	case MMC_BUS_WIDTH_DDR_8:
		host_ctrl1_r |= EXT_DAT_XFER_BM;
		break;
	default:
		ERROR("%s: width %d not supported.\n",
		      __func__, width);
		err = -EOPNOTSUPP;
		break;
	}

	/* Configure the bus width in case of a valid argument */
	if (0 == err)
		mmio_write_8(base + SDHCI_HOST_CTRL1_R_OFF, host_ctrl1_r);

	VERBOSE("%s: bus width=0x%x\n", __func__, width);

	return err;
}

static void adi_sdhci_initialize_host_controller(void)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint32_t u32_reg_data;
	uint16_t u16_reg_data;
	uint8_t u8_reg_data;
	enum mmc_device_type dev_type = adi_sdhci_get_dev_type();

	/* Reset and clear interrupt registers to default */
	mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, NORMAL_INT_STAT_MASK);
	mmio_write_16(base + SDHCI_ERROR_INT_STAT_R_OFF, ERROR_INT_STAT_MASK);
	mmio_write_16(base + SDHCI_NORMAL_INT_STAT_EN_R_OFF, 0x00U);
	mmio_write_16(base + SDHCI_ERROR_INT_STAT_EN_R_OFF, 0x00U);
	mmio_write_16(base + SDHCI_NORMAL_INT_SIGNAL_EN_R_OFF, 0x00U);
	mmio_write_16(base + SDHCI_ERROR_INT_SIGNAL_EN_R_OFF, 0x00U);

	/* Host Controller Setup Sequence for eMMC/SD Interface */
	/* Setting up the common parameters for all versions */
	/* MMC Bus Voltage select set to VDD(1.8V) */
	u8_reg_data = (mmio_read_8(base + SDHCI_PWR_CTRL_R_OFF) & ~SD_BUS_VOL_VDD1_BM);
	mmio_write_8(base + SDHCI_PWR_CTRL_R_OFF, u8_reg_data);
	u8_reg_data = (mmio_read_8(base + SDHCI_PWR_CTRL_R_OFF) | SD_BUS_VOL_VDD1_1V8);
	/* Enable VDD1 power of the card */
	u8_reg_data |= SD_BUS_PWR_VDD1_BM;
	mmio_write_8(base + SDHCI_PWR_CTRL_R_OFF, u8_reg_data);

	/* Data Timeout Counter Value */
	u8_reg_data = (mmio_read_8(base + SDHCI_TOUT_CTRL_R_OFF) & ~TOUT_CNT_BM);
	mmio_write_8(base + SDHCI_TOUT_CTRL_R_OFF, u8_reg_data);
	u8_reg_data = (mmio_read_8(base + SDHCI_TOUT_CTRL_R_OFF) | TOUT_CNT_VALUE14);
	mmio_write_8(base + SDHCI_TOUT_CTRL_R_OFF, u8_reg_data);

	/* Enable eMMC/SD Interface */
	u16_reg_data = (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) & ~UHS2_IF_ENABLE_BM);
	mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);

	if (MMC_IS_EMMC == dev_type) {
		/* Card connected to Controller is an eMMC card */
		u16_reg_data = (mmio_read_16(base + SDHCI_EMMC_CTRL_R_OFF) | CARD_IS_EMMC_BM);
		mmio_write_16(base + SDHCI_EMMC_CTRL_R_OFF, u16_reg_data);

		/* ADI specific: potential glitch on RX clock (PHY DL2 output)
		 * on initialization (because PHY DL2 input is connected to
		 * PHY DL1 output (tx clock))
		 * This configuration helps to fix this issue
		 */
		u32_reg_data = (POST_CHANGE_DLY_LESS_4_CYCLES << POST_CHANGE_DLY_OFF) |
			       (TUNE_CLK_STOP_EN << TUNE_CLK_STOP_EN_OFF);
		mmio_write_32(base + SDHCI_AT_CTRL_R_OFF, u32_reg_data);
	}

	/* Setting up Host Version 4 Parameters */
	/* Host Version 4 Enable */
	u16_reg_data = (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) | HOST_VER4_ENABLE_BM);
	mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);

	/* Select System Addressing of V4 Mode */
	if (SYS_ADDR_64_V4_BM == (mmio_read_32(base + SDHCI_CAPABILITIES1_R_OFF) & SYS_ADDR_64_V4_BM)) {
		u16_reg_data = (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) | ADDRESSING_BM);
		mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);
	}

	/* For SD interface, enable Asynchronous interrupt if the Asynchronous
	 * Interrupt Support is set to 1 in Capabilities register */
	if ((MMC_IS_SD == dev_type) && ((mmio_read_32(base + SDHCI_CAPABILITIES1_R_OFF) & ASYNC_INT_SUPPORT_BM))) {
		u16_reg_data = (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) | ASYNC_INT_ENABLE_BM);
		mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);
	}

	/* Set 1.8V signalling */
	u16_reg_data = mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) | SIGNALING_EN;
	mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);

	/* Enable the default normal interrupts */
	u16_reg_data = (CMD_COMPLETE_BM | XFER_COMPLETE_BM |
			BUF_RD_READY_STAT_EN_BM | BUF_WR_READY_STAT_EN_BM | DMA_INTERRUPT_BM);
	mmio_write_16(base + SDHCI_NORMAL_INT_STAT_EN_R_OFF, u16_reg_data);

	/* Enable the default error interrupts */
	u16_reg_data = (CMD_LINE_ERR_INTR_BM | DATA_LINE_ERR_INTR_BM | ADMA_ERROR_STATUS_EN_BM);
	mmio_write_16(base + SDHCI_ERROR_INT_STAT_EN_R_OFF, u16_reg_data);

	/* Configure bus clock frequency and enable internal clocks */
	/* Set boot frequency to 400KHz as suggested by Synopsys */
	(void)adi_sdhci_set_clk(MMC_BOOT_CLK_RATE);
}

static int adi_sdhci_non_dma_xfer(int lba, uintptr_t buf, const size_t size, const bool dir)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout = SDHCI_DATA_XFER_TIMEOUT_US_1_S;
	uint32_t xfer_cnt = 0U;
	uint32_t xfer_blk_cnt = 0U;
	uint32_t *buffer;
	uint32_t pstate_mask;
	volatile uint16_t normal_int_stat;
	uint16_t int_mask;
	size_t block_cnt;
	size_t block_size;

	buffer = (uint32_t *)buf;

	if (size < MMC_BLOCK_SIZE) {
		block_size = size;
		block_cnt = 1;
	} else {
		block_size = MMC_BLOCK_SIZE;
		block_cnt = size / block_size;
		/* Xfers with sizes which aren't multiples of block size are not supported */
		if (size % block_size != 0)
			return -EINVAL;
	}

	if (SDHCI_DATA_XFER_READ == dir) {
		int_mask = BUF_RD_READY_STAT_EN_BM;
		pstate_mask = BUF_RD_ENABLE_BM;
	} else {
		int_mask = BUF_WR_READY_STAT_EN_BM;
		pstate_mask = BUF_WR_ENABLE_BM;
	}

	/* Wait till all the blocks of data are written/read */
	while (xfer_blk_cnt < (uint32_t)block_cnt) {
		/* Wait for Buffer Write/Read Ready interrupt */
		normal_int_stat = mmio_read_16(base + SDHCI_NORMAL_INT_STAT_R_OFF);

		/* Handle error interrupts */
		/* Perform Error Recovery Sequence as given in Spec and return */
		if (normal_int_stat & ERR_INTERRUPT_BM)
			return adi_sdhci_handle_bus_errors();

		if (normal_int_stat & int_mask) {
			/* In case of write, check if space is available for writing data and
			 * in case of read, check if valid data exists in the host buffer.
			 * If yes, proceed. Else, wait */
			if (0U == (mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) & pstate_mask))
				continue;

			/* Clear Buffer Write/Read Ready status */
			mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, int_mask);

			/* Reset the counter */
			xfer_cnt = 0U;

			/* Write/Read block data to or from Buffer Data Port register */
			while (xfer_cnt < (uint32_t)(block_size / 4U)) {
				if (SDHCI_DATA_XFER_READ == dir)
					buffer[(((uint32_t)(block_size / 4U) * xfer_blk_cnt) + xfer_cnt)] = mmio_read_32(base + SDHCI_BUF_DATA_R_OFF);
				else
					mmio_write_32(base + SDHCI_BUF_DATA_R_OFF, buffer[(((uint32_t)(block_size / 4U) * xfer_blk_cnt) + xfer_cnt)]);
				++xfer_cnt;
			}
			++xfer_blk_cnt;
		}

		if (timeout-- > 0U) {
			udelay(10U);
		} else {
			ERROR("%s: Transfer data timeout.\n", __func__);
			return -ETIMEDOUT;
		}
	}

	/* Wait for xfer complete interrupt */
	timeout = timeout_init_us(SDHCI_CMD_TIMEOUT_US_100_MS);

	while (0U == (mmio_read_16(base + SDHCI_NORMAL_INT_STAT_R_OFF) & XFER_COMPLETE_BM)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: Data Xfer never completed.\n", __func__);
			return -ETIMEDOUT;
		}
	}

	/* Clear transfer complete status */
	mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, XFER_COMPLETE_BM);

	VERBOSE("%s: block size=%d, block count=%d\n", __func__, (int)block_size, (int)block_cnt);

	return 0;
}

static int adi_sdhci_dma_xfer(uintptr_t buf)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout = SDHCI_DATA_XFER_TIMEOUT_US_1_S;
	uint32_t sdma_buf_addr = (uint32_t)buf;
	volatile uint16_t normal_int_stat;

	/* Wait till DMA interrupt or xfer complete interrupt occurs */
	do {
		normal_int_stat = mmio_read_16(base + SDHCI_NORMAL_INT_STAT_R_OFF);

		/* Handle error interrupts */
		/* Perform Error Recovery Sequence as given in Spec and return */
		if (normal_int_stat & ERR_INTERRUPT_BM)
			return adi_sdhci_handle_bus_errors();

		if (normal_int_stat & DMA_INTERRUPT_BM) {
			/* Clear the DMA interrupt status */
			mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, DMA_INTERRUPT_BM);
			/* Update the buffer address in System Address register for SDMA */
			sdma_buf_addr &= ~(SDHCI_SDMA_BOUNDARY_SIZE - 1U);
			sdma_buf_addr += SDHCI_SDMA_BOUNDARY_SIZE;
			mmio_write_32(base + SDHCI_ADMA_SA_LOW_R_OFF, sdma_buf_addr);
		}

		if (normal_int_stat & XFER_COMPLETE_BM) {
			/* Clear the transfer complete interrupt status */
			mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, XFER_COMPLETE_BM);
			break;
		}

		if (timeout-- > 0U) {
			udelay(10U);
		} else {
			ERROR("%s: Transfer data timeout.\n", __func__);
			return -ETIMEDOUT;
		}
	} while ((!(normal_int_stat & DMA_INTERRUPT_BM)) || (!(normal_int_stat & XFER_COMPLETE_BM)));

	return 0;
}

static int adi_sdhci_handle_bus_errors(void)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;
	volatile uint16_t err_int_stat;
	struct mmc_cmd cmd;
	int ret;

	zeromem(&cmd, sizeof(struct mmc_cmd));

	/* Get error interrupt status */
	err_int_stat = mmio_read_16(base + SDHCI_ERROR_INT_STAT_R_OFF);

	/* Clear the error interrupt status */
	mmio_write_16(base + SDHCI_ERROR_INT_STAT_R_OFF, err_int_stat);

	/* Reset CMD line on occurrance of command line error */
	if (err_int_stat & CMD_LINE_ERR_INTR_BM) {
		ret = adi_sdhci_sw_reset(SW_RST_CMD_BM);
		if (ret != 0)
			return ret;
	}

	/* Reset DAT line on occurrance of data line error */
	if (err_int_stat & DATA_LINE_ERR_INTR_BM) {
		ret = adi_sdhci_sw_reset(SW_RST_DAT_BM);
		if (ret != 0)
			return ret;
	}

	/* Issue Abort command */
	abort_cmd_issued = true;
	cmd.cmd_idx = SDHCI_CMD_STOP_TRANSMISSION;
	cmd.cmd_arg = 0;
	cmd.resp_type = MMC_RESPONSE_R1B;

	ret = adi_sdhci_send_cmd(&cmd);
	if (0 == ret)
		ret = -EIO;

	/* Wait for the host controller to become ready for issuing the next command */
	timeout = timeout_init_us(SDHCI_CMD_TIMEOUT_US_100_MS);

	while (mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) & (CMD_INHIBIT_BM | CMD_INHIBIT_DAT_BM)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: CMD/DAT line busy\n", __func__);
			ret = -ETIMEDOUT;
			break;
		}
	}

	WARN("%s: Recovered from bus error. ERR Int Status:0x%x\n",
	     __func__, err_int_stat);

	return ret;
}

/****************** External Function Definitions ******************/
static void adi_sdhci_init(void)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;
	uint16_t u16_reg_data;

	/* This is needed when any of the BL stages tries to re-initialize the eMMC */
	card_initialized = false;

	/* Use non-dma mode for initialization */
	use_dma_mode = false;

	/* Reset the Host Controller */
	(void)adi_sdhci_sw_reset(SW_RST_ALL_BM);

	/* Perform PHY Power-up and Reset Programming Sequence if needed */
	if (adi_sdhci_params.phy_config_needed)
		(void)adi_sdhci_phy_init(adi_sdhci_params.phy_reg_base);

	/* Initialize the Host Controller. This includes Host Controller Setup Sequence +
	 * Host Controller Clock Setup Sequence */
	adi_sdhci_initialize_host_controller();

	/* For SD, wait up SDHCI_CARD_DETECT_TIMEOUT_US_1_MS for a card to be inserted.
	 * This is necessary to account for the delay in card detetion logic within the controller.
	 * (Cards present at boot will take ~100us to be detected)
	 */
	if (MMC_IS_SD == adi_sdhci_get_dev_type()) {
		timeout = timeout_init_us(SDHCI_CARD_DETECT_TIMEOUT_US_1_MS);
		while ((mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) & CARD_INSERTED_BM) != CARD_INSERTED_BM) {
			if (timeout_elapsed(timeout)) {
				ERROR("%s: SD-Card Not Inserted\n", __func__);
				break;
			}
		}
	}

	if ((MMC_IS_EMMC == adi_sdhci_get_dev_type()) || ((mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) & CARD_INSERTED_BM) == CARD_INSERTED_BM)) {
		/* Supply power and clock */
		u16_reg_data = (mmio_read_16(base + SDHCI_HOST_CTRL2_R_OFF) & ~UHS_MODE_SEL_BM);
		mmio_write_16(base + SDHCI_HOST_CTRL2_R_OFF, u16_reg_data);

		/* Supply Clock to Card */
		(void)adi_sdhci_control_card_clk(SDHCI_SUPPLY_CARD_CLK);
	}
}

static int adi_sdhci_send_cmd(struct mmc_cmd *cmd)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint64_t timeout;
	uint32_t pstate_mask;
	uint32_t cmd_r_flags;
	uint16_t xfer_mode = 0U;
	volatile uint16_t normal_int_stat;
	int err = 0;

	if (NULL == cmd)
		return -EINVAL;

	/* For SD, block all commands from being sent if an SD card isn't inserted.
	 * We can't provide feedback regarding this condition from adi_sdhci_init(),
	 * so this is the best we can do to cause the driver to fail.
	 */
	if ((adi_sdhci_get_dev_type() != MMC_IS_EMMC) && ((mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) & CARD_INSERTED_BM) != CARD_INSERTED_BM)) {
		ERROR("%s: SD-Card Not Inserted\n", __func__);
		return -EIO;
	}

	/******* Sequence to issue an SD command *******/
	pstate_mask = CMD_INHIBIT_BM;

	/* If host driver issues command using DAT line including busy signal, check Command Inhibit(DAT) */
	if ((cmd->resp_type & MMC_RSP_BUSY) || (adi_sdhci_is_data_xfr_cmd(cmd->cmd_idx)))
		pstate_mask |= CMD_INHIBIT_DAT_BM;

	/* For Abort command, no need to wait for Command Inhibit(DAT) */
	if (SDHCI_CMD_STOP_TRANSMISSION == cmd->cmd_idx)
		pstate_mask &= ~CMD_INHIBIT_DAT_BM;

	/* Wait for the Host Controller to become ready to issue a command */
	timeout = timeout_init_us(SDHCI_CMD_TIMEOUT_US_100_MS);

	while (mmio_read_32(base + SDHCI_PSTATE_REG_R_OFF) & pstate_mask) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: Failed to send cmd %d. CMD/DAT line busy.\n",
			      __func__, cmd->cmd_idx);
			return -ETIMEDOUT;
		}
	}

	/* Ideally, this condition will never hit because 136-bit commands do
	* not have busy waiting. So, just remove busy waiting and continue */
	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY)) {
		WARN("Removing busy wait from 136-bit commands.\n");
		cmd->resp_type &= ~MMC_RSP_BUSY;
	}

	/* Form RESP_TYPE_SELECT field of the CMD_R register */
	if (cmd->resp_type & MMC_RSP_136)
		cmd_r_flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		cmd_r_flags = SDHCI_CMD_RESP_SHORT_BUSY;
	else if (cmd->resp_type & MMC_RSP_48)
		cmd_r_flags = SDHCI_CMD_RESP_SHORT;
	else
		cmd_r_flags = SDHCI_CMD_RESP_NONE;

	/* Form CMD_CRC_CHK_ENABLE field of the CMD_R register */
	if (cmd->resp_type & MMC_RSP_CRC)
		cmd_r_flags |= CMD_CRC_CHK_ENABLE_BM;

	/* Form CMD_IDX_CHK_ENABLE field of the CMD_R register */
	if (cmd->resp_type & MMC_RSP_CMD_IDX)
		cmd_r_flags |= CMD_IDX_CHK_ENABLE_BM;

	/* Form DATA_PRESENT_SEL field of the CMD_R register */
	if (adi_sdhci_is_data_xfr_cmd(cmd->cmd_idx))
		cmd_r_flags |= DATA_PRESENT_SEL_BM;

	/* Form CMD_TYPE field of the CMD_R register. While issuing Abort CMD using
	 * CMD12/CMD52 or reset CMD using CMD0/CMD52, this should be 0x3 and for
	 * other commands 0x0. In case CMD0 is used for initialization, set as 0x0 */
	if ((SDHCI_CMD_STOP_TRANSMISSION == cmd->cmd_idx) ||
	    ((SDHCI_CMD_GO_IDLE_STATE == cmd->cmd_idx) && (card_initialized)))
		cmd_r_flags |= (CMD_TYPE_ABORT << CMD_TYPE_POS);

	/* Acknowledge pending interrupts before writing to CMD_R register */
	mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, NORMAL_INT_STAT_MASK);
	mmio_write_16(base + SDHCI_ERROR_INT_STAT_R_OFF, ERROR_INT_STAT_MASK);

	/* Write the argument to be sent with the command */
	mmio_write_32(base + SDHCI_ARGUMENT_R_OFF, cmd->cmd_arg);

	/* Set the value to the transfer mode register for data transfer commands */
	if (adi_sdhci_is_data_xfr_cmd(cmd->cmd_idx)) {
		/* Enable Block Count and Block Select for Multiple block transfers */
		if ((SDHCI_CMD_READ_MULTIPLE_BLOCK == cmd->cmd_idx) ||
		    (SDHCI_CMD_WRITE_MULTIPLE_BLOCK == cmd->cmd_idx))
			xfer_mode = (MULTI_BLK_SEL_BM | BLOCK_COUNT_ENABLE_BM);

		/* Set the data transfer direction.
		 * 0 - Write
		 * 1 - Read */
		if ((SDHCI_CMD_READ_SINGLE_BLOCK == cmd->cmd_idx) ||
		    (SDHCI_CMD_READ_MULTIPLE_BLOCK == cmd->cmd_idx) ||
		    (SDHCI_CMD_SEND_EXT_CSD == cmd->cmd_idx) ||
		    (SDHCI_CMD_SEND_SCR == cmd->cmd_idx))
			xfer_mode |= DATA_XFER_DIR_BM;

		/* Enable DMA functionality */
		if (adi_sdhci_use_dma_for_data_xfer())
			xfer_mode |= DMA_ENABLE_BM;
	}

	mmio_write_16(base + SDHCI_XFER_MODE_R_OFF, xfer_mode);

	/* Configure Command register with CMD register flags and CMD index
	 * SD command issued when upper byte [3] of CMD_R is written */
	mmio_write_16(base + SDHCI_CMD_R_OFF, SDHCI_MAKE_CMD(cmd->cmd_idx, cmd_r_flags));

	VERBOSE("%s: cmd_idx=0x%x, cmd_r_flags=0x%x, xfer_mode=0x%x, arg=0x%x\n",
		__func__, cmd->cmd_idx, cmd_r_flags, xfer_mode, cmd->cmd_arg);

	/******* Sequence to complete SD command ********/
	/* Check if response check is enabled */
	if (0U == (mmio_read_16(base + SDHCI_XFER_MODE_R_OFF) & RESP_INT_DISABLE_BM)) {
		/* Wait till command gets completed and response packet is received */
		timeout = timeout_init_us(SDHCI_DATA_XFER_TIMEOUT_US_1_S);

		do {
			normal_int_stat = mmio_read_16(base + SDHCI_NORMAL_INT_STAT_R_OFF);

			/* Break upon detection of any of the error interrupts */
			if (normal_int_stat & ERR_INTERRUPT_BM) {
				if (abort_cmd_issued) {
					/* Error recovery already performed once */
					ERROR("%s: Non-recoverable Error. cmd_idx=0x%x\n",
					      __func__, cmd->cmd_idx);
					err = -EIO;
				} else {
					/* Perform error recovery sequence as per Section 4.12.1 in Spec */
					err = adi_sdhci_handle_bus_errors();
				}
				break;
			}

			if (timeout_elapsed(timeout)) {
				ERROR("%s: Response not received for cmd %d.\n",
				      __func__, cmd->cmd_idx);
				err = -ETIMEDOUT;
				break;
			}
		} while (0U == (normal_int_stat & CMD_COMPLETE_BM));

		if (0 == err) {
			/* Clear the command complete interrupt status */
			mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, CMD_COMPLETE_BM);

			/* Get Response Data from Response registers to get necessary information
			 * about issued command */
			if ((cmd->resp_type != SDHCI_CMD_RESP_NONE) && (cmd->resp_data != NULL)) {
				/* Store 48 bit response (e.g. R1 Response) */
				cmd->resp_data[0] = mmio_read_32(base + SDHCI_RESP01_R_OFF);
				if (cmd->resp_type & MMC_RSP_136) {
					/* Store 136 bit the response (e.g. R2 response) */
					cmd->resp_data[1] = mmio_read_32(base + SDHCI_RESP23_R_OFF);
					cmd->resp_data[2] = mmio_read_32(base + SDHCI_RESP45_R_OFF);
					cmd->resp_data[3] = mmio_read_32(base + SDHCI_RESP67_R_OFF);
					/* CRC is stripped from 136-bit response, so we need to do some shifting */
					uint32_t carry_prev, carry = 0U;
					for (uint8_t i = 0U; i < 4U; ++i) {
						carry_prev = (cmd->resp_data[i] & 0xFF000000U) >> 24U;
						cmd->resp_data[i] <<= 8U;
						cmd->resp_data[i] |= carry;
						carry = carry_prev;
					}
				}
			}
		}
	}

	/* Check if command uses Transfer Complete Interrupt. This interrupt indicates stop of
	 * transaction on completion of a command pairing with response-with-busy(R1B, R5B) */
	if (cmd->resp_type & MMC_RSP_BUSY) {
		/* Wait for Transfer Complete interrupt */
		timeout = timeout_init_us(SDHCI_DATA_XFER_TIMEOUT_US_1_S);

		do {
			normal_int_stat = mmio_read_16(base + SDHCI_NORMAL_INT_STAT_R_OFF);

			/* Transfer Complete has higher priority than Data Timeout Error */
			if (normal_int_stat & XFER_COMPLETE_BM) {
				/* Clear the command complete interrupt status */
				mmio_write_16(base + SDHCI_NORMAL_INT_STAT_R_OFF, XFER_COMPLETE_BM);
				break;
			}
			/* Perform error recovery sequence as per Section 4.12.1 in Spec */
			if (normal_int_stat & ERR_INTERRUPT_BM) {
				if (abort_cmd_issued)
					/* Error recovery already performed once */
					err = -EIO;
				else
					/* Perform error recovery sequence as per Section 4.12.1 in Spec */
					err = adi_sdhci_handle_bus_errors();
				break;
			}

			if (timeout_elapsed(timeout)) {
				ERROR("%s: Command %d with busy response never completed.\n",
				      __func__, cmd->cmd_idx);
				err = -ETIMEDOUT;
				break;
			}
		} while (0U == (normal_int_stat & XFER_COMPLETE_BM));
	}

	abort_cmd_issued = false;

	/* If CMD(7) is successful, eMMC card would be initialized and active for
	 * transactions by the host controller. */
	if (SDHCI_CMD_SELECT_CARD == cmd->cmd_idx)
		card_initialized = true;

	return err;
}

static int adi_sdhci_set_ios(unsigned int clk, unsigned int width)
{
	int ret;

	/* Configure data bus width */
	ret = adi_sdhci_set_bus_width(width);
	if (ret != 0)
		return ret;

	/* Configure Clock */
	return adi_sdhci_change_clk_freq(clk);
}

static int adi_sdhci_prepare(int lba, uintptr_t buf, size_t size)
{
	uintptr_t base = adi_sdhci_params.reg_base;
	uint16_t u16_reg_data;
	uint8_t u8_reg_data;
	size_t block_cnt;
	size_t block_size;
	bool use_dma = adi_sdhci_use_dma_for_data_xfer();

	if (size < MMC_BLOCK_SIZE) {
		block_size = size;
		block_cnt = 1;
	} else {
		block_size = MMC_BLOCK_SIZE;
		block_cnt = size / block_size;
		/* Xfers with sizes which aren't multiples of block size are not supported */
		if (size % block_size != 0)
			return -EINVAL;
	}

	/* Clean the cache as it is required to make changes made by the processor
	* visible to system memory, so that it can be read by a DMA controller. */
	if (use_dma)
		flush_dcache_range(buf, size);

	/***** Prepare the Host Controller for data transfer *****/
	/* Clear the DMA_SEL bits in HOST_CTRL1_R */
	u8_reg_data = (mmio_read_8(base + SDHCI_HOST_CTRL1_R_OFF) & ~DMA_SEL_BM);
	/* Enable SDMA Selection */
	if (use_dma)
		u8_reg_data |= (DMA_SEL_SDMA << DMA_SEL_POS);
	mmio_write_8(base + SDHCI_HOST_CTRL1_R_OFF, u8_reg_data);

	/* Configure the block size of data transfers. In alignment with the
	 * implementation note from the Spec, "Even though it indicates larger than
	 * 512 bytes, block length shall be set to 512 byte for data transfer. This
	 * is because 512 byte block length is required to keep compatability with
	 * 512 bytes data boundary" */
	u16_reg_data = (mmio_read_16(base + SDHCI_BLOCK_SIZE_R_OFF) & ~XFER_BLOCK_SIZE_BM);
	u16_reg_data |= block_size;
	/* Configure SDMA Buffer Boundary. Set maximum value of 512K as the size of
	 * contiguous buffer in system memory to avoid frequent DMA interrupts */
	if (use_dma) {
		u16_reg_data &= ~SDMA_BUF_BDARY_BM;
		u16_reg_data |= (SDMA_BUF_BDARY_512K << SDMA_BUF_BDARY_POS);
	}
	mmio_write_16(base + SDHCI_BLOCK_SIZE_R_OFF, u16_reg_data);

	/* For Version 4.10 onwards, if v4 mode is enabled, 32-bit Block Count
	 * must be supported, in that case 16-bit block count register must be 0. */
	mmio_write_16(base + SDHCI_BLOCK_COUNT_R_OFF, 0x00U);
	mmio_write_32(base + SDHCI_32BIT_BLK_CNT_R_OFF, block_cnt);

	/* Set data location of the system memory */
	if (use_dma)
		mmio_write_32(base + SDHCI_ADMA_SA_LOW_R_OFF, (uint32_t)buf);

	return 0;
}

static int adi_sdhci_read(int lba, uintptr_t buf, size_t size)
{
	int ret;

	if (adi_sdhci_use_dma_for_data_xfer()) {
		ret = adi_sdhci_dma_xfer(buf);
		/* System memory is written by DMA and for those changes to
		 * be visible to the processor, invalidate the cache */
		inv_dcache_range(buf, size);
	} else {
		ret = adi_sdhci_non_dma_xfer(lba, buf, size, SDHCI_DATA_XFER_READ);
	}

	return ret;
}

static int adi_sdhci_write(int lba, uintptr_t buf, size_t size)
{
	if (adi_sdhci_use_dma_for_data_xfer())
		return adi_sdhci_dma_xfer(buf);
	else
		return adi_sdhci_non_dma_xfer(lba, buf, size, SDHCI_DATA_XFER_WRITE);
}

int adi_mmc_init(struct adi_mmc_params *params)
{
	int ret;

	assert((params != NULL) &&
	       ((params->reg_base & MMC_BLOCK_MASK) == 0U) &&
	       ((params->bus_width == MMC_BUS_WIDTH_1) ||
		(params->bus_width == MMC_BUS_WIDTH_4) ||
		(params->bus_width == MMC_BUS_WIDTH_8)));

	/* Store a copy of MMC config params */
	memcpy(&adi_sdhci_params, params, sizeof(struct adi_mmc_params));

	/* Store a copy of MMC device info */
	memcpy(&sdhci_dev_info, params->device_info, sizeof(struct mmc_device_info));
	/* Overwrite the MMC device info address with the SDHCI
	 * device info so that it's scope would be always global */
	adi_sdhci_params.device_info = &sdhci_dev_info;

	/* Infinite block transfers are not supported, emit a warning
	 * and force to finite block xfers. CMD23 flag needs to be set
	 * for mmc.c to follow finite transfer method. */
	if (0U == (adi_sdhci_params.flags & MMC_FLAG_CMD23)) {
		WARN("Enabling Finite block MMC transfers.\n");
		adi_sdhci_params.flags |= MMC_FLAG_CMD23;
	}

	ret = mmc_init(&adi_sdhci_ops, adi_sdhci_params.clk_rate,
		       adi_sdhci_params.bus_width, adi_sdhci_params.flags,
		       adi_sdhci_params.device_info);

	/* Use Non-DMA mode for MMC Initialization. Post init,
	 * follow the user configuration. */
	use_dma_mode = adi_sdhci_params.use_dma;

	return ret;
}

int adi_mmc_deinit(uintptr_t reg_base)
{
	adi_sdhci_params.reg_base = reg_base;
	uint16_t u16_reg_data;

	/* Disable SDCLK as part of MMC deinitialization
	 * This API will ensure no data xfers are in progress. */
	int ret = adi_sdhci_control_card_clk(SDHCI_STOP_CARD_CLK);
	if (ret != 0)
		return ret;

	/* Turn off Internal Clock and PLL. This will avoid
	 * unnecessary glitches when MMC source clock is changed */
	u16_reg_data = mmio_read_16(reg_base + SDHCI_CLK_CTRL_R_OFF);
	u16_reg_data &= ~(INTERNAL_CLK_EN_BM | PLL_ENABLE_BM);
	mmio_write_16(reg_base + SDHCI_CLK_CTRL_R_OFF, u16_reg_data);

	return 0;
}
