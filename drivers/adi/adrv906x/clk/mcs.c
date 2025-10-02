/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <adrv906x_board.h>
#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_gpint.h>
#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/pll.h>
#include <drivers/delay_timer.h>

#include "clk_switch_phases.h"
#include "mcs.h"

/*--------------------------------------------------------
 * DEFINES
 *------------------------------------------------------*/

#define FORCE_MCS_BYPASS        0

#define CLKPLL_MCS_COMPLETE     0x05
#define RFPLL_MCS_COMPLETE      0x04

#define MCS_COMPLETION_TIMEOUT_US       (50 * 1000)

typedef struct {
	uint8_t clkpll_freq_setting;
	uint8_t orx_adc_freq_setting;
	uint32_t branch_div;
	uint32_t root_clkdiv_div2;
	uint32_t root_clkdiv_fund;
	uint32_t digcore_clkdiv_thermcode;
	uint32_t digcore_intdiv_bincode;
	uint32_t ref_clk_divide_ratio;
	uint32_t tx_slice_analog_dac_clkdiv_thermcode;
	uint32_t tx_slice_analog_dac_clkdiv_div1_en;
	uint32_t rx_slice_clkdiv_thermcode;
	uint32_t rx_slice_clkdiv_div1_en;
	uint32_t orx_slice_clkdiv_thermcode;
	uint32_t orx_slice_clkdiv_div1_en;
	uint32_t tx_slice_tx_datapath_tx_dac_if_rd_ptr_init;
	uint32_t tx_slice_tx_datapath_tx_dac_if_wr_ptr_init;
	uint32_t tx_slice_tx_dig_tout_clk_divide_ratio;
	uint32_t rx_slice_adc_fifo_rd_ptr_init;
	uint32_t rx_slice_adc_fifo_wr_ptr_init;
	uint32_t orx_slice_orx_rd_ptr_init;
	uint32_t orx_slice_orx_wr_ptr_init;
	uint32_t rx_slice_ch_conf_9_rin_clk_divide_ratio;
	uint32_t rx_slice_ch_config_8_dp_filter_select;
	uint32_t rx_slice_ch_conf_8_fir2_in_clk_divide_ratio;
	uint32_t orx_slice_rin_clk_div_ratio;
	uint32_t tx_slice_analog_loopback_trm_clkdiv_thermcode;
	uint32_t tx_slic_analog_loopback_div1_en;
	uint32_t tx_slice_tx_datapath_filter_control_0_int3_enable;
	uint32_t orx_slice_orx_dig_stage1_mode;
} mcs_clk_dividers_config_t;

enum adrv906x_tile_type {
	ADRV906X_PRIMARY_TILE = 0,
	ADRV906X_SECONDARY_TILE,
	ADRV906X_TILE_TYPE_NUM,
};

/* Bitfield functions */
#define READ_BF_8BIT_REG   read8bf
#define WRITE_BF_8BIT_REG  write8bf
#define WRITE_BF_32BIT_REG write32bf

/*--------------------------------------------------------
 * GLOBALS
 *------------------------------------------------------*/

mcs_clk_dividers_config_t clk_divs_cfg[] =
{
	{ SETTING_CLKPLL_FREQ_7G,  SETTING_ORX_ADC_FREQ_3932M, 1, 0, 1, 3, 0, 0, 3, 0, 1, 0, 1, 0, 0, 7, 0, 0, 2, 0, 5, 0, 3, 0, 1, 3, 0, 0, 3 },
	{ SETTING_CLKPLL_FREQ_7G,  SETTING_ORX_ADC_FREQ_7864M, 1, 0, 1, 3, 0, 0, 3, 0, 1, 0, 0, 1, 0, 7, 0, 0, 2, 0, 5, 0, 3, 0, 0, 3, 0, 0, 3 },
	{ SETTING_CLKPLL_FREQ_11G, SETTING_ORX_ADC_FREQ_5898M, 2, 0, 1, 3, 1, 0, 3, 0, 1, 0, 0, 1, 0, 7, 0, 0, 2, 0, 5, 0, 1, 0, 0, 3, 0, 3, 0 },
	{ SETTING_CLKPLL_FREQ_11G, SETTING_ORX_ADC_FREQ_2949M, 2, 0, 1, 3, 1, 0, 3, 0, 1, 0, 1, 0, 0, 7, 0, 0, 2, 0, 5, 0, 1, 0, 1, 3, 0, 3, 0 },
};

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS
 *------------------------------------------------------*/
