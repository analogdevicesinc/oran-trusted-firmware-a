/*
 *************************************************************************
 *
 * Test file adapted from DV test file (emac1g_duplex_rdwr_test_a55.c).
 * Minimal changes required for BL1 test
 * - flush/invalidate dcache (DMA operations)
 * - Use aliased L4 mem region (0x00000000) instead of non-aliased one (0x38000000)
 * - Include different set of header files
 * - Rename some register fields name
 *
 * Functionality: send one packet and receive one packet
 *
 * How to reproduce:
 * 1. Copy the next Yoda header files from Yoda package to this folder (drivers/adi/test/)
 *   - emac_1g_addr_def.h
 *   - emac_1g_addr_rdef.h
 *   - gpio_pinmux_pad_addr_def.h
 *   - SPU_addr_def.h
 *   - SPU_CFG4_addr_def.h
 *   - a55_sys_cfg_addr_def.h
 *   - core_addr_cdef.h
 * 2. Add the following in drivers/adi/test/test_framework.c
 *    -> extern int GMAC_1G_test();
 *    -> Call GMAC_1G_test() inside test_main()
 * 3. Include this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 * 4. Open these tools (or alternate ones) in PC connected to Adrv906x
 *    - Wireshark: to see the packet sent from Adrv906x to PC
 *    - Ostinato:  to send a pakcet from PC to Adrv906x
 * 5. Run test, wait for packet sent by Adrv906x (Wireshark) and then send packet
 *    from PC (Ostinato). That's all...test shoud finish
 *
 *************************************************************************
 */

//#define __DV_TEST__  // Commented is for BL1 test

#ifdef __DV_TEST__
//#include "pre_test.h"
#include "a55-common.h"
#include "a55-testing.h"
#include "a55-isr.h"
#include "inc2/emac1g_macros_c_zealand.h"

#else

// Required Yoda header files
#include "emac_1g_addr_def.h"
#include "emac_1g_addr_rdef.h"
#include "gpio_pinmux_pad_addr_def.h"
#include "SPU_addr_def.h"
#include "SPU_CFG4_addr_def.h"
#include "a55_sys_cfg_addr_def.h"
#include "core_addr_cdef.h"
#include <stdio.h>
#include <arch_helpers.h>
#include <adrv906x_def.h>
#include <adrv906x_mmap.h>
#include <lib/mmio.h>

#include <drivers/delay_timer.h>
// Re-naming definitions to current Yoda 0.8.0.1 (missing 'EMAC_1G_')
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_AAL                        BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_AAL
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_FB                         BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_FB
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN4                      BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN4
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN8                      BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN8
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN16                     BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN16
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_MODE_SWR                               BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_MODE_SWR
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_LIST_ADDRESS_RDESLA     BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_LIST_ADDRESS_RDESLA
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_TAIL_POINTER_RDTP       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_TAIL_POINTER_RDTP
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_CONTROL_DSL                    BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_CONTROL_DSL
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_TXPBL               BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_TXPBL
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_RXPBL               BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_RXPBL
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_NIE           BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_NIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_AIE           BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_AIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_TSF             BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_TSF
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH_ADDRHI                   BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH_ADDRHI
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_LOW_ADDRLO                    BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_LOW_ADDRLO
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER_RA                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER_RA
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER_PM                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER_PM 
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RWTE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RWTE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RSE           BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RSE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RBUE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RBUE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RIE           BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TBUE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TBUE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TXSE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TXSE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TIE           BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_RESERVED_TXQEN  BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_RESERVED_TXQEN
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_RESERVED_TQS    BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_RESERVED_TQS
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH_AE                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH_AE
#define BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA                BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_PT                     BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_PT
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA                BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_ERIE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_ERIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_FBEE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_FBEE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_CDEE          BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_CDEE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_TAIL_POINTER_TDTP       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_TAIL_POINTER_TDTP
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_LIST_ADDRESS_TDESLA     BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_LIST_ADDRESS_TDESLA
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL2_RDRL               BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL2_RDRL
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_RING_LENGTH_TDRL        BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_RING_LENGTH_TDRL
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_PHYIE                 BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_PHYIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RGSMIIIE              BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RGSMIIIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC                     BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST                      BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_JE                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_JE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES                      BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_TE                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_TE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST                  BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_RE                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_RE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_PMTIE                 BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RESERVED_PMTIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_DM                       BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_DM
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_LPIIE                 BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RESERVED_LPIIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_TXSTSIE               BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_TXSTSIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RXSTSIE               BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RXSTSIE
#define BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_MDIOIE                BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_MDIOIE
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR                  BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RPS                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RPS
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_AIS                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_AIS
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TPS                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TPS
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST                  BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_ETI                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_ETI
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_NIS                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_NIS
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TI                      BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TI
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RI                      BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RI
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU
#define BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR                  BITP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR
#define BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RBU                     BITM_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RBU

