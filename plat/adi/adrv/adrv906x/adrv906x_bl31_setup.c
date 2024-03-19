/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <plat/common/platform.h>

#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adi_spu.h>

#include <platform_def.h>
#include <plat_interrupts.h>

#include <adrv906x_board.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_el3_int_handlers.h>
#include <adrv906x_gpint.h>
#include <adrv906x_mmap.h>
#include <adrv906x_tsgen.h>
#include <adrv906x_spu_def.h>

static void clk_switch_hook(void)
{
	/* Switching clocks in a running system is more extensive
	 * and is not supported.
	 */
	ERROR("Clock switch cannot be performed in BL31\n");
	plat_error_handler(-EPERM);
}

void plat_bl31_early_setup(void)
{
	uint32_t clkpll_setting = 0U;
	uint64_t clkpll_freq = 0ULL;

	/* Setup the device profile. Called here so that plat_console_boot_init will have the freq of the clk driving the UART*/
	plat_dprof_init();

	clkpll_setting = plat_get_clkpll_freq_setting();
	if (clkpll_setting == SETTING_CLKPLL_FREQ_7G)
		clkpll_freq = CLK_CLKPLL_FREQ_7GHZ;
	else
		clkpll_freq = CLK_CLKPLL_FREQ_11GHZ;

	/* Initialize the clock framework */
	clk_init(CLK_CTL, CLKPLL_BASE, DIG_CORE_BASE, clk_switch_hook, NULL);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_DEVCLK, DEVCLK_FREQ_DFLT);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_ROSC, ROSC_FREQ_DFLT);
	clk_notify_src_freq_change(CLK_CTL, CLK_SRC_CLKPLL, clkpll_freq);

	/* Enable CCS600_TSGEN timestamp generator module */
	plat_tsgen_enable(PLAT_TSGEN_BASE);
}

void plat_bl31_setup(void)
{
#if EL3_EXCEPTION_HANDLING == 0
	unsigned int flags = 0;
	set_interrupt_rm_flag(flags, NON_SECURE);
	register_interrupt_type_handler(INTR_TYPE_EL3, plat_interrupt_handler, flags);
#endif
	int err = 0;
	struct gpint_settings settings;

	/* the eMMC and/or SD were granted special MSEC permissions during BL2,
	 * but we should remove those permissions before BL31.
	 */
	adi_spu_disable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_EMMC0SLV);
	adi_spu_disable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_EMMC1SLV);
	adi_spu_disable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_QUAD_SPI_DMA_0);
	adi_spu_disable_msec(SPU_A55MMR_BASE, SPU_A55MMR_PERIPH_QUAD_SPI_DMA_1);

	/* Set GPINTs to default route to BL31 by setting route_nonsecure to 0 for each bit */
	settings.upper_word_route_nonsecure = 0;
	settings.lower_word_route_nonsecure = 0;
	adrv906x_gpint_set_routing(&settings);

	/* Set up handlers for each of the GPINTs */
	plat_request_intr_type_el3(IRQ_GP_INTERRUPT_SYNC_0, primary_gpint_handler);
	plat_request_intr_type_el3(IRQ_GP_INTERRUPT_SYNC_1, primary_gpint_handler);

	if (plat_get_dual_tile_enabled()) {
		err = plat_setup_secondary_mmap(true);
		if (err == 0) {
			/* Set up handlers for each of the GPINTs received on the Primary from the Secondary */
			plat_request_intr_type_el3(IRQ_C2C_OUT_HW_INTERRUPT_197, secondary_gpint_handler);
			plat_request_intr_type_el3(IRQ_GPINT_INTERRUPT_SECONDARY_TO_PRIMARY, secondary_gpint_handler);
		} else {
			ERROR("Failed to setup mmap for secondary tile %d\n", err);
			plat_set_dual_tile_disabled();
		}
	}

	/* Do board-specific setup */
	plat_board_bl31_setup();
}
