/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "adrv906x_clkrst_def.h"
#include "clkgen_regmap.h"
#include "clk_switch_phases.h"

#define TCS_DELAY_TIME_US       (10U)

/* Maximum supported clock driver instances */
#define MAX_CLK_INST 2

/* TIMER has a constant /32 */
#define TIMER_CLK_DIV 32U

/* WDT has a constant /32 */
#define WDT_CLK_DIV 32U

/* MMC Card Clock Divider Values */
#define MMC_CARD_CLK_DIV_FOR_ROSC        (1U)
#define MMC_CARD_CLK_DIV_FOR_DEVCLK      (0U)
#define MMC_CARD_CLK_DIV_FOR_CLKPLL      (4U)

/* DDR clock has seven supported frequencies*/
#define DDR_CLOCK_FREQS 7U
#define DDR_CLOCK_DIV1_MASK 0xC
#define DDR_CLOCK_DIV2_MASK 0x3
#define DDR_CLOCK_DIV1_SHIFT 2
#define DDR_CLOCK_DIV2_SHIFT 0

/* HSDIG freq when running on clkpll
 *
 * NOTE: This is technically configured through a series of dividers
 * that divide down the 7G or 11G PLL clock to 983MHz. These dividers
 * were historically handled by the pll driver (drivers/adi/adrv906x/legacy/pll/pll.c)
 * and we do not wish to touch that legacy driver. Because of this, we assume
 * that driver has configured these dividers appropriately such that the HSDIG clock
 * frequency as seen by this block is 983MHz.
 */
#define HSDIG_ON_CLKPLL_HZ (983040000ULL)

typedef struct {
	uintptr_t baseaddr;
	uintptr_t clkpll_addr;
	uintptr_t dig_core_addr;
	uint64_t ddr_freq;
	uint64_t src_freqs[CLK_SRC_NUM];
	clk_switch_hook_t pre_switch_hook;
	clk_switch_hook_t post_switch_hook;
} clk_data_t;

typedef struct {
	uint64_t freq;
	uint8_t div1;
	uint8_t div2;
} ddr_div_t;


static int num_instances = 0;
static clk_data_t clk_data[MAX_CLK_INST] = { 0 };
static char *id_name_tbl[CLK_ID_NUM] = {
	"A55Core",
	"SYSCLK",
	"HSDIGCLK",
	"TIMER",
	"WDT",
	"DDR",
	"EMMC"
};
static char *src_name_tbl[CLK_SRC_NUM] = {
	"CLKPLL",
	"DEVCLK",
	"ROSC"
};

static ddr_div_t ddr_lookup[DDR_CLOCK_FREQS] = {
	{ 1600000000, 2u, 0u },         /*Encoded values of /5,/1 */
	{ 1466000000, 1u, 1u },         /*Encoded values of /3,/2, same as dividers for 1333MHz due to analog limitations */
	{ 1333000000, 1u, 1u },         /*Encoded values of /3,/2 */
	{ 1200000000, 0u, 3u },         /*Encoded values of /2,/4, same as dividers for 1066MHz due to analog limitations */
	{ 1066000000, 0u, 3u },         /*Encoded values of /2,/4 */
	{ 933000000,  2u, 1u },         /*Encoded values of /5,/2, same as dividers for 800MHz due to analog limitations */
	{ 800000000,  2u, 1u },         /*Encoded values of /5,/2 */
};