#define PERIPHERAL_NS_EMAC_1G                                                     (0x201c0050)
#define EMAC_1G_YODA_OSC_CLK_DIV_MASK                                             (0x000FE000)
#define EMAC_1G_YODA_PHY_INTF_SEL_I_MASK                                          (0x00000380)
#define EMAC_1G_YODA_OSC_CLK_DIV_125MHZ                                           (0 << 13)
#define EMAC_1G_YODA_OSC_CLK_DIV_250MHZ                                           (1 << 13)
#define EMAC_1G_YODA_PHY_INTF_SEL_I_RGMII                                         (1 << 3)
#define EMAC_1G_YODA_PHY_INTF_SEL_I_RMII                                          (4 << 3)
#define EMAC_1G_CG_ENABLE                                                         (0x01)

static void write32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask, uint32_t val);

#define READ_BF_8BIT_REG   read8bf
#define WRITE_BF_8BIT_REG  write8bf
#define WRITE_BF_32BIT_REG write32bf

#define IDX_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS    (0X90000U)
#define pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS(base)    ((volatile       uint32_t *)      ((base)+IDX_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS))
#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_TERM_ENABLE (13U)
#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_TERM_ENABLE (0X00002000U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_TERM_ENABLE(base,val)    (WRITE_BF_32BIT_REG(pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_TERM_ENABLE, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_TERM_ENABLE, ((val))))

#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_ENABLE (12U)
#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_ENABLE (0X00001000U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_ENABLE(base,val)    (WRITE_BF_32BIT_REG( pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_ENABLE, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_ENABLE, ((val))))

#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RB (11U)
#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RB (0X00000800U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RB(base,val)    (WRITE_BF_32BIT_REG( pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RB, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RB, ((val))))

#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_KILLCLK (7U)
#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_KILLCLK (0X00000080U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_KILLCLK(base,val)    (WRITE_BF_32BIT_REG( pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_KILLCLK, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_KILLCLK, ((val))))

#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RATIO (9U)
#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RATIO (0X00000600U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RATIO(base,val)    (WRITE_BF_32BIT_REG( pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RATIO, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RATIO, ((val))))

#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_FUND (0X00000040U)
#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_FUND (6U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_FUND(base,val)    (WRITE_BF_32BIT_REG( pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_FUND, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_FUND, ((val))))

#define IDX_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS                      (0x00090004U)
#define pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS(base)               ((volatile uint32_t *)((base) + IDX_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS))
#define BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS_ETH1G_REFPATH_PD    (0X00020000U)
#define BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS_ETH1G_REFPATH_PD    (17U)
#define WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS_ETH1G_REFPATH_PD(base,val)    WRITE_BF_32BIT_REG(pREG_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS((base)), BITP_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS_ETH1G_REFPATH_PD, BITM_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS_ETH1G_REFPATH_PD,((val)))

#endif

#define CORES_ACTIVE 1
#define write_func(ADDR,DATA) *((volatile unsigned*)(ADDR)) = DATA;
#define read_func(ADDR)       (*((volatile unsigned*)(ADDR)))
static inline void ARM_REG_WRITE ( uint32_t addr, int32_t data );
static inline int32_t ARM_REG_READ ( uint32_t addr );
//*(uint8_t volatile *)(ADDR_DIGITAL_CORE_EAST_RFPLL_VCO_SHUNT_LDO_VCO_SHUNT_LDO_CTL_1) = (uint8_t)0x18;
// uint8_t shunt_east_ldo_stat = *(uint8_t volatile *)(ADDR_DIGITAL_CORE_EAST_RFPLL_VCO_SHUNT_LDO_VCO_SHUNT_LDO_STAT );
#ifdef __DV_TEST__
static inline void ARM_REG_WRITE( uint32_t addr, int32_t data )
{
    int32_t *write_ptr = ( int32_t * )( addr );
    *write_ptr = data;
}