static uint32_t read8bf(volatile uint8_t *addr, uint8_t position, uint8_t mask)
{
	return (mmio_read_8((uintptr_t)addr) & mask) >> position;
}

static void write8bf(volatile uint8_t *addr, uint8_t position, uint8_t mask, uint8_t val)
{
	uint8_t reg = mmio_read_8((uintptr_t)addr) & ~mask;

	reg |= (val << position) & mask;
	mmio_write_8((uintptr_t)addr, reg);
}

static void write32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask, uint32_t val)
{
	uint32_t reg = mmio_read_32((uintptr_t)addr) & ~mask;

	reg |= (val << position) & mask;
	mmio_write_32((uintptr_t)addr, reg);
}

static bool current_clock_source_is_devclk(bool dual_tile)
{
	if (clk_get_src(CLK_CTL) != CLK_SRC_DEVCLK)
		return false;
	if (dual_tile && (clk_get_src(SEC_CLK_CTL) != CLK_SRC_DEVCLK))
		return false;
	return true;
}

static mcs_clk_dividers_config_t *getconfig(uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting)
{
	int number_of_configurations = sizeof(clk_divs_cfg) / sizeof(mcs_clk_dividers_config_t);

	for (int i = 0; i < number_of_configurations; i++)
		if ((clk_divs_cfg[i].clkpll_freq_setting == clkpll_freq_setting) && (clk_divs_cfg[i].orx_adc_freq_setting == orx_adc_freq_setting))
			return &clk_divs_cfg[i];
	return NULL;
}

static void enable_scaled_devclk_to_all_plls(enum adrv906x_tile_type tile)
{
	uintptr_t baseaddr;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = DIG_CORE_BASE;
	else
		baseaddr = SEC_DIG_CORE_BASE;

	WRITE_CORE_CLKPLL_REFPATH_PD(baseaddr, 0);
	WRITE_CORE_WEST_RFPLL_SYNTH_REFPATH_PD(baseaddr, 0);
	WRITE_CORE_EAST_RFPLL_SYNTH_REFPATH_PD(baseaddr, 0);
}

static void setup_reference_clock_divider_for_clkpll(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	uintptr_t baseaddr;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = CLKPLL_BASE;
	else
		baseaddr = SEC_CLKPLL_BASE;

	WRITE_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(baseaddr, config->ref_clk_divide_ratio);
}

static void setup_clkpll_core_digital_clock_root_divider(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	uintptr_t baseaddr;
	uint8_t value, offset, mask;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = CLKPLL_BASE;
	else
		baseaddr = SEC_CLKPLL_BASE;

	WRITE_PLL_MEM_MAP_ROOT_CLKDIV_DIV2(baseaddr, config->root_clkdiv_div2);
	WRITE_PLL_MEM_MAP_ROOT_CLKDIV_FUND(baseaddr, config->root_clkdiv_fund);

	WRITE_PLL_MEM_MAP_DIGCORE_CLKDIV_THERMCODE(baseaddr, config->digcore_clkdiv_thermcode);
	WRITE_PLL_MEM_MAP_DIGCORE_INTDIV_BINCODE(baseaddr, config->digcore_intdiv_bincode);

	/* 3e. branch_div CLKGEN_DIGCORE_3.clkgen_spares<7:6> */
	value = READ_PLL_MEM_MAP_CLKGEN_SPARES(baseaddr);
	offset = 6;
	mask = 0xC0;
	value &= ~mask;
	value |= (config->branch_div << offset) & mask;
	WRITE_PLL_MEM_MAP_CLKGEN_SPARES(baseaddr, value);

	/* 3f. branch_div_killclk CLKGEN_DIGCORE_3.clkgen_spares<5> = 0 */
	value = READ_PLL_MEM_MAP_CLKGEN_SPARES(baseaddr);
	offset = 5;
	mask = 0x20;
	value &= ~mask;
	value |= (0 << offset) & mask;
	WRITE_PLL_MEM_MAP_CLKGEN_SPARES(baseaddr, value);
}