/* Set divider with a specific value for clk_src and clk_id */
static void clk_set_div(const uintptr_t baseaddr,
			int div_cnt,
			const clk_src_t clk_src,
			const clk_id_t clk_id)
{
	uint32_t clk_ctrl = 0U;
	uint32_t shift = 0U;
	uint32_t mask = 0U;

	switch (clk_src) {
	case CLK_SRC_ROSC:
		if (clk_id == CLK_ID_SYSCLK) {
			shift = SYSCLK_DIV_ROSC_SHIFT;
			mask = SYSCLK_DIV_ROSC_MASK;
		} else if (clk_id == CLK_ID_CORE) {
			shift = CORECLK_DIV_ROSC_SHIFT;
			mask = CORECLK_DIV_ROSC_MASK;
		}
		break;

	case CLK_SRC_DEVCLK:
		if (clk_id == CLK_ID_SYSCLK) {
			shift = SYSCLK_DIV_DEVCLK_SHIFT;
			mask = SYSCLK_DIV_DEVCLK_MASK;
		} else if (clk_id == CLK_ID_CORE) {
			shift = CORECLK_DIV_DEVCLK_SHIFT;
			mask = CORECLK_DIV_DEVCLK_MASK;
		}
		break;

	case CLK_SRC_CLKPLL:
		if (clk_id == CLK_ID_SYSCLK) {
			shift = SYSCLK_DIV_PLL_SHIFT;
			mask = SYSCLK_DIV_PLL_MASK;
		} else if (clk_id == CLK_ID_CORE) {
			shift = CORECLK_DIV_PLL_SHIFT;
			mask = CORECLK_DIV_PLL_MASK;
		}
		break;

	default:
		WARN("Cannot set clk divider clk_src %d is not supported\n", clk_src);
		return;
	}

	div_cnt <<= shift;
	clk_ctrl = mmio_read_32(baseaddr + CLK_CTRL_OFFSET);
	clk_ctrl &= ~mask;
	clk_ctrl |= div_cnt;
	mmio_write_32(baseaddr + CLK_CTRL_OFFSET, clk_ctrl);
}

/* Get divider for clk_src and clk_id */
static uint32_t clk_get_div(const uintptr_t baseaddr,
			    const clk_src_t clk_src,
			    const clk_id_t clk_id)
{
	uint32_t clk_ctrl = 0U;
	uint32_t div_cnt = 0U;
	uint32_t shift = 0U;
	uint32_t mask = 0U;
	uint32_t divider = 0U;

	switch (clk_src) {
	case CLK_SRC_ROSC:
		if (clk_id == CLK_ID_SYSCLK) {
			mask = SYSCLK_DIV_ROSC_MASK;
			shift = SYSCLK_DIV_ROSC_SHIFT;
		} else if (clk_id == CLK_ID_CORE) {
			mask = CORECLK_DIV_ROSC_MASK;
			shift = CORECLK_DIV_ROSC_SHIFT;
		}
		break;

	case CLK_SRC_DEVCLK:
		if (clk_id == CLK_ID_SYSCLK) {
			mask = SYSCLK_DIV_DEVCLK_MASK;
			shift = SYSCLK_DIV_DEVCLK_SHIFT;
		} else if (clk_id == CLK_ID_CORE) {
			mask = CORECLK_DIV_DEVCLK_MASK;
			shift = CORECLK_DIV_DEVCLK_SHIFT;
		}
		break;

	case CLK_SRC_CLKPLL:
		if (clk_id == CLK_ID_SYSCLK) {
			mask = SYSCLK_DIV_PLL_MASK;
			shift = SYSCLK_DIV_PLL_SHIFT;
		} else if (clk_id == CLK_ID_CORE) {
			mask = CORECLK_DIV_PLL_MASK;
			shift = CORECLK_DIV_PLL_SHIFT;
		}
		break;

	default:
		return 1;
	}

	clk_ctrl = mmio_read_32(baseaddr + CLK_CTRL_OFFSET);
	div_cnt = clk_ctrl & mask;
	div_cnt >>= shift;
	divider = 0x1 << div_cnt;

	return divider;
}

/* Get the clock source for the specific instance in the clk_data[] table */
static clk_src_t get_clk_src(const int inst_idx)
{
	uint32_t clk_sw_ctrl;
	uintptr_t baseaddr = clk_data[inst_idx].baseaddr;

	clk_sw_ctrl = mmio_read_32(baseaddr + CLK_SW_CTRL_OFFSET);
	if (clk_sw_ctrl & CLK_SW_SEL_ROSC_MASK)
		return CLK_SRC_ROSC;
	else if (clk_sw_ctrl & CLK_SW_SEL_DEVCLK_MASK)
		return CLK_SRC_DEVCLK;
	else
		return CLK_SRC_CLKPLL;
}

