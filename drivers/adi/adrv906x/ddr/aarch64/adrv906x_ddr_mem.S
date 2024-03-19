/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#define VAL(str) #str
#define TOSTRING(str) VAL(str)

.global adi_dmem0_1d
.global adi_dmem0_2d
.global adi_sec_dmem0_1d
.global adi_sec_dmem0_2d
.global adi_imem_1d
.global adi_imem_2d
.global adi_dmem0_1d_end
.global adi_dmem0_2d_end
.global adi_sec_dmem0_1d_end
.global adi_sec_dmem0_2d_end
.global adi_imem_1d_end
.global adi_imem_2d_end

/*These variables must be kept in the same order to keep the get_mem_info function from breaking in ddr_phy_helpers.c. If you need to add to this list, add to the end*/
adi_dmem0_1d:
	.incbin TOSTRING(PRI_1D_DMEM_PSTATE0_BIN)
adi_dmem0_1d_end:

adi_dmem0_2d:
	.incbin TOSTRING(PRI_2D_DMEM_PSTATE0_BIN)
adi_dmem0_2d_end:

adi_sec_dmem0_1d:
	.incbin TOSTRING(SEC_1D_DMEM_PSTATE0_BIN)
adi_sec_dmem0_1d_end:

adi_sec_dmem0_2d:
	.incbin TOSTRING(SEC_2D_DMEM_PSTATE0_BIN)
adi_sec_dmem0_2d_end:

adi_imem_1d:
	.incbin TOSTRING(DDR_1D_IMEM_BIN)
adi_imem_1d_end:

adi_imem_2d:
	.incbin TOSTRING(DDR_2D_IMEM_BIN)
adi_imem_2d_end:
