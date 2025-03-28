/*
 * Copyright(c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <adrv906x_device_profile.h>
#include <drivers/adi/adrv906x/ddr/ddr.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "../../ddr_config.h"
#include "../../ddr_regmap.h"

/**
 *******************************************************************************
 * Function: ddr_pre_reset_init
 *
 * @brief      Writes static registers to DDR controller
 *
 * @details    Writes static registers to DDR controller
 *
 * Reference to other related functions
 *
 * Notes:
 *
 *******************************************************************************
 */
ddr_error_t ddr_2gb_1rank_x16_1gbx16_3200_pre_reset_init(uintptr_t base_addr_ctrl, bool ecc)
{
	ddr_error_t returnVal = ERROR_DDR_NO_ERROR;
	int i;

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_MSTR, 0x81040210);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_MRCTRL0, 0x40003030);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_MRCTRL1, 0x00008043);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_MRCTRL2, 0x4d692486);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRCTL, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PWRTMG, 0x00400004);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_HWLPCTL, 0x00330002);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RFSHCTL0, 0x00210000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RFSHCTL1, 0x003a00a1);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RFSHCTL3, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RFSHTMG, 0x00c301b8);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG0, 0x013f7f10);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG1, 0x00001fa2);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCTL, 0x00000300);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONADDR0, 0x01000fc0);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONADDR1, 0x20019294);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_CRCPARCTL0, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_CRCPARCTL1, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT0, 0x00020002);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT1, 0x00010002);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT3, 0x0c510103);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT4, 0x00280400);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT5, 0x00110000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT6, 0x00000640);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_INIT7, 0x00001059);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DIMMCTL, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RANKCTL, 0x0000022f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_RANKCTL1, 0x00000002);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG0, 0x1618361a);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG1, 0x00050624);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG2, 0x080b0610);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG3, 0x0000400c);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG4, 0x0a04060b);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG5, 0x08080504);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG8, 0x08081210);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG9, 0x0002050c);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG10, 0x000e0d05);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG11, 0x1f0b011f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG12, 0x0c000010);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DRAMTMG15, 0x80000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ZQCTL0, 0xd1000040);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ZQCTL1, 0x000dd330);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFITMG0, 0x0491820b);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFITMG1, 0x02070303);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFILPCFG0, 0x03005001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFILPCFG1, 0x00000031);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIUPD0, 0xc0400018);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIUPD1, 0x00090017);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIUPD2, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIMISC, 0x00000051);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFITMG2, 0x0000110b);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFITMG3, 0x00000008);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBICTL, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DFIPHYMSTR, 0x58000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP0, 0x0000001f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP1, 0x003f0909);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP2, 0x01010100);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP3, 0x01010101);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP4, 0x00001f1f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP5, 0x070f0707);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP6, 0x07070707);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP7, 0x00000f07);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP8, 0x00003f01);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP9, 0x07070707);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP10, 0x07070707);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP11, 0x00000007);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ODTCFG, 0x06000610);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ODTMAP, 0x00003231);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SCHED, 0x80b11f18);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SCHED1, 0x00002000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PERFHPR1, 0x0f000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PERFLPR1, 0x80000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_PERFWR1, 0x08000800);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SCHED3, 0x04040208);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SCHED4, 0x08400810);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SCHED5, 0x10000104);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG0, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBG1, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_DBGCMD, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTL, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_SWCTLSTATIC, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_POISONCFG, 0x00100001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADVECCINDEX, 0x000001ca);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONPAT0, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCPOISONPAT2, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCTRL_0, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCTRL_1, 0x00000001);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_PWRTMG, 0x00400004);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_RFSHCTL0, 0x00210000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_RFSHTMG, 0x00c301b8);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_INIT3, 0x0c510103);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_INIT4, 0x00280400);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_INIT6, 0x00000640);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_INIT7, 0x00001059);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_RANKCTL, 0x0000022f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_RANKCTL1, 0x00000002);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG0, 0x1618361a);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG1, 0x00050624);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG2, 0x080b0610);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG3, 0x0000400c);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG4, 0x0a04060b);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG5, 0x08080504);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG8, 0x08081210);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG9, 0x0002050c);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG10, 0x000e0d05);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG11, 0x1f0b011f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG12, 0x0c000010);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DRAMTMG15, 0x80000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_ZQCTL0, 0xd1000040);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DFITMG0, 0x0491820b);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DFITMG1, 0x02070303);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DFITMG2, 0x0000110b);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_DFITMG3, 0x00000008);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_FREQ1_FREQ1_ODTCFG, 0x06000610);
	for (i = 0; i < ADI_DDR_CTRL_TIMEOUT; i++) {
		if (mmio_read_32(base_addr_ctrl + DDR_UMCTL2_REGS_RFSHCTL3) == 0x00000000)
			break;
		else
			mdelay(1);
	}

	if (i == ADI_DDR_CTRL_TIMEOUT)
		return ERROR_DDR_CTRL_INIT_FAILED;

	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCCFG, 0x00000000);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_0, 0x0000000f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_1, 0x0000000f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_0, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_1, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_0, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_1, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_0, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_1, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_0, 0x0000400f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_1, 0x0000400f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_0, 0x0000500f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_1, 0x0000500f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_0, 0x0000500f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_1, 0x0000500f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_0, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_1, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_0, 0x0000500f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_1, 0x0000500f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_0, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_1, 0x0000100f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_0, 0x0000400f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGR_1, 0x0000400f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_0, 0x0000000f);
	mmio_write_32(base_addr_ctrl + DDR_UMCTL2_MP_PCFGW_1, 0x0000000f);
	/*These are overrides needed for turning on ECC, and adjusting the address map to tell the controller not to write to the space reserved for the inline ECC */
	if (ecc) {
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ECCCFG0, 0x013f7f34);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP0, 0x001f1f1f);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP1, 0x003f0c0c);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP2, 0x01010100);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP3, 0x14141401);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP4, 0x00001f1f);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP5, 0x040f0202);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP6, 0x04040404);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP7, 0x00000f04);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP8, 0x00003f01);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP9, 0x02020202);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP10, 0x04040404);
		mmio_write_32(base_addr_ctrl + DDR_UMCTL2_REGS_ADDRMAP11, 0x001f1f04);
	}

	return returnVal;
}
