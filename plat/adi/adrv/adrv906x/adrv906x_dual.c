/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <arch/aarch64/arch_helpers.h>
#include <common/debug.h>
#include <drivers/adi/adi_te_interface.h>
#include <drivers/adi/adi_c2cc.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <adrv906x_clkrst_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_dual.h>
#include <adrv906x_mmap.h>
#include <adrv906x_secondary_image.h>
#include <plat_err.h>
#include <platform_def.h>

/* training setup parameters */
#define ADI_ADRV906X_C2C_TRAINING_POLYNOMIAL 0xD008      /* x^16 + x^15 + x^13 + x^4 + 1 */
#define ADI_ADRV906X_C2C_TRAINING_SEED 0x42
#define ADI_ADRV906X_C2C_ROSC_DIV 0
#define ADI_ADRV906X_C2C_DEVCLK_DIV 0
#define ADI_ADRV906X_C2C_PLL_DIV 0

/* primary-to-secondary training delays */
#define ADI_ADRV906X_C2C_P2S_RXCMD2TRNTRM 0x15
#define ADI_ADRV906X_C2C_P2S_CMD2TXCLKSW 0x59
#define ADI_ADRV906X_C2C_P2S_TXCLKSW2PTRN 0x3F
#define ADI_ADRV906X_C2C_P2S_RXCMD2LD 0x5D
#define ADI_ADRV906X_C2C_P2S_LDDUR 0x27
#define ADI_ADRV906X_C2C_P2S_MTCHDUR 0x15
#define ADI_ADRV906X_C2C_P2S_TXPTRNDUR 0x54
#define ADI_ADRV906X_C2C_P2S_SWBCKCLK 0x25
#define ADI_ADRV906X_C2C_P2S_SWBCKTRM 0x5C
#define ADI_ADRV906X_C2C_P2S_CALDONE 0x60
#define ADI_ADRV906X_C2C_P2S_RESP2CMD 0x58
#define ADI_ADRV906X_C2C_P2S_BLKTRF 0x50

/* secondary-to-primary training delays */
#define ADI_ADRV906X_C2C_S2P_RXCMD2TRNTRM 0x20
#define ADI_ADRV906X_C2C_S2P_CMD2TXCLKSW 0x5E
#define ADI_ADRV906X_C2C_S2P_TXCLKSW2PTRN 0x2B
#define ADI_ADRV906X_C2C_S2P_RXCMD2LD 0x4F
#define ADI_ADRV906X_C2C_S2P_LDDUR 0x3A
#define ADI_ADRV906X_C2C_S2P_MTCHDUR 0x15
#define ADI_ADRV906X_C2C_S2P_TXPTRNDUR 0x5E
#define ADI_ADRV906X_C2C_S2P_SWBCKCLK 0x25
#define ADI_ADRV906X_C2C_S2P_SWBCKTRM 0x39
#define ADI_ADRV906X_C2C_S2P_CALDONE 0x62
#define ADI_ADRV906X_C2C_S2P_RESP2CMD 0x2B
#define ADI_ADRV906X_C2C_S2P_BLKTRF 0x50

#define SECONDARY_TE_HOST_BOOT_TIMEOUT_US 50000
#define SECONDARY_TE_HOST_BOOT_ENABLE 0x1048

extern void memcpy16(void *dst, const void *src, unsigned int len);

static struct adi_c2cc_training_settings adrv906x_c2c_training_params = {
	.tx_clk			={
		.rosc_div	= ADI_ADRV906X_C2C_ROSC_DIV,
		.devclk_div	= ADI_ADRV906X_C2C_DEVCLK_DIV,
		.pll_div	= ADI_ADRV906X_C2C_PLL_DIV,
	},
	.generator		={
		.poly		= ADI_ADRV906X_C2C_TRAINING_POLYNOMIAL,
		.seed		= ADI_ADRV906X_C2C_TRAINING_SEED,
	},
	.p2s_delay		={
		.resp2cmd	= ADI_ADRV906X_C2C_P2S_RESP2CMD,
		.cmd2txclksw	= ADI_ADRV906X_C2C_P2S_CMD2TXCLKSW,
		.txclksw2ptrn	= ADI_ADRV906X_C2C_P2S_TXCLKSW2PTRN,
		.txptrndur	= ADI_ADRV906X_C2C_P2S_TXPTRNDUR,
		.swbckclk	= ADI_ADRV906X_C2C_P2S_SWBCKCLK,
		.caldone	= ADI_ADRV906X_C2C_P2S_CALDONE,
		.rxcmd2trntrm	= ADI_ADRV906X_C2C_P2S_RXCMD2TRNTRM,
		.rxcmd2ld	= ADI_ADRV906X_C2C_P2S_RXCMD2LD,
		.lddur		= ADI_ADRV906X_C2C_P2S_LDDUR,
		.mtchdur	= ADI_ADRV906X_C2C_P2S_MTCHDUR,
		.swbcktrm	= ADI_ADRV906X_C2C_P2S_SWBCKTRM,
		.blktrf		= ADI_ADRV906X_C2C_P2S_BLKTRF,
	},
	.s2p_delay		={
		.resp2cmd	= ADI_ADRV906X_C2C_S2P_RESP2CMD,
		.cmd2txclksw	= ADI_ADRV906X_C2C_S2P_CMD2TXCLKSW,
		.txclksw2ptrn	= ADI_ADRV906X_C2C_S2P_TXCLKSW2PTRN,
		.txptrndur	= ADI_ADRV906X_C2C_S2P_TXPTRNDUR,
		.swbckclk	= ADI_ADRV906X_C2C_S2P_SWBCKCLK,
		.caldone	= ADI_ADRV906X_C2C_S2P_CALDONE,
		.rxcmd2trntrm	= ADI_ADRV906X_C2C_S2P_RXCMD2TRNTRM,
		.rxcmd2ld	= ADI_ADRV906X_C2C_S2P_RXCMD2LD,
		.lddur		= ADI_ADRV906X_C2C_S2P_LDDUR,
		.mtchdur	= ADI_ADRV906X_C2C_S2P_MTCHDUR,
		.swbcktrm	= ADI_ADRV906X_C2C_S2P_SWBCKTRM,
		.blktrf		= ADI_ADRV906X_C2C_S2P_BLKTRF,
	},
};