static inline int32_t ARM_REG_READ( uint32_t addr )
{
    int32_t volatile *read_ptr = ( int32_t * )( addr );
    return *read_ptr;
}
#else
static void write32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask, uint32_t val)
{
	uint32_t reg = mmio_read_32((uintptr_t)addr) & ~mask;

	reg |= (val << position) & mask;
	mmio_write_32((uintptr_t)addr, reg);
}

static inline void ARM_REG_WRITE( uint32_t addr, int32_t data )
{
    int32_t *write_ptr = (int32_t *)(intptr_t)( addr );
    *write_ptr = data;
}

static inline int32_t ARM_REG_READ( uint32_t addr )
{
    int32_t volatile *read_ptr = (int32_t *)(intptr_t)( addr );
    return *read_ptr;
}

#define   read_reg32   ARM_REG_READ
#endif

#ifdef __DV_TEST__
int main()
#else
int GMAC_1G_test()
#endif
{
//	int timer[8];
	   int read_data,read_data_str;
//	   int is_it_not_two_frame;


	//Changes for C Test
#ifdef __DV_TEST__
	    volatile int32_t tx_desc_start = 0x3801F200;
	    volatile int32_t rx_desc_start = 0x38027200;
	    volatile int32_t tx_mem_loc1_buf1 = 0x3802F200;
	    volatile int32_t tx_mem_loc1_buf2 = 0x38037200;
	    volatile int32_t tx_mem_loc2_buf1 = 0x3803F200;
	    volatile int32_t tx_mem_loc2_buf2 = 0x38047200;
	    volatile int32_t rx_mem_loc1_buf1 = 0x3804F200;
	    volatile int32_t rx_mem_loc1_buf11 = 0x3804F220;
	    volatile int32_t rx_mem_loc1_buf2 = 0x38057200;
	    volatile int32_t rx_mem_loc2_buf1 = 0x3805F200;
	    volatile int32_t rx_mem_loc2_buf2 = 0x38067200;
#else
#define GMAC_TEST_ADDR       0x00100000   // Aliased L4 addr space
	volatile int32_t tx_desc_start     = GMAC_TEST_ADDR + 0x0001F200;
	volatile int32_t rx_desc_start     = GMAC_TEST_ADDR + 0x00027200;
	volatile int32_t tx_mem_loc1_buf1  = GMAC_TEST_ADDR + 0x0002F200;
	volatile int32_t tx_mem_loc1_buf2  = GMAC_TEST_ADDR + 0x00037200;
	volatile int32_t tx_mem_loc2_buf1  = GMAC_TEST_ADDR + 0x0003F200;
	volatile int32_t tx_mem_loc2_buf2  = GMAC_TEST_ADDR + 0x00047200;
	volatile int32_t rx_mem_loc1_buf1  = GMAC_TEST_ADDR + 0x0004F200;
	volatile int32_t rx_mem_loc1_buf11 = GMAC_TEST_ADDR + 0x0004F220;
	volatile int32_t rx_mem_loc1_buf2  = GMAC_TEST_ADDR + 0x00057200;
	volatile int32_t rx_mem_loc2_buf1  = GMAC_TEST_ADDR + 0x0005F200;
	volatile int32_t rx_mem_loc2_buf2  = GMAC_TEST_ADDR + 0x00067200;
#endif
	    volatile int32_t buf1_size = 0x302;

#if 0
		*(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL85 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL86 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL87 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL88 = 0x3;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL89 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL90 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL91 = 0x0;
		*(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL92 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL93 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL94 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL95 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL96 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL97 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL98 = 0x0;
		*(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL99 = 0x0;
	        *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_GPIO_PINMUX_PAD_PINMUX_CONTROL_REGISTERS_GPIO_SOURCE_CONTROL_SEL100 = 0x0;
#endif

	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP10 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP11 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP12 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP13 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP14 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP15 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP34 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP35 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP36 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP45 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP46 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP59 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP60 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP61 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP62 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP63 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP64 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP65 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP66 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP67 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP68 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP70 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP72 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP74 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP76 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP78 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP80 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP86 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP147 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP148 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP149 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR0_REGMAP1_SECUREP151 = 0x2;
	    //*(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR1_REGMAP1_SECUREP9 = 0x2;
	    *(uint32_t volatile *)ADDR_PROC_DFE_PERIP_SPU_MMR4_REGMAP1_SECUREP29 = 0x2;
	    


		plat_setup_ns_sram_mmap();
	    //********************** TX DESCRIPTOR ************************************

	    for (int i = 0; i < 192; i+=4) {
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc1_buf1 + i*4) = (uint32_t)0xd9c8cea0;
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc1_buf1 + (i+1)*4) = (uint32_t)0xabcd8b9e;
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc1_buf1 + (i+2)*4) = (uint32_t)0xcafeface;
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc1_buf1 + (i+3)*4) = (uint32_t)0xa1b1c1d1;
			//clean_invalidate_cache((uint32_t volatile *)(tx_mem_loc1_buf1 + (i*4)));
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc2_buf1 + i*4 ) = (uint32_t)0xbeadcafe;
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc2_buf1 + (i+1)*4) = (uint32_t)0xa2b2c2d2;
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc2_buf1 + (i+2)*4) = (uint32_t)0x5432cafe;
		       *(uint32_t volatile *)(intptr_t)(tx_mem_loc2_buf1 + (i+3)*4) = (uint32_t)0xa3b3c3d3;
			//clean_invalidate_cache((uint32_t volatile *)(tx_mem_loc2_buf1 + (i*4)));
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc1_buf1 + i*4) = (uint32_t)0x0;
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc1_buf1 + (i+1)*4) = (uint32_t)0x0;
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc1_buf1 + (i+2)*4) = (uint32_t)0x0;
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc1_buf1 + (i+3)*4) = (uint32_t)0x0;
			//clean_invalidate_cache((uint32_t volatile *)(rx_mem_loc1_buf1 + (i*4)));
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc2_buf1 + i*4 ) = (uint32_t)0x0;
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc2_buf1 + (i+1)*4) = (uint32_t)0x0;
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc2_buf1 + (i+2)*4) = (uint32_t)0x0;
		       *(uint32_t volatile *)(intptr_t)(rx_mem_loc2_buf1 + (i+3)*4) = (uint32_t)0x0;
			//clean_invalidate_cache((uint32_t volatile *)(rx_mem_loc2_buf1 + (i*4)));
		}

