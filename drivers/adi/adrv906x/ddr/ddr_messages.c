/*
 * Copyright(c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX - License - Identifier: BSD - 3 - Clause
 */
#include "ddr_messages.h"
const ddr_message_t ddr_1d_log_messages[ONED_TRAINING_MESSAGE_STRING_COUNT] = {
	{ 0x00160000, "PMU4: TxDqDly Passing Regions (EyeLeft EyeRight -> EyeCenter) Units=1/32 UI\n"	   },
	{ 0x00170005, "PMU4: DB %d Lane %d: %3d %3d -> %3d\n"						   },
	{ 0x00180002, "PMU2: TXDQ delayLeft[%2d] = %3d (DISCONNECTED)\n"				   },
	{ 0x00190004, "PMU2: TXDQ delayLeft[%2d] = %3d oopScaled = %3d selectOop %d\n"			   },
	{ 0x001a0002, "PMU2: TXDQ delayRight[%2d] = %3d (DISCONNECTED)\n"				   },
	{ 0x001b0004, "PMU2: TXDQ delayRight[%2d] = %3d oopScaled = %3d selectOop %d\n"			   },
	{ 0x001c0003, "PMU: Error: Dbyte %d lane %d txDqDly passing region is too small (width = %d)\n"	   },
	{ 0x001d0000, "PMU4: TxDqDly Passing Regions (EyeLeft EyeRight -> EyeCenter) Units=1/32 UI\n"	   },
	{ 0x001e0002, "PMU4: DB %d Lane %d: (DISCONNECTED)\n"						   },
	{ 0x001f0005, "PMU4: DB %d Lane %d: %3d %3d -> %3d\n"						   },
	{ 0x00200002, "PMU3: Running 1D search csn %d for DM Right/NotLeft(%d) eye edge\n"		   },
	{ 0x00210002, "PMU3: WrDq DM byte%2d with Errcnt %d\n"						   },
	{ 0x00220002, "PMU3: WrDq DM byte%2d avgDly 0x%04x\n"						   },
	{ 0x00230002, "PMU1: WrDq DM byte%2d with Errcnt %d\n"						   },
	{ 0x00240001, "PMU: Error: Dbyte %d txDqDly DM training did not start inside the eye\n"		   },
	{ 0x00250000, "PMU4: DM TxDqDly Passing Regions (EyeLeft EyeRight -> EyeCenter) Units=1/32 UI\n"   },
	{ 0x00260002, "PMU4: DB %d Lane %d: (DISCONNECTED)\n"						   },
	{ 0x00270005, "PMU4: DB %d Lane %d: %3d %3d -> %3d\n"						   },
	{ 0x00280003, "PMU: Error: Dbyte %d lane %d txDqDly DM passing region is too small (width = %d)\n" },
	{ 0x00360000, "PMU4: RxClkDly Passing Regions (EyeLeft EyeRight -> EyeCenter)\n"		   },
	{ 0x00370002, "PMU4: DB %d nibble %d: (DISCONNECTED)\n"						   },
	{ 0x00380005, "PMU4: DB %d nibble %d: %3d %3d -> %3d\n"						   },
	{ 0x00390003, "PMU: Error: Dbyte %d nibble %d rxClkDly passing region is too small (width = %d)\n" },
};
const ddr_message_t ddr_2d_log_messages[TWOD_TRAINING_MESSAGE_STRING_COUNT] = {
	{ 0x0014001f, "PMU4: %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d >%3d< %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d\n" },
	{ 0x00150002, "PMU3: Voltage Range = [%d, %d]\n"												      },
	{ 0x00160004, "PMU4: -- DB%d L%d -- centers: delay = %d, voltage = %d \n"									      },
	{ 0x00170001, "PMU5: <<KEY>> 0 TxDqDlyTg%d <<KEY>> coarse(6:6) fine(5:0)\n"									      },
	{ 0x00180001, "PMU5: <<KEY>> 0 messageBlock VrefDqR%d <<KEY>> MR14(6:0)\n"									      },
	{ 0x00190001, "PMU5: <<KEY>> 0 messageBlock VrefDqR%d <<KEY>> MR6(6:0)\n"									      },
	{ 0x001a0001, "PMU5: <<KEY>> 0 RxClkDlyTg%d <<KEY>> fine(5:0)\n"										      },
	{ 0x001e0004, "PMU4: -------- %dD Write Scanning TG %d (CS 0x%x) Lanes 0x%03x --------\n"							      },
	{ 0x001f0002, "PMU0: training lanes 0x%03x using lanes 0x%03x\n"										      },
	{ 0x00200003, "PMU4: ------- 2D-DFE Read Scanning TG %d (CS 0x%x) Lanes 0x%03x -------\n"							      },
	{ 0x00210004, "PMU4: ------- %dD Read Scanning TG %d (CS 0x%x) Lanes 0x%03x -------\n"								      },
	{ 0x00220003, "PMU4: TG%d MR1[13,6,5]=0x%x MR6[13,9,8]=0x%x\n"											      },
	{ 0x00230002, "PMU0: training lanes 0x%03x using lanes 0x%03x\n"										      },
	{ 0x00240003, "PMU4: ------- 2D-DFE Read Scanning TG %d (CS 0x%x) Lanes 0x%03x -------\n"							      },
	{ 0x00250004, "PMU4: ------- %dD Read Scanning TG %d (CS 0x%x) Lanes 0x%03x -------\n"								      },
	{ 0x002c0002, "PMU4: Delay Weight = %d, Voltage Weight = %d\n"											      },
};