/* Get the clock_data structure associated with the provided base address */
static int get_clk_data(const uintptr_t baseaddr)
{
	int i = 0;
	int idx = -1;

	for (i = 0; i < MAX_CLK_INST; i++) {
		if (clk_data[i].baseaddr == baseaddr) {
			idx = i;
			break;
		}
	}

	return idx;
}

/* Initialize the clock driver for the specified base address and clock PLL */
void clk_init(const uintptr_t baseaddr, const uintptr_t clkpll_addr, const uintptr_t dig_core_addr, clk_switch_hook_t pre_switch_hook, clk_switch_hook_t post_switch_hook)
{
	if (num_instances >= MAX_CLK_INST) {
		WARN("Cannot register more than %d clock instances\n", MAX_CLK_INST);
	} else {
		if (get_clk_data(baseaddr) != -1) {
			WARN("Skipping clock init. %p already registered.\n", (void *)baseaddr);
		} else {
			clk_data[num_instances].baseaddr = baseaddr;
			clk_data[num_instances].clkpll_addr = clkpll_addr;
			clk_data[num_instances].dig_core_addr = dig_core_addr;
			clk_data[num_instances].pre_switch_hook = pre_switch_hook;
			clk_data[num_instances].post_switch_hook = post_switch_hook;
			num_instances++;
		}
	}
}

/* Get clock frequency for the specified clock ID */
uint64_t clk_get_freq(const uintptr_t baseaddr, const clk_id_t clk_id)
{
	clk_src_t clk_src = 0U;
	int inst_idx = 0U;

	/* Get a handle to the current clock_data struct */
	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot get clock freq. %p is not registered.\n", (void *)baseaddr);
		return 0U;
	}

	/* Get the current clock source, then get the freq for this clock ID on that source */
	if (clk_id == CLK_ID_DDR)
		clk_src = CLK_SRC_CLKPLL; /* DDR gets clock source only from CLKPLL, so we force the src to be CLKPLL for getting/setting DDR clock frequencies */
	else
		clk_src = get_clk_src(inst_idx);
	return clk_get_freq_by_src(baseaddr, clk_id, clk_src);
}