#ifndef __DV_TEST__
	flush_dcache_range((uintptr_t)tx_mem_loc1_buf1, buf1_size);
	flush_dcache_range((uintptr_t)tx_mem_loc2_buf1, buf1_size);
	flush_dcache_range((uintptr_t)rx_mem_loc1_buf1, buf1_size);
	flush_dcache_range((uintptr_t)rx_mem_loc2_buf1, buf1_size);
	inv_dcache_range((uintptr_t)rx_mem_loc1_buf1, buf1_size);
	inv_dcache_range((uintptr_t)rx_mem_loc2_buf1, buf1_size);

	// Configure 1G Eth input clock divider
	uint32_t clk_base = PERIPHERAL_NS_EMAC_1G;
	uint32_t clk_div = *(uint32_t volatile *)(intptr_t)(clk_base);
	// ... disable clock
	clk_div |= EMAC_1G_CG_ENABLE;
	*(uint32_t volatile *)(intptr_t)(clk_base) = clk_div;
	// ... set RGMII interface and clock divider for 250Mhz
	clk_div &= ~(EMAC_1G_YODA_PHY_INTF_SEL_I_MASK | EMAC_1G_YODA_OSC_CLK_DIV_MASK);
	clk_div |= (EMAC_1G_YODA_PHY_INTF_SEL_I_RGMII | EMAC_1G_YODA_OSC_CLK_DIV_250MHZ);
	*(uint32_t volatile *)(intptr_t)(clk_base) = clk_div;
	// ... re-enable clock
	clk_div &= ~EMAC_1G_CG_ENABLE;
	*(uint32_t volatile *)(intptr_t)(clk_base) = clk_div;

	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_REFCLK_CONTROLS_ETH1G_REFPATH_PD(A55_SYS_CFG, 0x0);
	WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_TERM_ENABLE(A55_SYS_CFG, 0x1);
        WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_KILLCLK(A55_SYS_CFG, 0x0);
        WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RB(A55_SYS_CFG, 0x1);
        WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_RATIO(A55_SYS_CFG, 0x0);
        WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVCLK_DIV_FUND(A55_SYS_CFG, 0x1);
        WRITE_A55_SYS_CFG_CLOCK_CONTROLS_ETH1G_DEVCLK_CONTROLS_ETH1G_DEVICE_CLK_BUFFER_ENABLE(A55_SYS_CFG, 0x1);

	