/* Performs pre initialization before programming a CLK PLL */
static int clk_initialize_clk_pll_programming(bool secondary, uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting)
{
	enum adrv906x_tile_type tile;

	/* Get configuration */
	mcs_clk_dividers_config_t *config = getconfig(clkpll_freq_setting, orx_adc_freq_setting);

	if (!config) {
		ERROR("MCS: no available configuration for clkpll freq %d and orx_adc freq %d.\n", clkpll_freq_setting, orx_adc_freq_setting);
		return -1;
	}

	if (secondary)
		tile = ADRV906X_SECONDARY_TILE;
	else
		tile = ADRV906X_PRIMARY_TILE;

	/* 1 */
	enable_scaled_devclk_to_all_plls(tile);

	/* 2 */
	setup_reference_clock_divider_for_clkpll(tile, config);

	/* 3 */
	setup_clkpll_core_digital_clock_root_divider(tile, config);
	return 0;
}

/* Preforms pre initialization before programming an Ethernet PLL*/
static int clk_initialize_eth_pll_programming(bool secondary)
{
	uintptr_t a55_sys_cfg_base_addr;
	uintptr_t emac_common_base;

	if (secondary) {
		a55_sys_cfg_base_addr = SEC_A55_SYS_CFG;
		emac_common_base = SEC_EMAC_COMMON_BASE;
	} else {
		a55_sys_cfg_base_addr = A55_SYS_CFG;
		emac_common_base = EMAC_COMMON_BASE;
	}

	/* Workaround for ethernet PLL warm reset issue.
	 * The warm reset signal does not propagate to the ethernet PLL
	 * subsystem, so we must reset it here, before initializing it.
	 */
	WRITE_EMAC_COMMON_ETHPLL_MEM_MAP_RSTN(emac_common_base, 0x0);
	udelay(50);
	WRITE_EMAC_COMMON_ETHPLL_MEM_MAP_RSTN(emac_common_base, 0x1);

	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_DEVCLK_CONTROLS_ETH_DEVCLK_DIV_KILLCLK(a55_sys_cfg_base_addr, 0x0);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_DEVCLK_CONTROLS_ETH_DEVCLK_DIV_MCS_RST(a55_sys_cfg_base_addr, 0x0);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_DEVCLK_CONTROLS_ETH_DEVCLK_DIV_RB(a55_sys_cfg_base_addr, 0x1);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_DEVCLK_CONTROLS_ETH_DEVCLK_DIV_FUND(a55_sys_cfg_base_addr, 0x1);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_DEVCLK_CONTROLS_ETH_DEVCLK_DIV_RATIO(a55_sys_cfg_base_addr, 0x0);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_DEVCLK_CONTROLS_ETH_DEVICE_CLK_BUFFER_ENABLE(a55_sys_cfg_base_addr, 0x1);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH_REFCLK_CONTROLS_ETH_PLL_REFPATH_PD(a55_sys_cfg_base_addr, 0x0);
	return 0;
}

static int clock_pll_initialization(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	uintptr_t clkpll_baseaddr;
	uintptr_t dig_core_baseaddr;
	uintptr_t clk_ctl_baseaddr;
	PllSelName_e pll_sel_name;
	int err = 0;
	uint64_t clkpll_freq = 0ULL;

	if (tile == ADRV906X_PRIMARY_TILE) {
		clkpll_baseaddr = CLKPLL_BASE;
		dig_core_baseaddr = DIG_CORE_BASE;
		clk_ctl_baseaddr = CLK_CTL;
		pll_sel_name = PLL_CLKGEN_PLL;
	} else {
		clkpll_baseaddr = SEC_CLKPLL_BASE;
		dig_core_baseaddr = SEC_DIG_CORE_BASE;
		clk_ctl_baseaddr = SEC_CLK_CTL;
		pll_sel_name = PLL_SEC_CLKGEN_PLL;
	}

	if (config->clkpll_freq_setting == SETTING_CLKPLL_FREQ_7G)
		clkpll_freq = CLK_CLKPLL_FREQ_7GHZ;
	else
		clkpll_freq = CLK_CLKPLL_FREQ_11GHZ;

	/*
	 * Init the ClkPll driver.
	 */
	err = pll_clk_power_init(clkpll_baseaddr, dig_core_baseaddr, clkpll_freq, DEVCLK_FREQ_DFLT, pll_sel_name);
	if (err) {
		ERROR("Pll Init = %d\n", err);
		return -ENXIO;
	}

	/*
	 * Program the ClkPll.
	 */

	err = pll_program(clkpll_baseaddr, pll_sel_name);
	if (err) {
		ERROR("Pll program = %d\n", err);
		return -ENXIO;
	}

	/* Notify the clock framework of the clkpll freq */
	clk_notify_src_freq_change(clk_ctl_baseaddr, CLK_SRC_CLKPLL, clkpll_freq);

	return 0;
}