/* Get clock frequency for the specified clock ID, from the specified source */
uint64_t clk_get_freq_by_src(const uintptr_t baseaddr, const clk_id_t clk_id, const clk_src_t clk_src)
{
	int inst_idx = 0;
	uint32_t divider = 0U;
	uint64_t clk_freq = 0U;
	uint64_t hsdig_freq = 0U;
	uint64_t sysclk_freq = 0U;
	uint8_t div1, div2, combined_div;
	uintptr_t pll_addr;
	unsigned int i;

	/* Get a handle to the current clock_data struct */
	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot get clock freq. %p is not registered.\n", (void *)baseaddr);
		return 0U;
	}

	/* Calculate the HSDIG freq using the specified source */
	hsdig_freq = clk_data[inst_idx].src_freqs[clk_src];

	if (hsdig_freq == 0U) {
		WARN("%s freq is unknown. Cannot calc %s freq.\n", src_name_tbl[clk_src], id_name_tbl[clk_id]);
		return 0U;
	}

	/* If sourced from clkpll, use the frequency assumed to be set by the PLL driver.
	 * See note above where HSDIG_ON_CLKPLL_HZ is defined
	 */
	if (clk_src == CLK_SRC_CLKPLL)
		hsdig_freq = HSDIG_ON_CLKPLL_HZ;

	/* Calculate the freq of the specified clock ID */
	switch (clk_id) {
	case CLK_ID_CORE:
		/* Calculate the divider */
		divider = clk_get_div(baseaddr, clk_src, CLK_ID_CORE);
		clk_freq = hsdig_freq / divider;
		break;
	case CLK_ID_SYSCLK:
		/* Calculate the divider */
		divider = clk_get_div(baseaddr, clk_src, CLK_ID_SYSCLK);
		sysclk_freq = hsdig_freq / divider;
		clk_freq = sysclk_freq;
		break;
	case CLK_ID_HSDIGCLK:
		/* the hsdig clock is fetched */
		clk_freq = hsdig_freq;
		break;
	case CLK_ID_TIMER:
		clk_freq = hsdig_freq / TIMER_CLK_DIV;
		break;
	case CLK_ID_WDT:
		/* Calculate the divider */
		divider = clk_get_div(baseaddr, clk_src, CLK_ID_SYSCLK);
		sysclk_freq = hsdig_freq / divider;
		clk_freq = sysclk_freq / WDT_CLK_DIV;
		break;
	case CLK_ID_DDR:
		/*Calculate the dividers */
		if (clk_src != CLK_SRC_CLKPLL) {
			WARN("Invalid clk source. Only valid source for DDR is CLK PLL\n");
			break;
		}
		pll_addr = clk_data[inst_idx].clkpll_addr;
		combined_div = mmio_read_8(pll_addr + CLKGEN_DIGCORE_3_OFFSET);
		div1 = (combined_div & DDR_CLOCK_DIV1_MASK) >> DDR_CLOCK_DIV1_SHIFT;
		div2 = (combined_div & DDR_CLOCK_DIV2_MASK) >> DDR_CLOCK_DIV2_SHIFT;
		for (i = 0; i < DDR_CLOCK_FREQS; i++) {
			if ((ddr_lookup[i].div1 == div1) && (ddr_lookup[i].div2 == div2) && (ddr_lookup[i].freq == clk_data[inst_idx].ddr_freq)) {
				clk_freq = ddr_lookup[i].freq;
				break;
			}
		}
		break;
	case CLK_ID_EMMC:
		if (CLK_SRC_CLKPLL == clk_src)
			divider = MMC_CARD_CLK_DIV_FOR_CLKPLL;
		else if (CLK_SRC_DEVCLK == clk_src)
			divider = MMC_CARD_CLK_DIV_FOR_DEVCLK;
		else
			divider = MMC_CARD_CLK_DIV_FOR_ROSC;
		clk_freq = hsdig_freq / (divider + 1);
		break;
	default:
		clk_freq = 0U;
		break;
	}

	return clk_freq;
}

/* Set the clock frequency of the specified clock ID, for the specified source */
void clk_set_freq_by_src(const uintptr_t baseaddr, const clk_id_t clk_id, const uint64_t freq, const clk_src_t clk_src)
{
	uint64_t tgt_clk_freq = 0U;
	uint64_t hsdig_freq = 0U;
	int inst_idx = 0;
	int div_cnt = 0;

	/* Get a handle to the current clock_data struct */
	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot get clock freq. %p is not registered.\n", (void *)baseaddr);
		return;
	}

	/* Calculate the HSDIG freq using the specified source */
	hsdig_freq = clk_data[inst_idx].src_freqs[clk_src];

	if (hsdig_freq == 0U) {
		WARN("%s freq is unknown. Cannot calc %s freq.\n", src_name_tbl[clk_src], id_name_tbl[clk_id]);
		return;
	}

	/* If sourced from clkpll, use the frequency assumed to be set by the PLL driver.
	 * See note above where HSDIG_ON_CLKPLL_HZ is defined
	 */
	if (clk_src == CLK_SRC_CLKPLL)
		hsdig_freq = HSDIG_ON_CLKPLL_HZ;

	/* Set the divider for the nearest freq of
	 * the specified clock ID
	 */
	if ((clk_id == CLK_ID_SYSCLK) || (clk_id == CLK_ID_CORE)) {
		tgt_clk_freq = hsdig_freq;
	} else {
		WARN("Unsupported clk_id %d\n", clk_id);
		return;
	}

	/* Choose the closest target freq. It is equal to or
	 * just below the freq requested.
	 */
	while ((int64_t)(tgt_clk_freq - freq) > 0) {
		tgt_clk_freq >>= 1;
		div_cnt++;
	}
	clk_set_div(baseaddr, div_cnt, clk_src, clk_id);
}