#endif

	   *(uint32_t volatile *)(intptr_t)(tx_desc_start) = (uint32_t)(tx_mem_loc1_buf1);
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+4) = (uint32_t)(tx_mem_loc1_buf2);
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+8) = (uint32_t)(buf1_size | 0x80000000);
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+12) = (uint32_t)(0xA1000000 | (2*buf1_size));
	//clean_invalidate_cache((uint32_t volatile *)(tx_desc_start));

	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+32) = (uint32_t)(tx_mem_loc2_buf1);
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+36) = (uint32_t)(tx_mem_loc2_buf2);
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+40) = (uint32_t)(buf1_size | 0x80000000);
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+44) = (uint32_t)(0x91000000 | (2*buf1_size));
	//clean_invalidate_cache((uint32_t volatile *)(tx_desc_start+32));

	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+64) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+68) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+72) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+76) = (uint32_t)0x0;
	//clean_invalidate_cache((uint32_t volatile *)(tx_desc_start+64));

	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+96) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+100) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+104) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(tx_desc_start+108) = (uint32_t)0x0;
	//clean_invalidate_cache((uint32_t volatile *)(tx_desc_start+96));
#ifndef __DV_TEST__
	flush_dcache_range((uintptr_t)tx_desc_start, 112);
#endif

	/************************* RX DESCRIPTOR ***************************************/
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start) = (uint32_t)(rx_mem_loc1_buf1);
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+4) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+8) = (uint32_t)(rx_mem_loc1_buf2);
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+12) = (uint32_t)0xC1000000;
	//clean_invalidate_cache((uint32_t volatile *)(rx_desc_start));

	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+32) = (uint32_t)(rx_mem_loc2_buf1);
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+36) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+40) = (uint32_t)(rx_mem_loc2_buf2);
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+44) = (uint32_t)0xC1000000;
	//clean_invalidate_cache((uint32_t volatile *)(rx_desc_start+32));

	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+64) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+68) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+72) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+76) = (uint32_t)0x0;
	//clean_invalidate_cache((uint32_t volatile *)(rx_desc_start+64));

	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+96) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+100) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+104) = (uint32_t)0x0;
	   *(uint32_t volatile *)(intptr_t)(rx_desc_start+108) = (uint32_t)0x0;
	//clean_invalidate_cache((uint32_t volatile *)(rx_desc_start+96));
#ifndef __DV_TEST__
	flush_dcache_range((uintptr_t)rx_desc_start, 112);
	inv_dcache_range((uintptr_t)rx_desc_start, 112);
