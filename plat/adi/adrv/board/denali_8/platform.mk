#
# Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/adi/adrv/board/denali-common/plat_denali.mk

DTC_FLAGS		+=	-i plat/adi/adrv/board/denali-common/fdts
DTC_CPPFLAGS		+=	-Iplat/adi/adrv/board/denali-common/fdts

# Add HW_CONFIG
FDT_SOURCES             +=      plat/adi/adrv/board/denali_8/fdts/denali.dts
HW_CONFIG               :=      ${BUILD_PLAT}/fdts/denali.dtb
$(eval $(call TOOL_ADD_PAYLOAD,${HW_CONFIG},--hw-config,${HW_CONFIG}))

# Add FW_CONFIG
FDT_SOURCES		+=	plat/adi/adrv/board/denali_8/fdts/denali-fw-config.dts
FW_CONFIG		:=	${BUILD_PLAT}/fdts/denali-fw-config.dtb
$(eval $(call TOOL_ADD_PAYLOAD,${FW_CONFIG},--fw-config,${FW_CONFIG}))