void clk_set_freq(const uintptr_t baseaddr, const clk_id_t clk_id, const uint64_t freq)
{
	unsigned int i;
	int inst_idx;
	uintptr_t pll_addr;
	uint8_t reg;

	/* Get a handle to the current clock_data struct */
	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot get clock freq. %p is not registered.\n", (void *)baseaddr);
		return;
	}

	pll_addr = clk_data[inst_idx].clkpll_addr;

	/* Set the DDR clock frequency */
	if (clk_id == CLK_ID_DDR) {
		for (i = 0; i < DDR_CLOCK_FREQS; i++) {
			if (ddr_lookup[i].freq == freq) {
				clk_data[inst_idx].ddr_freq = freq;
				reg = mmio_read_8(pll_addr + CLKGEN_DIGCORE_3_OFFSET);
				reg &= ~(DDR_CLOCK_DIV1_MASK | DDR_CLOCK_DIV2_MASK);
				reg |= ((ddr_lookup[i].div1 << DDR_CLOCK_DIV1_SHIFT) | (ddr_lookup[i].div2 << DDR_CLOCK_DIV2_SHIFT)) & (DDR_CLOCK_DIV1_MASK | DDR_CLOCK_DIV2_MASK);
				mmio_write_8(pll_addr + CLKGEN_DIGCORE_3_OFFSET, reg);
				break;
			}
		}

		if (i == DDR_CLOCK_FREQS)
			WARN("Unsupported frequency");
	} else {
		WARN("Unsupported clock id");
	}
}

void clk_enable_clock(const uintptr_t baseaddr, const clk_id_t clk_id)
{
	int inst_idx;
	uint32_t clk_reg;

	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot enable clock. %p is not registered.\n", (void *)baseaddr);
		return;
	}

	if (clk_id == CLK_ID_DDR) {  /* Enable the DDR clocks */
		clk_reg = mmio_read_8(clk_data[inst_idx].clkpll_addr + CLKGEN_DIGCORE_1_OFFSET);
		clk_reg |= DIGCORE_DIV1_EN_MASK;
		mmio_write_8(clk_data[inst_idx].clkpll_addr + CLKGEN_DIGCORE_1_OFFSET, clk_reg);
		udelay(1);
		mmio_write_32(baseaddr + DDR_CLK_CTRL_OFFSET, (DDRC_CORE_CLK_ENABLE_MASK | DDR_CTL_ARM_CLK_ENABLE_MASK | DDR_PHY_ARM_CLK_ENABLE_MASK));
	} else {
		WARN("Unsupported clock id");
	}
}

void clk_disable_clock(const uintptr_t baseaddr, const clk_id_t clk_id)
{
	int inst_idx;
	uint32_t clk_reg;

	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot enable clock. %p is not registered.\n", (void *)baseaddr);
		return;
	}

	if (clk_id == CLK_ID_DDR) {  /* Disable the DDR clocks */
		mmio_write_32(baseaddr + DDR_CLK_CTRL_OFFSET, ~(DDRC_CORE_CLK_ENABLE_MASK | DDR_CTL_ARM_CLK_ENABLE_MASK | DDR_PHY_ARM_CLK_ENABLE_MASK));
		clk_reg = mmio_read_8(clk_data[inst_idx].clkpll_addr + CLKGEN_DIGCORE_1_OFFSET);
		clk_reg &= ~DIGCORE_DIV1_EN_MASK;
		mmio_write_8(clk_data[inst_idx].clkpll_addr + CLKGEN_DIGCORE_1_OFFSET, clk_reg);
		udelay(1);
	} else {
		WARN("Unsupported clock id");
	}
}