#endif

	   //test_start();

	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CSR_SW_CTRL, 0x100);// enable response error for register accesses
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CSR_SW_CTRL) != 0x100)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD1); //AGIRISH
	     while(1);
	   }

	//asserting software reset
	   read_data = 0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_MODE_SWR;
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD2); //AGIRISH
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_MODE, read_data);
	   while(read_data != 0x0)//polling for 0 which indicates that reset operation completed
	   {
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_MODE);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD3); //AGIRISH
	   }
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD4); //AGIRISH

	 //address aligned burst is enabled
	   read_data = 0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_AAL; //AGIRISH WHAT HAPPENS IF WE DONT SET  THIS?
	   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_FB) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN4) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN8) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE_BLEN16);  //fixed burst length is enabled with burst length = 4/8/16 //can randomize these when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_DMA_SYSBUS_MODE) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD5); //AGIRISH
	     while(1);
	   }

	//keeping TX desc ring length to minimum supported value for now
	   read_data = 0x5 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_RING_LENGTH_TDRL;//randomize when using randomization //descriptors keep getting loaded irrespective of desc_first/desc_last bit
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_RING_LENGTH, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_RING_LENGTH) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD6); //AGIRISH
	     while(1);
	   }
	//keeping RX desc ring length to minimum supported value for now
	   read_data = 0x3 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL2_RDRL;//randomize when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL2, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL2) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD7); //AGIRISH
	     while(1);
	   }

	   //setting start address of TX first descriptor
	   //read_data_str = ARM_REG_READ(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG46);
	   read_data_str = tx_desc_start;
	   read_data_str = read_data_str >> 0x2;
	   read_data = read_data_str << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_LIST_ADDRESS_TDESLA;//2 LSBs are 0 by dfault => actual address is 0x00200000 //randomize when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_LIST_ADDRESS, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_LIST_ADDRESS) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD8); //AGIRISH
	     while(1);
	   }

	   //setting start address of TX last descriptor
	   read_data = (read_data_str + 0x1000) << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_TAIL_POINTER_TDTP;//2 LSBs are 0 by dfault => actual address is 0x00200040 //randomize when using randomization
	   //read_data = (read_data_str + 16) << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_TAIL_POINTER_TDTP;//2 LSBs are 0 by dfault => actual address is 0x00200040 //randomize when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_TAIL_POINTER, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TXDESC_TAIL_POINTER) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD9); //AGIRISH
	     while(1);
	   }

	   //setting start address of RX first descriptor
	   //read_data_str = ARM_REG_READ(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG47);
	   read_data_str = rx_desc_start;
	   read_data_str = read_data_str >> 0x2;
	   read_data = (read_data_str) << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_LIST_ADDRESS_RDESLA;//2 LSBs are 0 by dfault => actual address is 0x00208000 //randomize when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_LIST_ADDRESS, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_LIST_ADDRESS) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD10); //AGIRISH
	     while(1);
	   }

	   //setting start address of RX last descriptor
	   read_data = (read_data_str + 0x1000) << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_TAIL_POINTER_RDTP;//2 LSBs are 0 by dfault => actual address is 0x00208040 //randomize when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_TAIL_POINTER, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RXDESC_TAIL_POINTER) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD11); //AGIRISH
	     while(1);
	   }

	   //setting descriptor skip length to 4
	   read_data = 0x4 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_CONTROL_DSL;//few fields can randomize when using randomization
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD12); //AGIRISH
	     while(1);
	   }

	   //setting TX programmable burst length max to 16
	   read_data = 0x10 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_TXPBL;
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD13); //AGIRISH
	     while(1);
	   }

	   //setting RX programmable burst length max to 16
	   read_data = 0x10 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_RXPBL;
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD14); //AGIRISH
	     while(1);
	   }

	   //enabling all DMA interrupts
	   read_data = (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_NIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_AIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_CDEE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_FBEE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_ERIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RWTE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RSE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RBUE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_RIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TBUE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TXSE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_TIE) ;//commenting out early transmit interrupt because it is expected (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE_ETIE), still status will update but abnormal interrupt will not be generated
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_INTERRUPT_ENABLE) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD15); //AGIRISH
	     while(1);
	   }

	   //enabling transmit store and forward => transmission starts only after entire packet in queue
	   read_data = 0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_TSF; //TTC is not set to value because TSF is set here
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE, read_data);
	   read_data = read_data | (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_RESERVED_TXQEN) | (0x3F << BITP_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE_RESERVED_TQS); //TXQEN and TQS are RO reserved bits that are set to default values
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MTL_Q0_MTL_TXQ0_OPERATION_MODE) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD16); //AGIRISH
	     while(1);
	   }

	    //setting MAC address high (to match value in end_of_elab phase of a55_subsys_env file) ///IMPORTANT: Need to write this reg first before MAC_ADDR_LO else will be missed
	   read_data = 0x4800 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH_ADDRHI;
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH, read_data);
	   read_data = read_data | ((uint32_t)0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH_AE); //address0 is always enabled
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_HIGH) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD17); //AGIRISH
	     while(1);
	   }

	  //setting MAC address low (to match value in end_of_elab phase of a55_subsys_env file)
	   read_data = 0x48004800 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_LOW_ADDRLO;
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_LOW, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_ADDRESS0_LOW) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD18); //AGIRISH
	     while(1);
	   }

	   //passing all multicast addresses as well and receive all
	   read_data = ((uint32_t)0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER_RA) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER_PM); //remaining bits (and PM as well) could be randomized but are they required to be tested?
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PACKET_FILTER) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD19); //AGIRISH
	     while(1);
	   }


	   //setting pause time for TX packet and flow control busy
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL);
	   read_data = read_data & BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA;
	   while(read_data != 0x00)//ensuring that FCB bit is not preset
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD20); //AGIRISH
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL);
	     read_data = read_data & BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA;
	   }
	   read_data = (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_FCB_BPA) | (0xff < BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL_PT);//Pause Time set to 256 (some arbitary value)
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_Q0_TX_FLOW_CTRL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD21); //AGIRISH
	     while(1);
	   }

	   //enabling all MAC interrupts
	   read_data = (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_MDIOIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RXSTSIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_TXSTSIE) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_LPIIE) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_PMTIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_PHYIE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE_RGSMIIIE);
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_ENABLE) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD22); //AGIRISH
	     while(1);
	   }

	   //MAC config
	   //read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG45);
	   read_data = 0x0;