static void setup_rx_clkgen(const uintptr_t adc32_analog_regs_baseaddr, const uintptr_t rx_dig_baseaddr, mcs_clk_dividers_config_t *config)
{
	WRITE_TDR_DIG_MISC_CLKDIV_THERMCODE(adc32_analog_regs_baseaddr, config->rx_slice_clkdiv_thermcode);
	WRITE_TDR_DIG_MISC_DIV1_EN(adc32_analog_regs_baseaddr, config->rx_slice_clkdiv_div1_en);

	WRITE_RX_DIG_ADC_FIFO_RD_PTR_INIT(rx_dig_baseaddr, config->rx_slice_adc_fifo_rd_ptr_init);
	WRITE_RX_DIG_ADC_FIFO_WR_PTR_INIT(rx_dig_baseaddr, config->rx_slice_adc_fifo_wr_ptr_init);
	WRITE_RX_DIG_RIN_CLK_DIVIDE_RATIO(rx_dig_baseaddr, config->rx_slice_ch_conf_9_rin_clk_divide_ratio);
	WRITE_RX_DIG_RIN_CLK_ENABLE(rx_dig_baseaddr, 1);
	WRITE_RX_DIG_FIR2_IN_CLK_DIVIDE_RATIO(rx_dig_baseaddr, config->rx_slice_ch_conf_8_fir2_in_clk_divide_ratio);
	WRITE_RX_DIG_DP_FILTER_SELECT(rx_dig_baseaddr, config->rx_slice_ch_config_8_dp_filter_select);
}

static void setup_all_rx_clkgen(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		setup_rx_clkgen(SLICE_RX_0_TDR_DIG_MISC_BASE, SLICE_RX_0_BASE, config);
		setup_rx_clkgen(SLICE_RX_1_TDR_DIG_MISC_BASE, SLICE_RX_1_BASE, config);
		setup_rx_clkgen(SLICE_RX_2_TDR_DIG_MISC_BASE, SLICE_RX_2_BASE, config);
		setup_rx_clkgen(SLICE_RX_3_TDR_DIG_MISC_BASE, SLICE_RX_3_BASE, config);
	} else {
		setup_rx_clkgen(SEC_SLICE_RX_0_TDR_DIG_MISC_BASE, SEC_SLICE_RX_0_BASE, config);
		setup_rx_clkgen(SEC_SLICE_RX_1_TDR_DIG_MISC_BASE, SEC_SLICE_RX_1_BASE, config);
		setup_rx_clkgen(SEC_SLICE_RX_2_TDR_DIG_MISC_BASE, SEC_SLICE_RX_2_BASE, config);
		setup_rx_clkgen(SEC_SLICE_RX_3_TDR_DIG_MISC_BASE, SEC_SLICE_RX_3_BASE, config);
	}
}

static void setup_tx_clkgen(const uintptr_t tx_analog_dac_baseaddr, const uintptr_t tx_datapath_baseaddr, const uintptr_t tx_dig_baseaddr, mcs_clk_dividers_config_t *config)
{
	WRITE_TX_ANALOG_DAC_APACHE_CLKDIV_THERMCODE(tx_analog_dac_baseaddr, config->tx_slice_analog_dac_clkdiv_thermcode);
	WRITE_TX_ANALOG_DAC_APACHE_CLKDIV_DIV1_EN(tx_analog_dac_baseaddr, config->tx_slice_analog_dac_clkdiv_div1_en);

	WRITE_TX_DATAPATH_TX_DAC_IF_RD_PTR_INIT(tx_datapath_baseaddr, config->tx_slice_tx_datapath_tx_dac_if_rd_ptr_init);
	WRITE_TX_DATAPATH_TX_DAC_IF_WR_PTR_INIT(tx_datapath_baseaddr, config->tx_slice_tx_datapath_tx_dac_if_wr_ptr_init);
	WRITE_TX_DATAPATH_INT3_ENABLE(tx_datapath_baseaddr, config->tx_slice_tx_datapath_filter_control_0_int3_enable);

	WRITE_TX_DIG_TOUT_CLK_DIVIDE_RATIO(tx_dig_baseaddr, config->tx_slice_tx_dig_tout_clk_divide_ratio);
	WRITE_TX_DIG_TOUT_CLK_ENABLE(tx_dig_baseaddr, 1);
}

