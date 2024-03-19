/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include <drivers/mmc.h>
#include <drivers/adi/adi_sdhci.h>
#include <drivers/adi/adi_spu.h>
#include <drivers/adi/adrv906x/clk.h>
#include <lib/utils.h>

#include <platform_def.h>
#include <plat_pinctrl.h>
#include <adrv906x_spu_def.h>

/************************** Test INSTRUCTIONS ***************************/
/* This is a test framework to verify the functionality of SDHCI driver which
 * supports eMMC and SD interfaces. By default, this framework is not included
 * in TF-A build. Do the following before attempting to test the driver.
 * 1. Add the following in drivers/adi/test/test_framework.c
 *    -> extern void adi_sdhci_test_data_xfer(); or define the prototype
 *       of adi_sdhci_test_data_xfer() in a suitable header and include
 *       the header in test_framework.c
 *    -> Call adi_sdhci_test_data_xfer() inside test_main()
 * 2. Include this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 * 3. If needed, modify the following parameters(macros) as per the test requirements
 * 4. Un-comment the definition and call to adi_sdhci_test_reinit() if MMC Re-init is needed */

/************************** Test PARAMETERS ****************************/
/* MMC Interface used for testing, can take MMC_IS_EMMC or MMC_IS_SD as value */
#define INTERFACE                    MMC_IS_EMMC
/* Maximum size of data transfer in bytes - 17KB */
#define MAX_DAT_XFER_LEN_BYTES       (17U * 1024U)
/* Logical Block Address of the card from/to which data xfers are performed */
#define CARD_LBA                     (0x0U)
/* MMC Clock frequency used for testing, can take values from
 * eMMC interface: 1-26MHz
 * SD   interface: 1-25MHz */
#define CLK_RATE_HZ                  (26U * 1000U * 1000U)
/* MMC bus width used for testing, can take one of the following values
 * eMMC interface: MMC_BUS_WIDTH_1, MMC_BUS_WIDTH_4 or MMC_BUS_WIDTH_8
 * SD   interface: MMC_BUS_WIDTH_1 or MMC_BUS_WIDTH_4 */
#define BUS_WIDTH                    MMC_BUS_WIDTH_8
/* MMC DMA mode selection for data xfers */
#define USE_DMA_MODE                 true

/* mmc.c expects the address of write buffer array to be 512-byte aligned */
static uint8_t wr_buffer[MAX_DAT_XFER_LEN_BYTES] __aligned(512);
static uint8_t rd_buffer[MAX_DAT_XFER_LEN_BYTES] __aligned(16);

/* Function to initialize SDHCI driver with the test parameters */
static int adi_sdhci_test_init(enum mmc_device_type device)
{
	struct mmc_device_info mmc_info;
	struct adi_mmc_params mmc_params;
	extern const plat_pinctrl_settings sd_pin_grp[];
	extern const size_t sd_pin_grp_members;

	zeromem(&mmc_params, sizeof(struct adi_mmc_params));
	mmc_info.mmc_dev_type = device;

	/* Initialize common parameters */
	mmc_params.clk_rate = CLK_RATE_HZ;
	mmc_params.bus_width = BUS_WIDTH;
	mmc_params.device_info = &mmc_info;
	mmc_params.use_dma = USE_DMA_MODE;
	mmc_params.flags = MMC_FLAG_CMD23;
	mmc_params.src_clk_hz = clk_get_freq(CLK_CTL, CLK_ID_EMMC);

	/* Initialize device specific parameters */
	if (MMC_IS_EMMC == device) {
		mmc_params.reg_base = EMMC_0_BASE;
		/* eMMC interface of dwc_mshc includes a PHY solution */
		mmc_params.phy_config_needed = true;
		mmc_params.phy_reg_base = EMMC_0_PHY_BASE;
		/* Enable SPU MSEC */
		adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_EMMC0SLV);
	} else { /* MMC_IS_SD */
		mmc_params.reg_base = SD_0_BASE;
		/* SD interface of dwc_mshc does not include PHY solution */
		mmc_params.phy_config_needed = false;
		mmc_info.ocr_voltage = OCR_3_3_3_4 | OCR_3_2_3_3;
		/* Enable SPU MSEC */
		adi_spu_enable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_EMMC1SLV);
		/* Configure pinmux to enable SD pins */
		(void)plat_secure_pinctrl_set_group(sd_pin_grp, sd_pin_grp_members, true);
	}

	return adi_mmc_init(&mmc_params);
}

