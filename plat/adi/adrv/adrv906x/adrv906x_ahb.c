/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>

#include <adrv906x_ahb.h>
#include <adrv906x_ahb_def.h>
#include <adrv906x_def.h>

#define BIT_MANIPULATE_32(ADDR, MASK, OFFSET, VALUE)       do{                            \
		mmio_write_32(ADDR, ((mmio_read_32(ADDR) & ~MASK) | (VALUE << OFFSET)));          \
}while (0)

#define BIT_MANIPULATE_8(ADDR, MASK, OFFSET, VALUE)       do{                             \
		mmio_write_8(ADDR, ((mmio_read_8(ADDR) & ~MASK) | (VALUE << OFFSET)));            \
}while (0)

#define BIT_READ_32(ADDR, MASK, OFFSET)      (mmio_read_32(ADDR) & ~MASK) >> OFFSET
#define BIT_READ_8(ADDR, MASK, OFFSET)       (uint8_t)((mmio_read_8(ADDR) & ~MASK) >> OFFSET)

/* adrv906x channel properties*/
#define ADRV906X_RX_CH_1                                                                    0UL
#define ADRV906X_RX_CHAN_LEN                                                                4UL
#define ADRV906X_TX_CH_1                                                                    0UL
#define ADRV906X_TX_CHAN_LEN                                                                4UL
#define ADRV906X_ORX_CH_1                                                                   0UL
#define ADRV906X_ORX_CHAN_LEN                                                               1UL

typedef struct {
	uint8_t reg_rd_delay;                   /*!< Read enable delay */
	uint8_t analog_reg_wr_setup;            /*!< Analog write setup */
	uint8_t analog_reg_wr_hold;             /*!< Analog write hold */
	uint8_t analog_reg_rd_cycle;            /*!< Analog read cycle */
} ahb_spi_bridge_delays_t;

typedef struct {
	ahb_spi_bridge_delays_t clk_plls;       /*!<  plls */
	ahb_spi_bridge_delays_t core;           /*!<  Core( RF plls)*/
	ahb_spi_bridge_delays_t eth_plls;       /*!<  Eth plls */
	ahb_spi_bridge_delays_t rx;             /*!<  Rx slices */
	ahb_spi_bridge_delays_t tx;             /*!<  Tx Slices */
	ahb_spi_bridge_delays_t orx;            /*!<  ORx slices */
} abh_spi_bridge_config_t;

enum adrv906x_tile_type {
	ADRV906X_PRIMARY_TILE = 0,
	ADRV906X_SECONDARY_TILE,
	ADRV906X_TILE_TYPE_NUM,
};

static abh_spi_bridge_config_t ahb_spi_bridge_config =
{
	/* TODO: Values need to be recommended by systems team:
	 * AHB_reg_rd_delay
	 * AHB_analog_reg_wr_setup
	 * AHB_analog_reg_wr_hold
	 * AHB_analog_reg_rd_cycles
	 */
	{ 0x0FU, 0x3FU, 0x3FU, 0x3FU },         /* CLK PLLs */
	{ 0x0FU, 0x3FU, 0x3FU, 0x3FU },         /* Core/RF PLLs */
	{ 0x0FU, 0x3FU, 0x3FU, 0x3FU },         /* Eth PLLs */
	{ 0x0FU, 0x3FU, 0x3FU, 0x3FU },         /* Rx slices */
	{ 0x0FU, 0x3FU, 0x3FU, 0x3FU },         /* Tx slices */
	{ 0x0FU, 0x3FU, 0x3FU, 0x3FU } /* ORx slices */
};

const uint32_t core_base_addr[ADRV906X_TILE_TYPE_NUM] =
{
	DIG_CORE_BASE,
	SEC_DIG_CORE_BASE
};

const uint32_t clk_base_addr[ADRV906X_TILE_TYPE_NUM] =
{
	CLK_CTL,
	SEC_CLK_CTL
};

const uint32_t eth_base_addr[ADRV906X_TILE_TYPE_NUM] =
{
	EMAC_COMMON_BASE,
	SEC_EMAC_COMMON_BASE
};

const uint32_t rx_spi_base_addr[ADRV906X_TILE_TYPE_NUM][ADRV906X_RX_CHAN_LEN] =
{
	{
		SLICE_RX_0_BASE,
		SLICE_RX_1_BASE,
		SLICE_RX_2_BASE,
		SLICE_RX_3_BASE,
	},
	{
		SEC_SLICE_RX_0_BASE,
		SEC_SLICE_RX_1_BASE,
		SEC_SLICE_RX_2_BASE,
		SEC_SLICE_RX_3_BASE,
	}
};