static void setup_all_tx_clkgen(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		setup_tx_clkgen(SLICE_TX0_ANALOG_DAC_BASE, SLICE_TX_0_DATAPATH_BASE, SLICE_TX_0_BASE, config);
		setup_tx_clkgen(SLICE_TX1_ANALOG_DAC_BASE, SLICE_TX_1_DATAPATH_BASE, SLICE_TX_1_BASE, config);
		setup_tx_clkgen(SLICE_TX2_ANALOG_DAC_BASE, SLICE_TX_2_DATAPATH_BASE, SLICE_TX_2_BASE, config);
		setup_tx_clkgen(SLICE_TX3_ANALOG_DAC_BASE, SLICE_TX_3_DATAPATH_BASE, SLICE_TX_3_BASE, config);
	} else {
		setup_tx_clkgen(SEC_SLICE_TX0_ANALOG_DAC_BASE, SEC_SLICE_TX_0_DATAPATH_BASE, SEC_SLICE_TX_0_BASE, config);
		setup_tx_clkgen(SEC_SLICE_TX1_ANALOG_DAC_BASE, SEC_SLICE_TX_1_DATAPATH_BASE, SEC_SLICE_TX_1_BASE, config);
		setup_tx_clkgen(SEC_SLICE_TX2_ANALOG_DAC_BASE, SEC_SLICE_TX_2_DATAPATH_BASE, SEC_SLICE_TX_2_BASE, config);
		setup_tx_clkgen(SEC_SLICE_TX3_ANALOG_DAC_BASE, SEC_SLICE_TX_3_DATAPATH_BASE, SEC_SLICE_TX_3_BASE, config);
	}
}

static void setup_orx_clkgen(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	uintptr_t orx_analog_adc_baseaddr;
	uintptr_t orx_dig_base;

	if (tile == ADRV906X_PRIMARY_TILE) {
		orx_analog_adc_baseaddr = SLICE_ORX_ACTRL_WEST_BASE;
		orx_dig_base = SLICE_ORX_BASE;
	} else {
		orx_analog_adc_baseaddr = SEC_SLICE_ORX_ACTRL_WEST_BASE;
		orx_dig_base = SEC_SLICE_ORX_BASE;
	}

	WRITE_ACTRL_ORX_WEST_REGMAP_TRM_CLKDIV_THERMCODE(orx_analog_adc_baseaddr, config->orx_slice_clkdiv_thermcode);
	WRITE_ACTRL_ORX_WEST_REGMAP_TRM_CLKDIV_DIV1_EN(orx_analog_adc_baseaddr, config->orx_slice_clkdiv_div1_en);

	WRITE_ORX_DIG_ORX_RD_PTR_INIT(orx_dig_base, config->orx_slice_orx_rd_ptr_init);
	WRITE_ORX_DIG_ADC_WR_PTR_INIT(orx_dig_base, config->orx_slice_orx_wr_ptr_init);
	WRITE_ORX_DIG_RIN_CLK_DIVIDE_RATIO(orx_dig_base, config->orx_slice_rin_clk_div_ratio);
	WRITE_ORX_DIG_RIN_CLK_ENABLE(orx_dig_base, 1);
	WRITE_ORX_DIG_STAGE1_MODE(orx_dig_base, config->orx_slice_orx_dig_stage1_mode);
}

static void setup_tx_loopback_clkgen(const uintptr_t baseaddr, mcs_clk_dividers_config_t *config)
{
	WRITE_ACTRL_TXLB_WEST_REGMAP_TRM_CLKDIV_THERMCODE(baseaddr, config->tx_slice_analog_loopback_trm_clkdiv_thermcode);
	WRITE_ACTRL_TXLB_WEST_REGMAP_TRM_CLKDIV_DIV1_EN(baseaddr, config->tx_slic_analog_loopback_div1_en);
}

static void setup_all_tx_loopback_clkgen(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		setup_tx_loopback_clkgen(SLICE_TX_0_ACTRL_WEST_BASE, config);
		setup_tx_loopback_clkgen(SLICE_TX_1_ACTRL_WEST_BASE, config);
		setup_tx_loopback_clkgen(SLICE_TX_2_ACTRL_WEST_BASE, config);
		setup_tx_loopback_clkgen(SLICE_TX_3_ACTRL_WEST_BASE, config);
	} else {
		setup_tx_loopback_clkgen(SEC_SLICE_TX_0_ACTRL_WEST_BASE, config);
		setup_tx_loopback_clkgen(SEC_SLICE_TX_1_ACTRL_WEST_BASE, config);
		setup_tx_loopback_clkgen(SEC_SLICE_TX_2_ACTRL_WEST_BASE, config);
		setup_tx_loopback_clkgen(SEC_SLICE_TX_3_ACTRL_WEST_BASE, config);
	}
}

