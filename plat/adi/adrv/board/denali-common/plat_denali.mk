#
# Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/adi/adrv/adrv906x/plat_adrv906x.mk

PLAT_INCLUDES		+=	-Iplat/adi/adrv/board/denali-common/include

PLAT_BL_COMMON_SOURCES	+=	plat/adi/adrv/board/denali-common/aarch64/adi_dev_rotpk.S \
				plat/adi/adrv/board/denali-common/denali.c \
				plat/adi/adrv/board/denali-common/denali_nor_flash_part.c \
				plat/adi/adrv/board/denali-common/denali_nor_flash_part_def.c

DTC_FLAGS		+=	-i plat/adi/adrv/adrv906x/fdts
DTC_CPPFLAGS		+=	-Iplat/adi/adrv/adrv906x/fdts
