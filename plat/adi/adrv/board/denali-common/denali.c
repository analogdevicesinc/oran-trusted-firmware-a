/*
 * Copyright (c) 2023, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <common/debug.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/ddr/ddr.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar.h>
#include <drivers/adi/adrv906x/debug_xbar/debug_xbar_default_maps.h>
#include <drivers/adi/adrv906x/sysref_simulator.h>
#include <drivers/adi/adrv906x/zl30732.h>
#include <drivers/delay_timer.h>
#include <drivers/gpio.h>
#include <drivers/spi_nor.h>
#include <lib/utils.h>
#include <lib/mmio.h>

#ifdef  TFA_DEBUG
#define TEST_SCRATCHPAD_ADDR 0x18290200
#endif

#include <adrv906x_board.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_pinctrl.h>
#include <platform_def.h>
#include <plat_console.h>
#include <plat_err.h>
#include <plat_pinctrl.h>
#include <plat_wdt.h>

#define QSPI_FLASH_RECOVERY_TIME_US 2000U
#define QSPI_FLASH_RESET_TIME_US 2000U
#define BUFFER_SIZE     SZ_4K

/* ZL30732 Sysref */
#define ZL30732_SPI_BASE_ADDRESS        SPI_MASTER0_BASE
#define ZL30732_SPI_CS                  0

/*
 * DEBUG_GPIOS PINCTRL GROUP
 */