static void setup_rfplls_reference_clock_dividers(enum adrv906x_tile_type tile)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(EAST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(WEST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_PD(EAST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_PD(WEST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(EAST_RFPLL_BASE, 1);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(WEST_RFPLL_BASE, 1);
	} else {
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(SEC_EAST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDE_RATIO(SEC_WEST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_PD(SEC_EAST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_PD(SEC_WEST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(SEC_EAST_RFPLL_BASE, 1);
		WRITE_PLL_MEM_MAP_REF_CLK_DIVIDER_RESETB(SEC_WEST_RFPLL_BASE, 1);
	}
}

static void setup_sysref_path_for_external_sysref_signal(enum adrv906x_tile_type tile)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		WRITE_CORE_SYSREF_BUFFER_EN(DIG_CORE_BASE, 1);
		WRITE_CORE_SYSREF_SAMPLE_EN(DIG_CORE_BASE, 1);
	} else {
		WRITE_CORE_SYSREF_BUFFER_EN(SEC_DIG_CORE_BASE, 1);
		WRITE_CORE_SYSREF_SAMPLE_EN(SEC_DIG_CORE_BASE, 1);
	}
}

static void setup_mcs_clock_divider(const uintptr_t baseaddr, bool reset)
{
	WRITE_PLL_MEM_MAP_MCS_DEVICE_CLK_DIVIDER_SYNC_ENABLE(baseaddr, 1);
	if (reset)
		WRITE_PLL_MEM_MAP_LO_SYNC_RESETB(baseaddr, 0);
	WRITE_PLL_MEM_MAP_LO_SYNC_RESETB(baseaddr, 1);
	if (reset)
		WRITE_PLL_MEM_MAP_MCS_RESETB(baseaddr, 0);
	WRITE_PLL_MEM_MAP_MCS_RESETB(baseaddr, 1);
}

static void enable_mcs_clkgen_sync(const uintptr_t baseaddr)
{
	WRITE_PLL_MEM_MAP_MCS_CLKGEN_SYNC_ENABLE(baseaddr, 1);
}

static void setup_all_mcs_clock_dividers(enum adrv906x_tile_type tile)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		setup_mcs_clock_divider(CLKPLL_BASE, false);
		setup_mcs_clock_divider(EAST_RFPLL_BASE, true);
		setup_mcs_clock_divider(WEST_RFPLL_BASE, true);
		enable_mcs_clkgen_sync(CLKPLL_BASE);
	} else {
		setup_mcs_clock_divider(SEC_CLKPLL_BASE, false);
		setup_mcs_clock_divider(SEC_EAST_RFPLL_BASE, true);
		setup_mcs_clock_divider(SEC_WEST_RFPLL_BASE, true);
		enable_mcs_clkgen_sync(SEC_CLKPLL_BASE);
	}
}

static void pre_switch_devclk_clock(enum adrv906x_tile_type tile)
{
	uintptr_t baseaddr;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = CLK_CTL;
	else
		baseaddr = SEC_CLK_CTL;

	clk_set_src_pre_switch(baseaddr, CLK_SRC_CLKPLL);
}

static void post_switch_devclk_clock(enum adrv906x_tile_type tile)
{
	uintptr_t baseaddr;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = CLK_CTL;
	else
		baseaddr = SEC_CLK_CTL;

	clk_set_src_post_switch(baseaddr, CLK_SRC_CLKPLL);
}

static void switch_devclk_clock(enum adrv906x_tile_type tile)
{
	uintptr_t baseaddr;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = CLK_CTL;
	else
		baseaddr = SEC_CLK_CTL;

	clk_set_src(baseaddr, CLK_SRC_DEVCLK);
}

static void use_device_clock_as_hsdig_until_mcs(enum adrv906x_tile_type tile)
{
	uintptr_t baseaddr;

	if (tile == ADRV906X_PRIMARY_TILE)
		baseaddr = CLK_CTL;
	else
		baseaddr = SEC_CLK_CTL;

	clk_set_src_switch(baseaddr, CLK_SRC_CLKPLL, true);
}

static void set_mcs_enable(enum adrv906x_tile_type tile)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		WRITE_PLL_MEM_MAP_MCS_ENABLE(CLKPLL_BASE, 1);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(EAST_RFPLL_BASE, 1);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(WEST_RFPLL_BASE, 1);
	} else {
		WRITE_PLL_MEM_MAP_MCS_ENABLE(SEC_CLKPLL_BASE, 1);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(SEC_EAST_RFPLL_BASE, 1);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(SEC_WEST_RFPLL_BASE, 1);
	}
}

