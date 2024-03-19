#
# Copyright (c) 2017-2021, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

DDR_SOURCES		:=	drivers/adi/adrv906x/ddr/aarch64/adrv906x_ddr_mem.S\
				drivers/adi/adrv906x/ddr/ddr.c \
				drivers/adi/adrv906x/ddr/ddr_config.c \
				drivers/adi/adrv906x/ddr/ddr_debug.c \
				drivers/adi/adrv906x/ddr/ddr_messages.c \
				drivers/adi/adrv906x/ddr/ddr_phy_helpers.c \
				drivers/adi/adrv906x/ddr/ddr_post_reset_init_syslvl.c \


ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_2rank_x8_2gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_4GB_2RANK_X8_2GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_2rank_x16_1gbx16_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_4GB_2RANK_X16_1GBX16_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_2rank_x16_1gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_4GB_2RANK_X16_1GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_1rank_x16_2gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_4GB_1RANK_X16_2GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x16_1gbx16_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X16_1GBX16_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x8_2gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X8_2GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_2rank_x8_1gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_2GB_2RANK_X8_1GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X16_1GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_multi_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X16_1GBX8_MULTI_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_2rank_x8_2gbx8_3200)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_2GB_2RANK_X8_2GBX8_3200))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_2rank_x8_2gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_4GB_2RANK_X8_2GBX8_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_2rank_x16_1gbx16_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_4GB_2RANK_X16_1GBX16_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_2rank_x16_1gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_4GB_2RANK_X16_1GBX8_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_4GB_1rank_x16_2gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_4GB_1RANK_X16_2GBX8_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x16_1gbx16_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X16_1GBX16_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x8_2gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X8_2GBX8_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_2rank_x8_1gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_2GB_2RANK_X8_1GBX8_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X16_1GBX8_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_multi_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_PRIMARY_2GB_1RANK_X16_1GBX8_MULTI_1600))
else ifeq ($(DDR_PRIMARY_CONFIG), DDR_2GB_2rank_x8_2gbx8_1600)
PRI_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_1DDMEM
PRI_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_pre_reset.c
DDR_PRIMARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_PRIMARY_2GB_2RANK_X8_2GBX8_1600))
endif


ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_2rank_x8_2gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_3200/4GB_2rank_x8_2gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_4GB_2RANK_X8_2GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_2rank_x16_1gbx16_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_3200/4GB_2rank_x16_1gbx16_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_4GB_2RANK_X16_1GBX16_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_2rank_x16_1gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_3200/4GB_2rank_x16_1gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_4GB_2RANK_X16_1GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_1rank_x16_2gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_3200/4GB_1rank_x16_2gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_4GB_1RANK_X16_2GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x16_1gbx16_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_3200/2GB_1rank_x16_1gbx16_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X16_1GBX16_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x8_2gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_3200/2GB_1rank_x8_2gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X8_2GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_2rank_x8_1gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_3200/2GB_2rank_x8_1gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_2GB_2RANK_X8_1GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_3200/2GB_1rank_x16_1gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X16_1GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_multi_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_3200/2GB_1rank_x16_1gbx8_multi_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X16_1GBX8_MULTI_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_2rank_x8_2gbx8_3200)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_3200/2GB_2rank_x8_2gbx8_3200_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_2GB_2RANK_X8_2GBX8_3200))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_2rank_x8_2gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x8_2gbx8_1600/4GB_2rank_x8_2gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_4GB_2RANK_X8_2GBX8_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_2rank_x16_1gbx16_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx16_1600/4GB_2rank_x16_1gbx16_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_4GB_2RANK_X16_1GBX16_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_2rank_x16_1gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_2rank_x16_1gbx8_1600/4GB_2rank_x16_1gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_4GB_2RANK_X16_1GBX8_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_4GB_1rank_x16_2gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/4GB_1rank_x16_2gbx8_1600/4GB_1rank_x16_2gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_4GB_1RANK_X16_2GBX8_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x16_1gbx16_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx16_1600/2GB_1rank_x16_1gbx16_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X16_1GBX16_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x8_2gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x8_2gbx8_1600/2GB_1rank_x8_2gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X8_2GBX8_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_2rank_x8_1gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_1gbx8_1600/2GB_2rank_x8_1gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_2GB_2RANK_X8_1GBX8_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_1600/2GB_1rank_x16_1gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X16_1GBX8_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_1rank_x16_1gbx8_multi_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_1rank_x16_1gbx8_multi_1600/2GB_1rank_x16_1gbx8_multi_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = true
$(eval $(call add_defines, DDR_SECONDARY_2GB_1RANK_X16_1GBX8_MULTI_1600))
else ifeq ($(DDR_SECONDARY_CONFIG), DDR_2GB_2rank_x8_2gbx8_1600)
SEC_1D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_1DDMEM
SEC_2D_DMEM_PSTATE0_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_2DDMEM
DDR_SOURCES +=	drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_phy.c \
				drivers/adi/adrv906x/ddr/configs/2GB_2rank_x8_2gbx8_1600/2GB_2rank_x8_2gbx8_1600_pre_reset.c
DDR_SECONDARY_ECC_ISX16 = false
$(eval $(call add_defines, DDR_SECONDARY_2GB_2RANK_X8_2GBX8_1600))
endif


DDR_1D_IMEM_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/imems/ddr4_pmu_train_imem.bin
DDR_2D_IMEM_BIN = ${TF_A_DIR}/drivers/adi/adrv906x/ddr/imems/ddr4_2d_pmu_train_imem.bin

$(eval $(call add_defines,\
	DDR_PRIMARY_ECC_ISX16 \
	DDR_SECONDARY_ECC_ISX16 \
	))
ASFLAGS += -D PRI_1D_DMEM_PSTATE0_BIN=${PRI_1D_DMEM_PSTATE0_BIN} -D PRI_2D_DMEM_PSTATE0_BIN=${PRI_2D_DMEM_PSTATE0_BIN} -D SEC_1D_DMEM_PSTATE0_BIN=${SEC_1D_DMEM_PSTATE0_BIN} -D SEC_2D_DMEM_PSTATE0_BIN=${SEC_2D_DMEM_PSTATE0_BIN} -D DDR_1D_IMEM_BIN=${DDR_1D_IMEM_BIN} -D DDR_2D_IMEM_BIN=${DDR_2D_IMEM_BIN}