const uint32_t tx_spi_base_addr[ADRV906X_TILE_TYPE_NUM][ADRV906X_TX_CHAN_LEN] =
{
	{
		SLICE_TX_0_BASE,
		SLICE_TX_1_BASE,
		SLICE_TX_2_BASE,
		SLICE_TX_3_BASE,
	},
	{
		SEC_SLICE_TX_0_BASE,
		SEC_SLICE_TX_1_BASE,
		SEC_SLICE_TX_2_BASE,
		SEC_SLICE_TX_3_BASE,
	}
};

const uint32_t orx_spi_base_addr[ADRV906X_TILE_TYPE_NUM][ADRV906X_ORX_CHAN_LEN] =
{
	{
		SLICE_ORX_BASE,
	},
	{
		SEC_SLICE_ORX_BASE
	}
};

/* Set write delay and read count settings for core slice, clk pll & eth plls */
static void adrv906x_ahb_core_config(uint8_t tile_idx)
{
	const abh_spi_bridge_config_t *p_ahb_spi_bridge_config = &ahb_spi_bridge_config;

	if (tile_idx < ADRV906X_TILE_TYPE_NUM) {
		/* Setup clk plls */
		BIT_MANIPULATE_32(clk_base_addr[tile_idx] + CLK_AHB_CONFIG_REG,
				  CLK_AHB_REG_WR_SETUP_MASK,
				  CLK_AHB_REG_WR_SETUP_POS,
				  p_ahb_spi_bridge_config->clk_plls.analog_reg_wr_setup);
		BIT_MANIPULATE_32(clk_base_addr[tile_idx] + CLK_AHB_CONFIG_REG,
				  CLK_AHB_REG_WR_HOLD_MASK,
				  CLK_AHB_REG_WR_HOLD_POS,
				  p_ahb_spi_bridge_config->clk_plls.analog_reg_wr_hold);
		BIT_MANIPULATE_32(clk_base_addr[tile_idx] + CLK_AHB_CONFIG_REG,
				  CLK_AHB_REG_RD_CYCLES_MASK,
				  CLK_AHB_REG_RD_CYCLES_POS,
				  p_ahb_spi_bridge_config->clk_plls.analog_reg_rd_cycle);

		/* Setup the core */
		BIT_MANIPULATE_8(core_base_addr[tile_idx] + CORE_AHB_WR_SETUP_CYCLES_REG,
				 CORE_AHB_WR_SETUP_CYCLES_MASK,
				 CORE_AHB_WR_SETUP_CYCLES_POS,
				 p_ahb_spi_bridge_config->core.analog_reg_wr_setup);
		BIT_MANIPULATE_8(core_base_addr[tile_idx] + CORE_AHB_WR_HOLD_CYCLES_REG,
				 CORE_AHB_WR_HOLD_CYCLES_MASK,
				 CORE_AHB_WR_HOLD_CYCLES_POS,
				 p_ahb_spi_bridge_config->core.analog_reg_wr_hold);
		BIT_MANIPULATE_8(core_base_addr[tile_idx] + CORE_AHB_RD_CYCLES_REG,
				 CORE_AHB_RD_CYCLES_MASK,
				 CORE_AHB_RD_CYCLES_POS,
				 p_ahb_spi_bridge_config->core.analog_reg_rd_cycle);

		/* Setup the Eth PLLs */
		BIT_MANIPULATE_32(eth_base_addr[tile_idx] + ETH_AHB_BRIDGE_ANA_REGS,
				  ETH_AHB_REG_WR_SETUP_MASK,
				  ETH_AHB_REG_WR_SETUP_POS,
				  p_ahb_spi_bridge_config->eth_plls.analog_reg_wr_setup);
		BIT_MANIPULATE_32(eth_base_addr[tile_idx] + ETH_AHB_BRIDGE_ANA_REGS,
				  ETH_AHB_REG_WR_HOLD_MASK,
				  ETH_AHB_REG_WR_HOLD_POS,
				  p_ahb_spi_bridge_config->eth_plls.analog_reg_wr_hold);
		BIT_MANIPULATE_32(eth_base_addr[tile_idx] + ETH_AHB_BRIDGE_ANA_REGS,
				  ETH_AHB_REG_RD_CYCLES_MASK,
				  ETH_AHB_REG_RD_CYCLES_POS,
				  p_ahb_spi_bridge_config->eth_plls.analog_reg_rd_cycle);
		BIT_MANIPULATE_32(eth_base_addr[tile_idx] + ETH_AHB_BRIDGE_REGS,
				  ETH_AHB_REG_RD_DELAY_MASK,
				  ETH_AHB_REG_RD_DELAY_POS,
				  p_ahb_spi_bridge_config->eth_plls.reg_rd_delay);
	}
}

