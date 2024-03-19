/*
 * Copyright (c) 2023, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.h>
#include <drivers/adi/adrv906x/sysref_simulator.h>
#include <drivers/adi/adrv906x/zl30732.h>
#include <drivers/delay_timer.h>
#include <drivers/gpio.h>
#include <drivers/spi_nor.h>
#include <lib/utils.h>

#include <adrv906x_board.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_pinctrl.h>
#include <platform_def.h>
#include <plat_console.h>
#include <plat_err.h>
#include <plat_pinctrl.h>

#define QSPI_FLASH_RECOVERY_TIME_US 2000U
#define QSPI_FLASH_RESET_TIME_US 2000U
#define BUFFER_SIZE     SZ_4K

/* ZL30732 Sysref */
#define ZL30732_SPI_BASE_ADDRESS        SPI_MASTER0_BASE
#define ZL30732_SPI_CS                  1

/* Buffer to store data before erase operation */
uint8_t erase_buffer[BUFFER_SIZE];

int plat_get_nor_data(struct nor_device *device)
{
	device->size = SZ_128M;

	zeromem(&device->read_op, sizeof(struct spi_mem_op));
	/* Read Operation */
	device->read_op.cmd.opcode = SPI_NOR_OP_READ_1_4_4;
	device->read_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->read_op.addr.nbytes = 3U;
	device->read_op.addr.buswidth = SPI_MEM_BUSWIDTH_4_LINE;
	device->read_op.dummy.nbytes = 5U;
	device->read_op.dummy.buswidth = SPI_MEM_BUSWIDTH_4_LINE;
	device->read_op.data.buswidth = SPI_MEM_BUSWIDTH_4_LINE;
	device->read_op.data.dir = SPI_MEM_DATA_IN;

	zeromem(&device->write_op, sizeof(struct spi_mem_op));
	/* Write Operation */
	device->write_op.cmd.opcode = SPI_NOR_OP_WRITE_1_4_4;
	device->write_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->write_op.addr.nbytes = 3U;
	device->write_op.addr.buswidth = SPI_MEM_BUSWIDTH_4_LINE;
	device->write_op.dummy.nbytes = 0U;
	device->write_op.dummy.buswidth = 0U;
	device->write_op.data.buswidth = SPI_MEM_BUSWIDTH_4_LINE;
	device->write_op.data.dir = SPI_MEM_DATA_OUT;

	zeromem(&device->erase_op, sizeof(struct spi_mem_op));
	/* Erase Operation */
	device->erase_op.cmd.opcode = SPI_NOR_OP_ERASE_4SS;
	device->erase_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->erase_op.addr.nbytes = 3U;
	device->erase_op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->erase_op.dummy.nbytes = 0U;
	device->erase_op.dummy.buswidth = 0U;
	device->erase_op.data.buswidth = 0U;
	device->erase_op.data.dir = SPI_MEM_DATA_OUT;
	device->erase_op.data.buf = (void *)erase_buffer;

	device->erase_size = BUFFER_SIZE;
	device->program_size = SZ_256;

	return 0;
}

void plat_do_spi_nor_reset(void)
{
	/* Setup reset pin */
	gpio_set_direction(QSPI_FLASH_RESETN_PIN, GPIO_DIR_OUT);

	/* Assert qspi pin */
	gpio_set_value(QSPI_FLASH_RESETN_PIN, GPIO_LEVEL_LOW);

	/* Delay */
	udelay(QSPI_FLASH_RESET_TIME_US);

	/* De-assert qspi pin */
	gpio_set_value(QSPI_FLASH_RESETN_PIN, GPIO_LEVEL_HIGH);

	/* Recovery delay */
	udelay(QSPI_FLASH_RECOVERY_TIME_US);
}

void init_sysref(void)
{
	extern const plat_pinctrl_settings spimaster0_pin_grp[];
	extern const size_t spimaster0_pin_grp_members;
	static bool init_done = false;

	if (!init_done) {
		/* Init SPI MASTER0 to control external sysref */
		plat_secure_pinctrl_set_group(spimaster0_pin_grp, spimaster0_pin_grp_members, true, PINCTRL_BASE);

		/* Init zl30732 chip */
		if (!adi_zl30732_init(ZL30732_SPI_BASE_ADDRESS, clk_get_freq(CLK_CTL, CLK_ID_SYSCLK))) {
			ERROR("Cannot init ZL30732 SPI.\n");
			plat_error_handler(-ENXIO);
		}
		init_done = true;
	}
}

bool plat_sysref_enable(void)
{
	bool dual_tile = plat_get_dual_tile_enabled();

	if (plat_is_sysc()) return true;

	if (plat_is_protium() || plat_is_palladium()) {
		simulated_sysref_enable(DIG_CORE_BASE);
		if (dual_tile)
			simulated_sysref_enable(SEC_DIG_CORE_BASE);
		return true;
	} else {
		init_sysref();
		return adi_zl30732_sysref_enable(ZL30732_SPI_BASE_ADDRESS, ZL30732_SPI_CS, dual_tile);
	}
}

void plat_sysref_disable(void)
{
	bool dual_tile = plat_get_dual_tile_enabled();

	if (plat_is_sysc()) return;

	if (plat_is_protium() || plat_is_palladium()) {
		simulated_sysref_disable(DIG_CORE_BASE);
		if (dual_tile)
			simulated_sysref_disable(SEC_DIG_CORE_BASE);
	} else {
		init_sysref();
		adi_zl30732_sysref_disable(ZL30732_SPI_BASE_ADDRESS, ZL30732_SPI_CS, dual_tile);
	}
}

bool plat_clkdev_init(void)
{
	/* Currently this is unsupported (and is unrecoverable)
	 * Enable the console, print a message, and halt the boot.
	 */
	plat_console_boot_init();
	ERROR("Booting from ROSC is not supported\n");
	plat_halt_handler();

	return true;
}

void plat_board_bl1_setup(void)
{
	extern const plat_pinctrl_settings debug_gpios_pin_grp[];
	extern const size_t debug_gpios_pin_grp_members;

	/* Initialize Debug GPIOs */
	plat_secure_pinctrl_set_group(debug_gpios_pin_grp, debug_gpios_pin_grp_members, true, PINCTRL_BASE);

	/* Initialize Debug Crossbar */
	adi_adrv906x_debug_xbar_set_default_map(DEBUG_XBAR_SOURCE_CONTROL_BASE, BRINGUP_MAPPING);
}

void plat_board_bl2_setup(void)
{
	/* Nothing to do */
}

void plat_board_bl31_setup(void)
{
	/* Nothing to do */
}