//	   is_it_not_two_frame = read_data;
	//1G jumbo
	   if(read_data == 0x1) {
	     read_data = (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_JE) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES); //Jumbo packet is enabled :: CST - stripping CRC //FES - 0 => 1G, not 10M, PS - 0 => 1G, not 100M  //other bits can be randomized
	   }
	//10M Jumbo
	   else if(read_data == 0x3) {
	     read_data = (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_JE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES); //Jumbo packet is enabled :: CST - stripping CRC //FES - 0 => 1G, not 10M, PS - 0 => 1G, not 100M  //other bits can be randomized
	   }
	//100M Jumbo
	   else if(read_data == 0x5) {
	     read_data = (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_JE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES); //Jumbo packet is enabled :: CST - stripping CRC //FES - 0 => 1G, not 10M, PS - 0 => 1G, not 100M  //other bits can be randomized
	   }
	//10M basic
	   else if(read_data == 0x2) {
	     read_data = (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES); //CST - stripping CRC //FES - 0 => 1G, not 10M, PS - 0 => 1G, not 100M  //other bits can be randomized
	   }
	//100M basic
	   else if(read_data == 0x4) {
	     read_data = (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES); //CST - stripping CRC //FES - 0 => 1G, not 10M, PS - 0 => 1G, not 100M  //other bits can be randomized
	   }
	//1G basic
	   else if(read_data == 0x0) {
	     read_data = (0x2 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_SARC) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_CST) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_PS) | (0x0 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_FES); //CST - stripping CRC //FES - 0 => 1G, not 10M, PS - 0 => 1G, not 100M  //other bits can be randomized
	   }
#if 1
	   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_DM); //full duplex mode set by default
#endif
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION, read_data);
#if 0
	   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_DM); //full duplex mode set by default