const plat_pinctrl_settings debug_gpios_pin_grp[] = {
	/*pin#              SRCMUX                DS                ST     P_EN   PU     extendedOptions*/
	{ GPIO_DEBUG_0_PIN, GPIO_DEBUG_0_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_1_PIN, GPIO_DEBUG_1_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_2_PIN, GPIO_DEBUG_2_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_3_PIN, GPIO_DEBUG_3_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_4_PIN, GPIO_DEBUG_4_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_5_PIN, GPIO_DEBUG_5_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_6_PIN, GPIO_DEBUG_6_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U },
	{ GPIO_DEBUG_7_PIN, GPIO_DEBUG_7_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U }
};
const size_t debug_gpios_pin_grp_members = sizeof(debug_gpios_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * SPI MASTER0 PINCTRL GROUP, used with external sysref ZL30732 on CS0
 * CS1 is configured here as well because it requires an internal pullup.
 */
const plat_pinctrl_settings spimaster0_pin_grp[] = {
	/*pin#                        SRCMUX                      DS                ST     P_EN   PU     extendedOptions*/
	{ SPI_MASTER0_MISO_DIO_PIN,   0 /*Dedicated IO*/,	  CMOS_PAD_DS_0100, true,  false, false, 0U },
	{ SPI_MASTER0_MOSI_DIO_PIN,   0 /*Dedicated IO*/,	  CMOS_PAD_DS_0100, false, false, false, 0U },
	{ SPI_MASTER0_CLK_DIO_PIN,    0 /*Dedicated IO*/,	  CMOS_PAD_DS_0100, false, false, false, 0U },
	{ SPI_MASTER0_SELB_0_DIO_PIN, 0 /*Dedicated IO*/,	  CMOS_PAD_DS_0100, false, true,  true,	 0U },
	{ SPI_MASTER0_SELB_1_PIN,     SPI_MASTER0_SELB_1_MUX_SEL, CMOS_PAD_DS_0100, false, true,  true,	 0U }
};
const size_t spimaster0_pin_grp_members = sizeof(spimaster0_pin_grp) / sizeof(plat_pinctrl_settings);

/*
 * POWER PINCTRL GROUP, A55_GPIO_S_102_PIN used for ADM1266 reset
 */
const plat_pinctrl_settings power_pin_grp[] = {
	/*pin#                SRCMUX                  DS                ST     P_EN   PU     extendedOptions*/
	{ A55_GPIO_S_102_PIN, A55_GPIO_S_102_MUX_SEL, CMOS_PAD_DS_0100, false, false, false, 0U }
};
const size_t power_pin_grp_members = sizeof(power_pin_grp) / sizeof(plat_pinctrl_settings);

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

static void init_sysref(void)
{
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

/* Example to configure MCS kickoff on dual chip without C2C */
#define MCS_KICKOFF_OUTPUT_PIN  A55_GPIO_S_55_PIN
#define MCS_KICKOFF_OUTPUT_MUX  A55_GPIO_S_55_MUX_SEL
#define MCS_KICKOFF_INPUT_PIN   A55_GPIO_S_47_PIN
#define MCS_KICKOFF_INPUT_MUX   A55_GPIO_S_47_MUX_SEL

#define MCS_KICKOFF_OFF         GPIO_LEVEL_LOW
#define MCS_KICKOFF_ON          GPIO_LEVEL_HIGH
#define MCS_KICKOFF_TIMEOUT_US  (1000 * 1000 * 10)

static void plat_configure_mcs_kickoff_input(void)
{
	/*                                              pin#                   SRCMUX                 DS                ST    P_EN  PU     extendedOptions */
	plat_pinctrl_settings mcs_kickoff_input_pin = { MCS_KICKOFF_INPUT_PIN, MCS_KICKOFF_INPUT_MUX, CMOS_PAD_DS_0100, true, true, false, 0 };

	plat_secure_pinctrl_set(mcs_kickoff_input_pin, true, PINCTRL_BASE);
	gpio_set_direction(MCS_KICKOFF_INPUT_PIN, GPIO_DIR_IN);
}

static void plat_configure_mcs_kickoff_output(void)
{
	/*                                               pin#                    SRCMUX                  DS                ST    P_EN  PU     extendedOptions */
	plat_pinctrl_settings mcs_kickoff_output_pin = { MCS_KICKOFF_OUTPUT_PIN, MCS_KICKOFF_OUTPUT_MUX, CMOS_PAD_DS_0100, true, true, false, 0 };

	plat_secure_pinctrl_set(mcs_kickoff_output_pin, true, PINCTRL_BASE);
	gpio_set_direction(MCS_KICKOFF_OUTPUT_PIN, GPIO_DIR_OUT);
}

static void plat_assert_mcs_kickoff_output(void)
{
	gpio_set_value(MCS_KICKOFF_OUTPUT_PIN, MCS_KICKOFF_ON);
}

static void plat_deassert_mcs_kickoff_output(void)
{
	gpio_set_value(MCS_KICKOFF_OUTPUT_PIN, MCS_KICKOFF_OFF);
}

static int plat_get_mcs_kickoff_input(void)
{
	return gpio_get_value(MCS_KICKOFF_INPUT_PIN);
}

static bool plat_mcs_kickoff(void)
{
	if (!plat_get_dual_tile_no_c2c_enabled()) return false;

	if (plat_get_dual_tile_no_c2c_primary()) {
		/* Primary */
		plat_configure_mcs_kickoff_input();
		/* Wait for MCS kickoff input */
		uint64_t timeout = timeout_init_us(MCS_KICKOFF_TIMEOUT_US);
		while (plat_get_mcs_kickoff_input() == MCS_KICKOFF_OFF)
			if (timeout_elapsed(timeout))
				return false;
	} else {
		/* Secondary */
		plat_configure_mcs_kickoff_output();
		plat_assert_mcs_kickoff_output();
	}

	/* MCS kickoff completed */
	return true;
}

bool plat_sysref_enable(void)
{
	bool dual_tile = plat_get_dual_tile_enabled();

	/* MCS kickoff: function to sync multiple adrv906x without C2C */
	if (plat_get_dual_tile_no_c2c_enabled()) {
		/* Both tiles run the mcs kickoff */
		if (!plat_mcs_kickoff())
			return false;
		/* Secondary shouldn´t do anything related to sysref */
		if (!plat_get_dual_tile_no_c2c_primary()) return true;

		/* Primary shall enable both sysref outputs, same as in the dual_tile wich c2c case */
		dual_tile = true;
	}

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

bool plat_sysref_disable(bool mcs_completed)
{
	bool sysref_disable_ok;
	bool dual_tile = plat_get_dual_tile_enabled();

	bool secondary_deassert_failed = false;

	if (plat_get_dual_tile_no_c2c_enabled()) {
		if (plat_get_dual_tile_no_c2c_primary()) {
			/* Primary */
			if (mcs_completed) {
				/* Wait for secondary gpio deassert */
				uint64_t timeout = timeout_init_us(MCS_KICKOFF_TIMEOUT_US);
				while (plat_get_mcs_kickoff_input() == MCS_KICKOFF_ON)
					if (timeout_elapsed(timeout)) {
						/* Secondary has not deasserted the kickoff gpio */
						secondary_deassert_failed = true;
						/* Let the Primary continue to disable the sysref outputs */
						break;
					}
			}
		} else {
			/* Secondary */
			if (mcs_completed) {
				/* MCS succeded: deassert kickoff output */
				plat_deassert_mcs_kickoff_output();
				/* Secondary shouldn´t do anything related to sysref */
				return true;
			} else {
				/* MCS failed: leave kickoff output asserted, disable WDT and halt the boot */
				plat_secure_wdt_stop();
				while (1);
			}
		}
		/* Primary shall disable both sysref outputs, same as in the dual_tile wich c2c case */
		dual_tile = true;
	}

	if (plat_is_sysc()) return true;

	if (plat_is_protium() || plat_is_palladium()) {
		simulated_sysref_disable(DIG_CORE_BASE);
		if (dual_tile)
			simulated_sysref_disable(SEC_DIG_CORE_BASE);
		return true;
	} else {
		init_sysref();
		sysref_disable_ok = adi_zl30732_sysref_disable(ZL30732_SPI_BASE_ADDRESS, ZL30732_SPI_CS, dual_tile);

		/* Return failure if Secondary has not deasserted the kickoff gpio */
		if (secondary_deassert_failed) return false;

		return sysref_disable_ok;
	}
}

bool plat_clkdev_init(void)
{
	plat_console_boot_init();
#ifdef TFA_DEBUG
	mmio_write_8(TEST_SCRATCHPAD_ADDR, 0xa5);
#endif
	return true;
}

void ddr_board_custom_pre_training(uintptr_t base_addr_phy)
{
	INFO("Using default config values for DDR PHY training.\n");

	/*  You can utilize the base address passed in to the pre_training function to write directly
	 *  to DDR registers. The defines for all of the DDR register offsets, masks, etc are
	 *  in drivers/adi/adrv906x/ddr/ddr_regmap.h
	 *
	 *	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy), 0x18);
	 *	mmio_write_32((DDRPHYA_DBYTE0_P0_DBYTE0_P0_TXODTDRVSTREN_B1_P0 + base_addr_phy), 0x18);
	 *	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXODTDRVSTREN_B0_P0 + base_addr_phy), 0x18);
	 *	mmio_write_32((DDRPHYA_DBYTE1_P0_DBYTE1_P0_TXODTDRVSTREN_B1_P0 + base_addr_phy), 0x18);
	 */
}

void plat_board_bl1_setup(void)
{
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
	gpio_set_direction(A55_GPIO_S_102_PIN, GPIO_DIR_OUT);
	gpio_set_value(A55_GPIO_S_102_PIN, GPIO_LEVEL_LOW);

	/* Init POWER controller to control external power sequencer chip ADM1266 */
	plat_secure_pinctrl_set_group(power_pin_grp, power_pin_grp_members, true, PINCTRL_BASE);
}

static void __dead2 plat_board_psci_system_off(void)
{
	NOTICE("System Off \n");
	console_flush();

	/* Sequence to trigger ADM1266 power off:
	 * GPIO_102 high
	 * Delay 50ms
	 * GPIO_102 low
	 */
	plat_secure_wdt_ping(); /* Make sure WDT doesn't expire while delaying below */
	gpio_set_value(A55_GPIO_S_102_PIN, GPIO_LEVEL_HIGH);
	/* Must hold pin high for certain amount of time and then set low to tell sequencer to shutdown instead of reset.
	 *  Exact delay time obtained from Apps */
	mdelay(55);
	gpio_set_value(A55_GPIO_S_102_PIN, GPIO_LEVEL_LOW);
	mdelay(1000);
	ERROR("System Off failed");
	panic();
}

void __dead2 plat_board_system_reset(void)
{
	NOTICE("System Reset \n");
	console_flush();

	/* To trigger ADM1266 reset, set GPIO_102 high until reset occurs (or we time out) */
	plat_secure_wdt_ping(); /* Make sure WDT doesn't expire while delaying below */
	gpio_set_value(A55_GPIO_S_102_PIN, GPIO_LEVEL_HIGH);
	mdelay(1050);
	ERROR("System Reset failed\n");
	panic();
}

plat_psci_ops_t *plat_board_psci_override_pm_ops(plat_psci_ops_t *ops)
{
	ops->system_off = plat_board_psci_system_off;
	ops->system_reset = plat_board_system_reset;
	return ops;
}

#if 0
int plat_board_get_mac(uint32_t index, uint8_t **mac)
{
	/* Optional hook to implement custom logic to obtain the Ethernet MAC information */

	/* Index convention:
	 * 0: Primary 1G Ethernet
	 * 1: Primary first 10/25G Ethernet
	 * 2: Primary second 10/25G Ethernet
	 * 3: Secondary 1G Ethernet
	 * 4: Secondary first 10/25G Ethernet
	 * 5: Secondary second 10/25G Ethernet
	 *
	 * 'mac' argument in the caller side is just a pointer. Keep the MAC
	 * information as a global 6-byte array in your file (ie:
	 * char your_mac_array_var[6];) and just return a pointer to it:
	 *   *mac = your_mac_array_var;
	 *
	 * Either retuning an error or a zero MAC will be considered as MAC
	 * info not available through the custom method.
	 */

	return 0;
}
#endif
