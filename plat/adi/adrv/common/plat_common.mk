#
# Copyright (c) 2017-2021, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Add CRC extensions. Needed for plat_bootctrl.c
BL1_CPPFLAGS += -march=armv8-a+crc
BL2_CPPFLAGS += -march=armv8-a+crc
BL31_CPPFLAGS += -march=armv8-a+crc

PLAT_PARTITION_MAX_ENTRIES := 32
$(eval $(call add_define,PLAT_PARTITION_MAX_ENTRIES))

HW_ASSISTED_COHERENCY	:=	1
USE_COHERENT_MEM	:=	0

ifeq (${TEST_FRAMEWORK}, 1)
TF_CFLAGS		+=	-DTEST_FRAMEWORK
TF_CFLAGS_aarch64	+=	-DTEST_FRAMEWORK
ASFLAGS			+=	-DTEST_FRAMEWORK
ASFLAGS_aarch64		+=	-DTEST_FRAMEWORK
endif

PLAT_INCLUDES		:=	-Iinclude/plat/common -Iplat/adi/adrv/common/include

PLAT_BL_COMMON_SOURCES	:=	common/tf_crc32.c \
				drivers/adi/te/adi_te_interface.c \
				drivers/auth/img_parser_mod.c \
				drivers/delay_timer/delay_timer.c \
				drivers/delay_timer/generic_delay_timer.c \
				drivers/io/io_storage.c \
				drivers/io/io_block.c \
				drivers/io/io_fip.c \
				drivers/io/io_memmap.c \
				drivers/io/io_mtd.c \
				drivers/mmc/mmc.c \
				drivers/mtd/nor/spi_nor.c \
				drivers/mtd/spi-mem/spi_mem.c \
				drivers/partition/partition.c \
				drivers/partition/gpt.c \
				lib/cpus/aarch64/cortex_a55.S \
				lib/fconf/fconf.c \
				plat/adi/adrv/common/aarch64/plat_helpers.S \
				plat/adi/adrv/common/plat_boot.c \
				plat/adi/adrv/common/plat_bootcfg.c \
				plat/adi/adrv/common/plat_bootctrl.c \
				plat/adi/adrv/common/plat_err.c \
				plat/adi/adrv/common/plat_io_storage.c \
				plat/adi/adrv/common/plat_stack_protector.c \
				plat/adi/adrv/common/plat_trusted_boot.c \
				plat/common/aarch64/crash_console_helpers.S

ifneq (${TRUSTED_BOARD_BOOT},0)
PLAT_BL_COMMON_SOURCES	+=	drivers/auth/auth_mod.c \
				drivers/auth/crypto_mod.c \
				drivers/auth/tbbr/tbbr_cot_common.c
endif

# Include translation tables
include lib/xlat_tables_v2/xlat_tables.mk
PLAT_BL_COMMON_SOURCES  +=      ${XLAT_TABLES_LIB_SRCS}

# Include FDT library to allow reading of config files
include lib/libfdt/libfdt.mk
PLAT_BL_COMMON_SOURCES		+=	common/fdt_wrappers.c \
					common/uuid.c

# Include GICv3 driver
GICV3_SUPPORT_GIC600    :=	1
include drivers/arm/gic/v3/gicv3.mk
PLAT_GIC_SOURCES        :=	${GICV3_SOURCES} \
				plat/adi/adrv/common/plat_int_gicv3.c \
				plat/common/plat_gicv3.c

ifeq (${TEST_FRAMEWORK}, 1)
PLAT_BL_COMMON_SOURCES	+=	drivers/adi/test/test_framework.c
endif

BL1_SOURCES		+=	plat/adi/adrv/common/plat_bl1_setup.c

ifneq (${TRUSTED_BOARD_BOOT},0)
BL1_SOURCES		+=	drivers/auth/tbbr/tbbr_cot_bl1.c
endif

BL2_SOURCES		+=	common/desc_image_load.c \
				drivers/arm/tzc/tzc400.c \
				plat/adi/adrv/common/aarch64/plat_bl2_mem_params_desc.c \
				plat/adi/adrv/common/plat_bl2_setup.c \
				plat/adi/adrv/common/plat_image_load.c \
				plat/adi/adrv/common/plat_security.c
ifeq (${DEBUG},1)
BL2_SOURCES		+=	plat/adi/adrv/common/plat_cli.c
endif

ifneq (${TRUSTED_BOARD_BOOT},0)
BL2_SOURCES		+=	drivers/auth/tbbr/tbbr_cot_bl2.c
endif

BL31_SOURCES		+=	${PLAT_GIC_SOURCES} \
				plat/common/plat_psci_common.c \
				plat/adi/adrv/common/plat_bl31_setup.c \
				plat/adi/adrv/common/plat_interrupts.c \
				plat/adi/adrv/common/plat_pintmux_svc.c \
				plat/adi/adrv/common/plat_pinctrl_svc.c \
				plat/adi/adrv/common/plat_pm.c \
				plat/adi/adrv/common/plat_sip_svc.c \
				plat/adi/adrv/common/plat_wdt_svc.c

ifeq (${EL3_EXCEPTION_HANDLING}, 1)
BL31_SOURCES		+=	plat/common/aarch64/plat_ehf.c \
						plat/adi/adrv/common/plat_sdei.c
endif

ifneq (${TRUSTED_BOARD_BOOT},0)
BL31_SOURCES		+=	drivers/auth/mbedtls/mbedtls_common.c
endif

ifneq (${TRUSTED_BOARD_BOOT},0)
include drivers/auth/mbedtls/mbedtls_x509.mk
include drivers/auth/mbedtls/mbedtls_crypto.mk
endif

# Enable workarounds for selected Cortex-A55 errata.
ERRATA_A55_1530923	:=	1