clk_src_t clk_get_src(const uintptr_t baseaddr)
{
	int inst_idx;

	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot get clock source. %p is not registered.\n", (void *)baseaddr);
		return CLK_SRC_NUM;
	}
	return get_clk_src(inst_idx);
}

void clk_set_src_pre_switch(const uintptr_t baseaddr, const clk_src_t clk_src)
{
	uint32_t otp_clk_ctrl;
	int inst_idx;

	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1) {
		WARN("Cannot set clock src. %p is not registered.\n", (void *)baseaddr);
		return;
	}

	if (clk_src >= CLK_SRC_NUM) {
		WARN("Invalid clock source: %d\n", (int)clk_src);
		return;
	}

	if (get_clk_src(inst_idx) != clk_src)
		INFO("Switching clock source from %s to %s\n", src_name_tbl[get_clk_src(inst_idx)], src_name_tbl[clk_src]);

	/* Call platform pre-switch hook, if registered */
	if (clk_data[inst_idx].pre_switch_hook != NULL)
		clk_data[inst_idx].pre_switch_hook();

	/* Disable OTP clk first. make sure there is no OTP IO. */
	otp_clk_ctrl = mmio_read_32(baseaddr + OTP_CLK_CTRL_OFFSET);
	otp_clk_ctrl &= ~(OTP_CLK_ENABLE_MASK);
	mmio_write_32(baseaddr + OTP_CLK_CTRL_OFFSET, otp_clk_ctrl);
}

void clk_set_src_switch(const uintptr_t baseaddr, const clk_src_t clk_src, bool use_device_clk_until_mcs)
{
	int inst_idx;
	uint32_t clk_sw_ctrl;
	uint32_t rosc_ctrl;
	uintptr_t addr;

	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1)
		return;

	if (clk_src >= CLK_SRC_NUM)
		return;

	addr = baseaddr + CLK_SW_CTRL_OFFSET;
	clk_sw_ctrl = mmio_read_32(addr);
	if (clk_src == CLK_SRC_ROSC) {
		clk_sw_ctrl &= ~CLK_SW_SEL_DEVCLK_MASK;
		clk_sw_ctrl |= CLK_SW_SEL_ROSC_MASK;

		/* enable rosc */
		rosc_ctrl = mmio_read_32(baseaddr + ROSC_CTRL_OFFSET);
		rosc_ctrl |= ROSC_CLK_ENABLE_MASK;
		mmio_write_32(baseaddr + ROSC_CTRL_OFFSET, rosc_ctrl);
		/* NOTE: design requires delay >= 12ns */
		udelay(1);
	} else if (clk_src == CLK_SRC_DEVCLK) {
		clk_sw_ctrl |= CLK_SW_SEL_DEVCLK_MASK;
		clk_sw_ctrl &= ~CLK_SW_SEL_ROSC_MASK;
	} else {
		clk_sw_ctrl &= ~CLK_SW_SEL_DEVCLK_MASK;
		clk_sw_ctrl &= ~CLK_SW_SEL_ROSC_MASK;
	}

	if (use_device_clk_until_mcs) {
		if (clk_src == CLK_SRC_CLKPLL) {
			clk_sw_ctrl &= ~CLK_SW_SEL_DEVCLK_MASK;
			clk_sw_ctrl &= ~CLK_SW_SEL_ROSC_MASK;
			clk_sw_ctrl |= CLK_SW_SEL_DEVCLK_UNTIL_MCS_MASK;
		} else {
			WARN("Invalid clock source to enable use_device_clk_until_mcs\n");
			assert(0);
		}
	} else {
		clk_sw_ctrl &= ~CLK_SW_SEL_DEVCLK_UNTIL_MCS_MASK;
	}

	__asm __volatile("dsb sy\n\t"
			 "str %w[val], [%[dst]]\n\t"
			 "dsb sy\n\t"
			 ::[val] "r" (clk_sw_ctrl), [dst] "r" (addr)
			 : "memory");
}