/* Set write delay and read count settings for Tx, Rx, and Orx */
static void adrv906x_ahb_submaps_config(uint8_t tile_idx)
{
	(void)tile_idx;

	const abh_spi_bridge_config_t *p_ahb_spi_bridge_config = &ahb_spi_bridge_config;
	uint32_t ch;
	uint32_t base;
	if (tile_idx < ADRV906X_TILE_TYPE_NUM) {
		/* Setup the Rx sub-map */
		for (ch = ADRV906X_RX_CH_1; ch < ADRV906X_RX_CHAN_LEN; ch++) {
			base = rx_spi_base_addr[tile_idx][ch];
			BIT_MANIPULATE_32(base + RX_AHB_CONFIG_REG,
					  RX_AHB_RD_DELAY_MASK,
					  RX_AHB_RD_DELAY_POS,
					  (uint32_t)p_ahb_spi_bridge_config->rx.reg_rd_delay);
			BIT_MANIPULATE_32(base + RX_AHB_CONFIG_REG,
					  RX_AHB_WR_SETUP_MASK,
					  RX_AHB_WR_SETUP_POS,
					  (uint32_t)p_ahb_spi_bridge_config->rx.analog_reg_wr_setup);
			BIT_MANIPULATE_32(base + RX_AHB_CONFIG_REG,
					  RX_AHB_WR_HOLD_MASK,
					  RX_AHB_WR_HOLD_POS,
					  (uint32_t)p_ahb_spi_bridge_config->rx.analog_reg_wr_hold);
			BIT_MANIPULATE_32(base + RX_AHB_CONFIG_REG,
					  RX_AHB_RD_CYCLES_MASK,
					  RX_AHB_RD_CYCLES_POS,
					  (uint32_t)p_ahb_spi_bridge_config->rx.analog_reg_rd_cycle);
		}

		/* Setup the Tx sub-map */
		for (ch = ADRV906X_TX_CH_1; ch < ADRV906X_TX_CHAN_LEN; ch++) {
			base = tx_spi_base_addr[tile_idx][ch];
			BIT_MANIPULATE_32(base + TX_AHB_CONFIG_REG,
					  TX_AHB_RD_DELAY_MASK,
					  TX_AHB_RD_DELAY_POS,
					  (uint32_t)p_ahb_spi_bridge_config->tx.reg_rd_delay);
			BIT_MANIPULATE_32(base + TX_AHB_CONFIG_REG,
					  TX_AHB_WR_SETUP_MASK,
					  TX_AHB_WR_SETUP_POS,
					  (uint32_t)p_ahb_spi_bridge_config->tx.analog_reg_wr_setup);
			BIT_MANIPULATE_32(base + TX_AHB_CONFIG_REG,
					  TX_AHB_WR_HOLD_MASK,
					  TX_AHB_WR_HOLD_POS,
					  (uint32_t)p_ahb_spi_bridge_config->tx.analog_reg_wr_hold);
			BIT_MANIPULATE_32(base + TX_AHB_CONFIG_REG,
					  TX_AHB_RD_CYCLES_MASK,
					  TX_AHB_RD_CYCLES_POS,
					  (uint32_t)p_ahb_spi_bridge_config->tx.analog_reg_rd_cycle);
		}

		/* Setup the Orx sub-map */
		for (ch = ADRV906X_ORX_CH_1; ch < ADRV906X_ORX_CHAN_LEN; ch++) {
			base = orx_spi_base_addr[tile_idx][ch];
			BIT_MANIPULATE_32(base + ORX_AHB_CONFIG_REG,
					  ORX_AHB_RD_DELAY_MASK,
					  ORX_AHB_RD_DELAY_POS,
					  (uint32_t)p_ahb_spi_bridge_config->orx.reg_rd_delay);
			BIT_MANIPULATE_32(base + ORX_AHB_CONFIG_REG,
					  ORX_AHB_WR_SETUP_MASK,
					  ORX_AHB_WR_SETUP_POS,
					  (uint32_t)p_ahb_spi_bridge_config->orx.analog_reg_wr_setup);
			BIT_MANIPULATE_32(base + ORX_AHB_CONFIG_REG,
					  ORX_AHB_WR_HOLD_MASK,
					  ORX_AHB_WR_HOLD_POS,
					  (uint32_t)p_ahb_spi_bridge_config->orx.analog_reg_wr_hold);
			BIT_MANIPULATE_32(base + ORX_AHB_CONFIG_REG,
					  ORX_AHB_RD_CYCLES_MASK,
					  ORX_AHB_RD_CYCLES_POS,
					  (uint32_t)p_ahb_spi_bridge_config->orx.analog_reg_rd_cycle);
		}
	}
}

void adrv906x_ahb_init(bool is_primary)
{
	uint8_t tile_idx;

	if (is_primary)
		tile_idx = ADRV906X_PRIMARY_TILE;
	else
		tile_idx = ADRV906X_SECONDARY_TILE;

	adrv906x_ahb_core_config(tile_idx);
	adrv906x_ahb_submaps_config(tile_idx);
}