/* Function to re-initialize MMC after a data xfer */
#if 0
static int adi_sdhci_test_reinit(void)
{
	int ret;

	ret = mmc_write_blocks(0, (uintptr_t)&wr_buffer, MMC_BLOCK_SIZE);

	if (MMC_BLOCK_SIZE == ret) {
		printf("MMC Write of %d bytes to   Addr 0x%x Passed\n", MMC_BLOCK_SIZE, 0);
	} else {
		printf("MMC Write of %d bytes to   Addr 0x%x Failed\n", MMC_BLOCK_SIZE, 0);
		return 0;
	}

	ret = mmc_read_blocks(0, (uintptr_t)&rd_buffer, MMC_BLOCK_SIZE);

	if (MMC_BLOCK_SIZE == ret) {
		printf("MMC Read  of %d bytes from Addr 0x%x Passed\n", MMC_BLOCK_SIZE, 0);
	} else {
		printf("MMC Read  of %d bytes from Addr 0x%x Failed\n", MMC_BLOCK_SIZE, 0);
		return 0;
	}

	for (unsigned int j = 0U; j < MMC_BLOCK_SIZE; ++j) {
		if (wr_buffer[j] != rd_buffer[j]) {
			printf("Data mismatch\n");
			printf("Test Failed\n");
			return 0;
		}
	}
	printf("Read data matches with write data!!!\n\n");

	ret = adi_sdhci_test_init(INTERFACE);

	if (0 == ret) {
		printf("MMC Re-initialized Successfully\n\n");
	} else {
		printf("\nMMC Re-init Failed\n");
		printf("Test Failed\n");
	}

	return ret;
}
#endif

void adi_sdhci_test_data_xfer(void)
{
	unsigned int flag = 0U;
	int ret;

	if (0 == adi_sdhci_test_init(INTERFACE)) {
		printf("\nMMC Initialized Successfully. Bus width = %d, clk = %dHz\n\n", BUS_WIDTH * 4U, CLK_RATE_HZ);
	} else {
		printf("\nMMC Init Failed\n");
		printf("Test Failed\n");
		return;
	}

#if 0
	if (adi_sdhci_test_reinit() != 0)
		return;
#endif

	/* Fill the write buffer */
	for (unsigned int i = 0U; i < MAX_DAT_XFER_LEN_BYTES; ++i)
		wr_buffer[i] = i + 5;

	/* Perform read/write for sizes starting from 512 till MAX_DAT_XFER_LEN_BYTES
	 * with all lengths being multiple of 512 */
	for (uint8_t i = 1U; i <= (MAX_DAT_XFER_LEN_BYTES / MMC_BLOCK_SIZE); ++i) {
		ret = mmc_write_blocks((CARD_LBA * i), (uintptr_t)&wr_buffer, (MMC_BLOCK_SIZE * i));

		if ((MMC_BLOCK_SIZE * i) == ret) {
			printf("MMC Write of %d bytes to   Addr 0x%x Passed\n", (MMC_BLOCK_SIZE * i), (CARD_LBA * i));
		} else {
			printf("MMC Write of %d bytes to   Addr 0x%x Failed\n", (MMC_BLOCK_SIZE * i), (CARD_LBA * i));
			flag = 0xFFFFFFFFU;
			break;
		}

		ret = mmc_read_blocks((CARD_LBA * i), (uintptr_t)&rd_buffer, (MMC_BLOCK_SIZE * i));

		if ((MMC_BLOCK_SIZE * i) == ret) {
			printf("MMC Read  of %d bytes from Addr 0x%x Passed\n", (MMC_BLOCK_SIZE * i), (CARD_LBA * i));
		} else {
			printf("MMC Read  of %d bytes from Addr 0x%x Failed\n", (MMC_BLOCK_SIZE * i), (CARD_LBA * i));
			flag = 0xFFFFFFFFU;
			break;
		}

		for (unsigned int j = 0U; j < (MMC_BLOCK_SIZE * i); ++j) {
			if (wr_buffer[j] != rd_buffer[j]) {
				printf("Data mismatch from index %d\n", j);
				flag = 0xFFFFFFFFU;
				break;
			}
		}
		if (0U == flag)
			printf("Read data matches with write data!!!\n\n");
		else
			break;
	}

	if (0U == flag)
		printf("Test Passed\n");
	else
		printf("Test Failed\n");
}