bool adrv906x_c2c_enable(void)
{
	adi_c2cc_init(C2CC_BASE, SEC_C2CC_BASE, C2C_MODE_NORMAL);
	return adi_c2cc_enable();
}

bool adrv906x_c2c_enable_high_speed(void)
{
	return adi_c2cc_enable_high_speed(&adrv906x_c2c_training_params);
}

void adrv906x_release_secondary_reset(void)
{
	/* Hardware must be held in reset for at least 50ns for it to take effect.
	 * Explicitly do this here in software and do not rely on on-chip or on-board pull downs.
	 */
	mmio_write_32(CLK_CTL + SECONDARY_CTRL_OFFSET, 0x0);
	mdelay(1);

	/* Release reset */
	mmio_write_32(CLK_CTL + SECONDARY_CTRL_OFFSET, 0x1);
}

void adrv906x_activate_secondary_reset(void)
{
	mmio_write_32(CLK_CTL + SECONDARY_CTRL_OFFSET, 0x0);
}

int adrv906x_enable_secondary_tile(void)
{
	uint64_t timeout;

	/* Release secondary reset */
	adrv906x_release_secondary_reset();

	/* Wait 5ms between releasing reset and enabling the bridge */
	mdelay(5);

	/* Enable C2C bridge */
	adrv906x_c2c_enable();

	/* Wait for the secondary's TE to report it is ready for host boot,
	 * which indicates it has completed all platform-related hardware init.
	 */
	timeout = timeout_init_us(SECONDARY_TE_HOST_BOOT_TIMEOUT_US);
	while (!adi_enclave_is_host_boot_ready(SEC_TE_MAILBOX_BASE)) {
		if (timeout_elapsed(timeout)) {
			plat_warn_message("Timed out waiting for secondary TE to be ready.");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

int adrv906x_load_secondary_image(void)
{
	plat_sec_boot_cfg_t *boot_cfg_ptr;

	/* Ensure secondary TE is ready for host boot */
	if (!adi_enclave_is_host_boot_ready(SEC_TE_MAILBOX_BASE))
		return -ENXIO;

	/* Load the secondary image */
	memcpy16((void *)PLAT_SEC_IMAGE_DST_ADDR, (void *)PLAT_SEC_IMAGE_SRC_ADDR, PLAT_SEC_IMAGE_SIZE);
	flush_dcache_range((uintptr_t)PLAT_SEC_IMAGE_DST_ADDR, PLAT_SEC_IMAGE_SIZE);

	if (plat_get_secondary_linux_enabled()) {
		/* Setup the secondary boot config params */
		boot_cfg_ptr = (plat_sec_boot_cfg_t *)PLAT_SEC_BOOT_CFG_ADDR;
		boot_cfg_ptr->kernel_cfg_addr = plat_get_secondary_dram_base();
		boot_cfg_ptr->syscnt_freq = clk_get_freq(CLK_CTL, CLK_ID_TIMER);
		boot_cfg_ptr->uart_clk_freq = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
		boot_cfg_ptr->magic = PLAT_SEC_BOOT_MAGIC;
		flush_dcache_range((uintptr_t)boot_cfg_ptr, sizeof(*boot_cfg_ptr));
	} else {
		/* Tell TE we are done loading the special AppPack and can enable the mailbox */
		mmio_write_8(SEC_A55_SYS_CFG + SECONDARY_TE_HOST_BOOT_ENABLE, 0x1);

		/* Give time for the mailbox to initialize */
		mdelay(5);

		/* Check if mailbox is now initialized on the secondary */
		adi_enclave_mailbox_init(SEC_TE_MAILBOX_BASE);
	}

	return 0;
}

struct adi_c2cc_training_settings *adrv906x_c2cc_get_training_settings(void)
{
	return &adrv906x_c2c_training_params;
}