void clk_set_src_post_switch(const uintptr_t baseaddr, const clk_src_t clk_src)
{
	int inst_idx;
	uint32_t otp_clk_ctrl;
	uint32_t rosc_ctrl;
	uint32_t mmc_card_clk_div;
	uint32_t abh_clk_en;

	inst_idx = get_clk_data(baseaddr);
	if (inst_idx == -1)
		return;

	if (clk_src >= CLK_SRC_NUM)
		return;

	/* Enable OTP clk */
	otp_clk_ctrl = mmio_read_32(baseaddr + OTP_CLK_CTRL_OFFSET);
	otp_clk_ctrl |= (OTP_CLK_ENABLE_MASK);
	mmio_write_32(baseaddr + OTP_CLK_CTRL_OFFSET, otp_clk_ctrl);

	if (clk_src != CLK_SRC_ROSC) {
		rosc_ctrl = mmio_read_32(baseaddr + ROSC_CTRL_OFFSET);
		rosc_ctrl &= ~ROSC_CLK_ENABLE_MASK;
		mmio_write_32(baseaddr + ROSC_CTRL_OFFSET, rosc_ctrl);
	}

	/* Enable eMMC/SD ABH clocks, then configure card clock dividers
	 * depending on the clock source
	 */
	abh_clk_en = mmio_read_32(baseaddr + EMMC_ABH_CLK_EN);
	abh_clk_en |= ABH_CLK_EN_MASK;
	mmio_write_32(baseaddr + EMMC_ABH_CLK_EN, abh_clk_en);
	abh_clk_en = mmio_read_32(baseaddr + SD_ABH_CLK_EN);
	abh_clk_en |= ABH_CLK_EN_MASK;
	mmio_write_32(baseaddr + SD_ABH_CLK_EN, abh_clk_en);

	switch (clk_src) {
	case CLK_SRC_ROSC:   mmc_card_clk_div = MMC_CARD_CLK_DIV_FOR_ROSC;   break;
	case CLK_SRC_DEVCLK: mmc_card_clk_div = MMC_CARD_CLK_DIV_FOR_DEVCLK; break;
	default:             mmc_card_clk_div = MMC_CARD_CLK_DIV_FOR_CLKPLL; break;
	}
	mmio_write_32(baseaddr + EMMC_CARD_CLK_DIV_OFFSET, mmc_card_clk_div);
	mmio_write_32(baseaddr + SD_CARD_CLK_DIV_OFFSET, mmc_card_clk_div);

	/* Call platform post-switch hook, if registered */
	if (clk_data[inst_idx].post_switch_hook != NULL)
		clk_data[inst_idx].post_switch_hook();
}

void clk_set_src(const uintptr_t baseaddr, const clk_src_t clk_src)
{
	clk_set_src_pre_switch(baseaddr, clk_src);
	clk_set_src_switch(baseaddr, clk_src, false);
	clk_set_src_post_switch(baseaddr, clk_src);
}

/* Notify this driver of a change in frequency for the specified source */
void clk_notify_src_freq_change(const uintptr_t baseaddr, clk_src_t clk_src, const uint64_t freq)
{
	int idx;

	idx = get_clk_data(baseaddr);

	if (idx == -1)
		WARN("Ignoring freq change notification. %p is not registered.\n", (void *)baseaddr);
	else
		clk_data[idx].src_freqs[clk_src] = freq;
}

