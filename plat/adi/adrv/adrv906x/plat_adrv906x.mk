#
# Copyright (c) 2017-2021, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include drivers/adi/adrv906x/ddr/ddr.mk
include plat/adi/adrv/common/plat_common.mk

ifdef APP_PACK_ANTI_ROLLBACK_VERSION
BL1_CFLAGS += -DAPP_PACK_ANTI_ROLLBACK_VERSION=${APP_PACK_ANTI_ROLLBACK_VERSION}
endif

PLAT_INCLUDES		+=	-Iplat/adi/adrv/adrv906x/include

PLAT_BL_COMMON_SOURCES	+=	drivers/adi/adrv906x/debug_xbar/debug_xbar.c \
				drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.c \
				drivers/adi/mmc/systemc_pl180_mmc.c \
				drivers/adi/otp/adi_otp.c \
				drivers/adi/spu/adi_spu.c \
				drivers/arm/pl011/aarch64/pl011_console.S \
				plat/adi/adrv/adrv906x/aarch64/adrv906x_helpers.S \
				plat/adi/adrv/adrv906x/adrv906x_boot.c \
				plat/adi/adrv/adrv906x/adrv906x_console.c \
				plat/adi/adrv/adrv906x/adrv906x_device_profile.c \
				plat/adi/adrv/adrv906x/adrv906x_err.c \
				plat/adi/adrv/adrv906x/adrv906x_int_gicv3.c \
				plat/adi/adrv/adrv906x/adrv906x_mmap.c \
				plat/adi/adrv/adrv906x/adrv906x_otp.c \
				plat/adi/adrv/adrv906x/adrv906x_pinctrl.c \
				plat/adi/adrv/adrv906x/adrv906x_pinctrl_init.c \
				plat/adi/adrv/adrv906x/adrv906x_pinmux_source_def.c \
				plat/adi/adrv/adrv906x/adrv906x_pm.c \
				plat/adi/adrv/adrv906x/adrv906x_ras.c \
				plat/adi/adrv/adrv906x/adrv906x_status_reg.c \
				plat/adi/adrv/adrv906x/adrv906x_tsgen.c

BL1_SOURCES		+=	drivers/adi/adrv906x/clk/clk.c \
				drivers/adi/adrv906x/clk/mcs.c \
				drivers/adi/adrv906x/clk/zl30732.c \
				drivers/adi/adrv906x/ddr/ddr_edac.c \
				drivers/adi/adrv906x/gpio/adrv906x_gpio.c \
				drivers/adi/i2c/adi_twi_i2c.c \
				drivers/adi/mmc/adi_sdhci.c \
				drivers/adi/phy/adi_sdhci_phy.c \
				drivers/adi/spi/adi_raw_spi.c \
				drivers/adi/spi/adi_qspi.c \
				drivers/arm/sp805/sp805.c \
				drivers/gpio/gpio.c \
				plat/adi/adrv/adrv906x/adrv906x_bl1_setup.c \
				plat/adi/adrv/adrv906x/adrv906x_ddr.c \
				plat/adi/adrv/adrv906x/adrv906x_gpint.c \
				plat/adi/adrv/adrv906x/adrv906x_peripheral_clk_rst.c \
				plat/adi/adrv/adrv906x/adrv906x_pinctrl_svc.c \
				plat/adi/adrv/adrv906x/adrv906x_wdt.c

BL2_SOURCES		+=	${DDR_SOURCES} \
				drivers/adi/adrv906x/clk/clk.c \
				drivers/adi/adrv906x/clk/mcs.c \
				drivers/adi/adrv906x/clk/sysref_simulator.c \
				drivers/adi/adrv906x/clk/zl30732.c \
				drivers/adi/adrv906x/gpio/adrv906x_gpio.c \
				drivers/adi/adrv906x/legacy/ldo/ldo.c \
				drivers/adi/adrv906x/legacy/mbias/mbias.c \
				drivers/adi/adrv906x/legacy/pll/pll.c \
				drivers/adi/adrv906x/legacy/temperature/temperature.c \
				drivers/adi/adrv906x/legacy/utils/utils.c \
				drivers/adi/c2cc/adi_c2cc.c \
				drivers/adi/i2c/adi_twi_i2c.c \
				drivers/adi/mmc/adi_sdhci.c \
				drivers/adi/phy/adi_sdhci_phy.c \
				drivers/adi/spi/adi_raw_spi.c \
				drivers/adi/spi/adi_qspi.c \
				drivers/arm/sp805/sp805.c \
				drivers/gpio/gpio.c \
				plat/adi/adrv/adrv906x/aarch64/adrv906x_secondary_image.S \
				plat/adi/adrv/adrv906x/adrv906x_ahb.c \
				plat/adi/adrv/adrv906x/adrv906x_bl2_setup.c \
				plat/adi/adrv/adrv906x/adrv906x_ddr.c \
				plat/adi/adrv/adrv906x/adrv906x_dual.c \
				plat/adi/adrv/adrv906x/adrv906x_gpint.c \
				plat/adi/adrv/adrv906x/adrv906x_peripheral_clk_rst.c \
				plat/adi/adrv/adrv906x/adrv906x_pinctrl_svc.c \
				plat/adi/adrv/adrv906x/adrv906x_security.c \
				plat/adi/adrv/adrv906x/adrv906x_wdt.c
ifeq (${DEBUG},1)
BL2_SOURCES		+=	plat/adi/adrv/adrv906x/adrv906x_cli.c
endif

BL31_SOURCES	+=	drivers/adi/adrv906x/clk/clk.c \
				drivers/adi/adrv906x/clk/mcs.c \
				drivers/adi/adrv906x/ddr/ddr_edac.c \
				drivers/adi/adrv906x/gpio/adrv906x_gpio.c \
				drivers/adi/adrv906x/transmuter/transmuter.c \
				drivers/adi/adrv906x/transmuter/transmuter_def.c \
				drivers/arm/sp805/sp805.c \
				drivers/gpio/gpio.c \
				plat/adi/adrv/adrv906x/adrv906x_bl31_setup.c \
				plat/adi/adrv/adrv906x/adrv906x_ddr.c \
				plat/adi/adrv/adrv906x/adrv906x_fixup_hw_config.c \
				plat/adi/adrv/adrv906x/adrv906x_gpint.c \
				plat/adi/adrv/adrv906x/adrv906x_el3_int_handlers.c \
				plat/adi/adrv/adrv906x/adrv906x_pintmux.c \
				plat/adi/adrv/adrv906x/adrv906x_pinctrl_svc.c \
				plat/adi/adrv/adrv906x/adrv906x_sdei.c \
				plat/adi/adrv/adrv906x/adrv906x_sip_svc.c \
				plat/adi/adrv/adrv906x/adrv906x_sram.c \
				plat/adi/adrv/adrv906x/adrv906x_topology.c \
				plat/adi/adrv/adrv906x/adrv906x_wdt.c

# If floating point is needed disable the "use general reg only" flag
ifeq (${ENABLE_FP}, 1)
TF_CFLAGS_aarch64 := $(subst -mgeneral-regs-only,,${TF_CFLAGS_aarch64})
endif

# Add argument for secondary image binary
$(eval $(call add_defines, SECONDARY_IMAGE_BIN))