#endif
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD23); //AGIRISH
	     while(1);
	   }

	  //MAC config to start operation
	   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_TE) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION_RE);
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_CONFIGURATION) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD24); //AGIRISH
	     while(1);
	   }


	    //starting TX DMA transmission
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL);
	   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST);
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD25); //AGIRISH
	     while(1);
	   }

	   //starting RX DMA transmission
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL);
	   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR);
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD26); //AGIRISH
	     while(1);
	   }
	   int read_data1;
	   read_data1 = *(int32_t volatile *)(uintptr_t)rx_mem_loc1_buf11;
	   ARM_REG_WRITE(ADDR_DIGITAL_CORE_SPI_ONLY_REGS_SCRATCH_REGS_SCRATCH_REGS5, read_data1);
	   	   if(read_data1== 0XAB89EFCD)
	   	   {
	   	   *(uint8_t volatile *)(ADDR_DIGITAL_CORE_SPI_ONLY_REGS_SCRATCH_REGS_SCRATCH_REGS0) = (uint8_t)0xAC;
	   	   }
	   	   else
	   	   {
	   		*(uint8_t volatile *)(ADDR_DIGITAL_CORE_SPI_ONLY_REGS_SCRATCH_REGS_SCRATCH_REGS0) = (uint8_t)0xFF;
	   	   }
	   ARM_REG_WRITE(ADDR_DIGITAL_CORE_SPI_ONLY_REGS_SCRATCH_REGS_SCRATCH_REGS5, read_data1);
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS);
	   read_data = read_data & (~(BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU));
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG2, 0xcab);
	   while(read_data != (BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_NIS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_ETI))//polling for status bits that are expected
	   {

	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB1); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, read_data); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, (BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_NIS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_ETI)); //AGIRISH
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB2); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, read_data); //AGIRISH
	     read_data = read_data & (~(BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU));
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB3); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, read_data); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG3, 0xface);
	     read_data1 = *(int32_t volatile *)(uintptr_t)rx_mem_loc1_buf11;
	     ARM_REG_WRITE(ADDR_DIGITAL_CORE_SPI_ONLY_REGS_SCRATCH_REGS_SCRATCH_REGS5, read_data1);


	   }

	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG3,0xcafe);
	   read_data = read_data | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU;
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS, read_data);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB4); //AGIRISH

	    //stopping TX DMA transmission
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL);
	   read_data = read_data & (~(BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST));
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD27); //AGIRISH
	     while(1);
	   }

	   //stopping RX DMA transmission
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL);
	   read_data = read_data & (~(BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR));
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL, read_data);
	   if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL) != read_data)
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD28); //AGIRISH
	     while(1);
	   }
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG4,0xadda);
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS);
	   while(read_data != (                                                           BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RPS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_AIS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TPS))//polling for abnormal interrupt because stopping TX + RX DMA leads to respective process stopped interrupts
	   //while(read_data != ( BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RBU | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RPS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_AIS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TPS))//polling for abnormal interrupt because stopping TX + RX DMA leads to respective process stopped interrupts
	   {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD29); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB1); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, read_data); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB2); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG7,0xabd);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, ( BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RPS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_AIS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TPS)); //AGIRISH
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xAB2); //AGIRISH
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, read_data); //AGIRISH
	   }
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG6,0xdada);
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS, read_data);

	   asm("NOP");
	   asm("NOP");
	   asm("NOP");
	   asm("NOP");

	   //enabling power down and magic packet enable
	//   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS);
	//   read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS_PWRDWN) | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS_MGKPKTEN);
	//   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS, read_data);

	   //handshake with sv to ensure that magic packet is sent only after configuration is complete
	   ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG45, 0xdeadbeef);


	   //polling for magic packet receive interrupt //poll this first since PMT control status bit is self clearing and possibility arises of read being missed
	//   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_STATUS);
	//   read_data = read_data & BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_STATUS_PMTIS;
	//   while(read_data == 0)
	//   {
	//     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD30); //AGIRISH
	//     asm("NOP");
	////     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_STATUS);
	//     read_data = read_data & BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_INTERRUPT_STATUS_PMTIS;
	//   }

	   //polling for magic packet receive after checking for interrupt
	//   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS);
	//   read_data = read_data & BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS_MGKPRCVD;
	//   while(read_data == 0)
	//   {
	//     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD31); //AGIRISH
	//     asm("NOP");
	//     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS);
	//     read_data = read_data & BITM_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PMT_CONTROL_STATUS_MGKPRCVD;
	//   }

	   //reading phyif control status in order to temporarily clear sbd_intr
	   read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_MAC_MAC_PHYIF_CONTROL_STATUS);
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");
	     asm("NOP");

#if 0
	   if(is_it_not_two_frame == 0x0 || is_it_not_two_frame == 0x4) {
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG45);
	     while(read_data != 0x22222222) {
	       read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG45);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD32); //AGIRISH
	     }

	     //starting TX DMA transmission
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL);
	     read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL_ST);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL, read_data);
	     if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_TX_CONTROL) != read_data)
	     {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD33); //AGIRISH
	       while(1);
	     }

	     //starting RX DMA transmission
	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL);
	     read_data = read_data | (0x1 << BITP_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL_SR);
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL, read_data);
	     if(read_func(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_RX_CONTROL) != read_data)
	     {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD34); //AGIRISH
	       while(1);
	     }

	     read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS);
	     read_data = read_data & (~(BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU));
	     while(read_data != (BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_NIS | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_ETI | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_RBU | BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_AIS))//polling for status bits that are expected //abnormal interrupt because ETI and RBU
	     {
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_A55_SYS_CFG_SCRATCH_REG0, 0xBAD35); //AGIRISH
	       read_data = ARM_REG_READ(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS);
	       read_data = read_data & (~(BITM_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS_TBU));
	     }
	     ARM_REG_WRITE(ADDR_PROC_DFE_PERIP_EMAC_1G_DWC_EQOS_TOP_MAP_EQOS_DMA_CH0_DMA_CH0_STATUS, read_data);
	   }
#endif

#ifdef __DV_TEST__
        if(read_reg32(0x3804F210)==0xab89efcd)
	{
		*(uint8_t volatile *)(0x18290205) = 0xad;
	}
        else
        {
               *(uint8_t volatile *)(0x18290205) = 0xff;
        }
   //END OF TEST
    //test_report();
#endif

        return 0;

}