/* Print clock info to stdout */
void clk_print_info(const uintptr_t baseaddr)
{
	int id_idx;
	int inst_idx;
	int src_idx;
	clk_data_t *p_clk_data;

	inst_idx = get_clk_data(baseaddr);

	if (inst_idx == -1) {
		WARN("Can't print info for unregistered clock  %p.\n", (void *)baseaddr);
	} else {
		p_clk_data = &(clk_data[inst_idx]);
		INFO("Clock info for %p\n", (void *)p_clk_data->baseaddr);
		for (src_idx = 0; src_idx < CLK_SRC_NUM; src_idx++)
			INFO("%s freq: %lu\n", src_name_tbl[src_idx], p_clk_data->src_freqs[src_idx]);
		INFO("Current source: %s\n", src_name_tbl[get_clk_src(inst_idx)]);
		for (src_idx = 0; src_idx < CLK_SRC_NUM; src_idx++)
			for (id_idx = 0; id_idx < CLK_ID_NUM; id_idx++)
				if (id_idx == CLK_ID_DDR) {
					if (src_idx == CLK_SRC_CLKPLL)
						INFO("%s-on-%s freq: %lu\n", id_name_tbl[id_idx], src_name_tbl[CLK_SRC_CLKPLL], clk_get_freq_by_src(p_clk_data->baseaddr, id_idx, CLK_SRC_CLKPLL));
				} else {
					INFO("%s-on-%s freq: %lu\n", id_name_tbl[id_idx], src_name_tbl[src_idx], clk_get_freq_by_src(p_clk_data->baseaddr, id_idx, src_idx));
				}
	}
}

/* Clock initializations for devclk */
void clk_init_devclk(const uintptr_t baseaddr, const uintptr_t dig_core_addr)
{
	uint8_t reg_byte;
	uint32_t reg;
	uint32_t otp_clk_ctrl;

	/* Set digital_clock_divider_sync_en.
	 * Forces all clock dividers in the system to synchronize.
	 */
	reg = mmio_read_32(baseaddr + CLK_SW_CTRL_OFFSET);
	mmio_write_32(baseaddr + CLK_SW_CTRL_OFFSET, reg | DIGITAL_CLOCK_DIVIDER_SYNC_EN_MASK);
	udelay(40);

	/* Ensure both mbias blocks are enabled */
	reg_byte = mmio_read_8(dig_core_addr + CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN0);
	mmio_write_8(dig_core_addr + CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN0, reg_byte &= ~(MBIAS_IGEN_PD_MASK));
	reg_byte = mmio_read_8(dig_core_addr + CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN1);
	mmio_write_8(dig_core_addr + CORE_MASTER_BIAS_CTRL_MBIAS_IGEN_PWRDWN1, reg_byte &= ~(MBIAS_IGEN_PD_MASK));

	/* Ensure DEVCLK_BUFFER_ENABLE is set */
	reg_byte = mmio_read_8(dig_core_addr + DEVCLK_BUFFER_CTRL_OFFSET);
	mmio_write_8(dig_core_addr + DEVCLK_BUFFER_CTRL_OFFSET, reg_byte | DEVCLK_BUFFER_ENABLE_MASK);
	udelay(30);

	/* Disable OTP clk first. make sure there is no OTP IO. */
	otp_clk_ctrl = mmio_read_32(baseaddr + OTP_CLK_CTRL_OFFSET);
	otp_clk_ctrl &= ~(OTP_CLK_ENABLE_MASK);
	mmio_write_32(baseaddr + OTP_CLK_CTRL_OFFSET, otp_clk_ctrl);

	/* Ensure DEVCLK_DIVIDER_MCS_RESETB is set to 1 */
	reg_byte = mmio_read_8(dig_core_addr + DEVCLK_DIVIDER_CONTROL_OFFSET);
	mmio_write_8(dig_core_addr + DEVCLK_DIVIDER_CONTROL_OFFSET, reg_byte | DEVCLK_DIVIDER_MCS_RESETB_MASK);
	udelay(10);

	/* Enable OTP clk */
	otp_clk_ctrl = mmio_read_32(baseaddr + OTP_CLK_CTRL_OFFSET);
	otp_clk_ctrl |= (OTP_CLK_ENABLE_MASK);
	mmio_write_32(baseaddr + OTP_CLK_CTRL_OFFSET, otp_clk_ctrl);
}