static void clear_mcs_enable(enum adrv906x_tile_type tile)
{
	if (tile == ADRV906X_PRIMARY_TILE) {
		WRITE_PLL_MEM_MAP_MCS_ENABLE(CLKPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(EAST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(WEST_RFPLL_BASE, 0);
	} else {
		WRITE_PLL_MEM_MAP_MCS_ENABLE(SEC_CLKPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(SEC_EAST_RFPLL_BASE, 0);
		WRITE_PLL_MEM_MAP_MCS_ENABLE(SEC_WEST_RFPLL_BASE, 0);
	}
}

static int prepare_for_mcs(enum adrv906x_tile_type tile, mcs_clk_dividers_config_t *config, bool mcs_bypass)
{
	int err = 0;

	/* 1 */
	enable_scaled_devclk_to_all_plls(tile);

	/* 2 */
	setup_reference_clock_divider_for_clkpll(tile, config);

	/* 3 */
	setup_clkpll_core_digital_clock_root_divider(tile, config);

	/* 4 */
	err = clock_pll_initialization(tile, config);
	if (err)
		return err;

	/* Check force mcs bypass */
	bool bypass_mcs = (FORCE_MCS_BYPASS || mcs_bypass);
	if (bypass_mcs) {
		INFO("Bypassing Multi-Chip Sync preparation for tile %d\n", tile + 1);
		/* Set PLL CLK */
		if (tile == ADRV906X_PRIMARY_TILE)
			clk_set_src(CLK_CTL, CLK_SRC_CLKPLL);
		else
			clk_set_src(SEC_CLK_CTL, CLK_SRC_CLKPLL);
		return 0;
	}

	/* TODO remove "if" when SLICES are supported on Protium */
	if (!plat_is_protium() && !plat_is_palladium()) {
		/* 5 */
		setup_all_rx_clkgen(tile, config);

		/* 6 */
		setup_all_tx_clkgen(tile, config);

		/* 7 */
		setup_orx_clkgen(tile, config);

		/* 8 */
		setup_all_tx_loopback_clkgen(tile, config);
	}

	/* 9 */
	setup_rfplls_reference_clock_dividers(tile);

	/* 10 */
	setup_sysref_path_for_external_sysref_signal(tile);

	/* 11 */
	setup_all_mcs_clock_dividers(tile);

	/* 12 */
	pre_switch_devclk_clock(tile); /* Note: we are likely to loose the UART here */

	/* 13 */
	use_device_clock_as_hsdig_until_mcs(tile);

	return 0;
}

static int is_mcs_complete(enum adrv906x_tile_type tile)
{
	// 14.a.iii */
	/* Note: done by HW */

	/* 14.a.iv */
	int ret;

	if (tile == ADRV906X_PRIMARY_TILE) {
		ret = READ_PLL_MEM_MAP_MCS_SPI_STATUS(CLKPLL_BASE) == CLKPLL_MCS_COMPLETE &&
		      READ_PLL_MEM_MAP_MCS_SPI_STATUS(EAST_RFPLL_BASE) == RFPLL_MCS_COMPLETE &&
		      READ_PLL_MEM_MAP_MCS_SPI_STATUS(WEST_RFPLL_BASE) == RFPLL_MCS_COMPLETE;
	} else {
		ret = READ_PLL_MEM_MAP_MCS_SPI_STATUS(SEC_CLKPLL_BASE) == CLKPLL_MCS_COMPLETE &&
		      READ_PLL_MEM_MAP_MCS_SPI_STATUS(SEC_EAST_RFPLL_BASE) == RFPLL_MCS_COMPLETE &&
		      READ_PLL_MEM_MAP_MCS_SPI_STATUS(SEC_WEST_RFPLL_BASE) == RFPLL_MCS_COMPLETE;
	}

	return ret;
}

/*--------------------------------------------------------
 * EXPORTED FUNCTIONS
 *------------------------------------------------------*/
bool clk_do_mcs(bool dual_tile, uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting, bool mcs_bypass)
{
	int err = 0;

	if (!current_clock_source_is_devclk(dual_tile)) {
		ERROR("MCS: current clock source is not devclk\n");
		return false;
	}

	/* Get configuration */
	mcs_clk_dividers_config_t *config = getconfig(clkpll_freq_setting, orx_adc_freq_setting);

	if (!config) {
		ERROR("MCS: no available configuration for clkpll freq %d and orx_adc freq %d.\n", clkpll_freq_setting, orx_adc_freq_setting);
		return false;
	}

	/* Prepare for MCS */
	err = prepare_for_mcs(ADRV906X_PRIMARY_TILE, config, mcs_bypass);
	if (err)
		return false;
	if (dual_tile) {
		err = prepare_for_mcs(ADRV906X_SECONDARY_TILE, config, mcs_bypass);
		if (err)
			return false;
	}

	/* Check force mcs bypass */
	bool bypass_mcs = (FORCE_MCS_BYPASS || mcs_bypass);
	if (bypass_mcs) {
		INFO("Bypassing Multi-Chip Sync\n");
		return true;
	}

	/* 14.a.i */
	set_mcs_enable(ADRV906X_PRIMARY_TILE);
	if (dual_tile)
		set_mcs_enable(ADRV906X_SECONDARY_TILE);

	/* 14.a.ii */
	bool sysref_enable_completed = false;
	if (plat_sysref_enable())
		sysref_enable_completed = true;

	/* Wait for MCS completion */
	bool mcs1_completed = false;
	uint64_t timeout = timeout_init_us(MCS_COMPLETION_TIMEOUT_US);
	while (!timeout_elapsed(timeout)) {
		if (is_mcs_complete(ADRV906X_PRIMARY_TILE)) {
			mcs1_completed = true;
			break;
		}
	}
	bool mcs2_completed = false;
	if (dual_tile) {
		timeout = timeout_init_us(MCS_COMPLETION_TIMEOUT_US);
		while (!timeout_elapsed(timeout)) {
			if (is_mcs_complete(ADRV906X_SECONDARY_TILE)) {
				mcs2_completed = true;
				break;
			}
		}
	}

	/* 14.a.v */
	clear_mcs_enable(ADRV906X_PRIMARY_TILE);
	if (dual_tile)
		clear_mcs_enable(ADRV906X_SECONDARY_TILE);

	/* 14.a.vi */
	bool sysref_disable_completed = false;
	if (plat_sysref_disable(mcs1_completed))
		sysref_disable_completed = true;

	/* 15. Check for any errors in the MCS sequence and report them */
	if ((sysref_enable_completed && sysref_disable_completed) && ((mcs1_completed && !dual_tile) ||
								      (mcs1_completed && dual_tile && mcs2_completed))) {
		/* If we pass MCS, finish the switch and report back */
		post_switch_devclk_clock(ADRV906X_PRIMARY_TILE);
		if (dual_tile)
			post_switch_devclk_clock(ADRV906X_SECONDARY_TILE);
		INFO("Multi-Chip Sync completed \n");
		return true;
	} else {
		/* If MCS fails, the current clock setup is in an unknown state,
		 *  so switch back to a known good clock(DEVCLK) to print before erroring out */
		switch_devclk_clock(ADRV906X_PRIMARY_TILE);

		if (!sysref_enable_completed)
			ERROR("MCS: Sysref enable failed\n");

		if (!sysref_disable_completed)
			ERROR("MCS: Sysref disable failed\n");

		if (!mcs1_completed) {
			if (dual_tile)
				ERROR("Multi-Chip Sync failed on Primary\n");
			else
				ERROR("Multi-Chip Sync failed\n");
		}

		if (dual_tile && !mcs2_completed)
			ERROR("Multi-Chip Sync failed on Secondary\n");

		return false;
	}
}

bool clk_verify_config(uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting)
{
	int number_of_configurations = sizeof(clk_divs_cfg) / sizeof(mcs_clk_dividers_config_t);

	for (int i = 0; i < number_of_configurations; i++)
		if ((clk_divs_cfg[i].clkpll_freq_setting == clkpll_freq_setting) && (clk_divs_cfg[i].orx_adc_freq_setting == orx_adc_freq_setting))
			return true;
	return false;
}

int clk_initialize_pll_programming(bool secondary, bool eth_pll, uint8_t clkpll_freq_setting, uint8_t orx_adc_freq_setting)
{
	int err;

	if (eth_pll)
		err = clk_initialize_eth_pll_programming(secondary);
	else
		err = clk_initialize_clk_pll_programming(secondary, clkpll_freq_setting, orx_adc_freq_setting);

	return err;
}
