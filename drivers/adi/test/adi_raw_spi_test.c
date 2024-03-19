/************************** Test INSTRUCTIONS ***************************/
/*
 * This is a test framework to verify the functionality of the raw SPI driver.
 *
 * By default, this framework is not included in the TF-A build.
 * Do the following to run this test when setting TEST_FRAMEWORK=1:
 * 1. Add the following in drivers/adi/test/test_framework.c
 *
 *   --- a/drivers/adi/test/test_framework.c
 *   +++ b/drivers/adi/test/test_framework.c
 *   @@ -10,5 +10,7 @@
 *    #include <stdio.h>
 *    #include <lib/utils.h>
 *
 *   +extern int adi_raw_spi_test(void);
 *   +
 *    int test_main(void)
 *    {
 *   @@ -19,5 +21,7 @@ int test_main(void)
 *
 *           printf("Inside test main\n");
 *
 *   +       adi_raw_spi_test();
 *   +
 *           return 0;
 *    }
 *
 * 2. Incude this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 *
 *   --- a/plat/adi/adrv/adrv906x/plat_adrv906x.mk
 *   +++ b/plat/adi/adrv/adrv906x/plat_adrv906x.mk
 *   @@ -33,6 +33,7 @@ BL1_SOURCES           +=      drivers/adi/mmc/adi_sdhci.c \
 *                                   drivers/adi/adrv906x/clk/clk.c \
 *                                   drivers/adi/adrv906x/gpio/adrv906x_gpio.c \
 *                                   drivers/adi/spi/adi_raw_spi.c \
 *   +                               drivers/adi/test/adi_raw_spi_test.c \
 *                                   drivers/adi/spi/adi_qspi.c \
 *                                   drivers/arm/sp805/sp805.c \
 *                                   drivers/gpio/gpio.c \
 *
 ************************************************************************/

#include <stdio.h>
#include <stdbool.h>

#include <lib/utils.h>
#include <lib/mmio.h>
#include <drivers/adi/adi_qspi.h>
#include <drivers/adi/adi_raw_spi.h>
#include <drivers/adi/adi_spu.h>
#include <drivers/adi/adrv906x/clk.h>
#include <drivers/adi/adrv906x/adrv906x_gpio.h>
#include <drivers/spi_nor.h>

#include <adrv906x_def.h>
#include <adrv906x_device_profile.h>
#include <adrv906x_pinctrl.h>
#include <adrv906x_pinmux_source_def.h>
#include <adrv906x_spu_def.h>
#include <plat_pinctrl.h>
#include <platform_def.h>
#include "../spi/adi_qspi_regs.h"       // SPI regs

#define FLASH_READ_JEDEC_CMD                  0x9E
#define FLASH_READ_STATUS_REG_CMD             0x05
#define FLASH_WRITE_STATUS_REG_CMD            0x01
#define FLASH_STATUS_REG_WRITE_ENABLE_BIT     0x80

#define N25Q00A_JEDEC                         0x20BB21          // System C card model
#define MT25QU02G_JEDEC                       0x20BB22          // Protium card model
/**
 * adi_raw_spi_test
 * - The only spi peripheral we have on SystemC/Protium is the QSPI memory.
 * - To test the raw SPI R/W we have to hack the SPI transactions so we can use some
 *   serial NOR Flash specific commands, instead of using the LSB bit as R/W indication.
 */
int adi_raw_spi_test(void)
{
	uint8_t address;
	uint8_t buffer_rx[4] = { 0 };

	printf("\n\n");
	printf("RAW SPI driver test\n");
	printf("-------------------\n");

	/**
	 * Init driver
	 */
	unsigned int clock_freq = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
	if (!adi_raw_spi_init(QSPI_0_BASE, 0, clock_freq, QSPI_CLK_FREQ_HZ)) {
		printf("ERROR adi_raw_spi_init\n");
		return 1;
	}

	/**
	 * Read test: obtain expected JEDEC
	 */
	printf("Read QSPI JEDEC...      ");
	address = FLASH_READ_JEDEC_CMD;
	if (!adi_raw_spi_read(QSPI_0_BASE, QSPI_FLASH_CHIP_SELECT, address, buffer_rx, 3)) {
		printf("ERROR adi_raw_spi_read\n");
		return 1;
	}
	uint32_t read_jedec = (buffer_rx[0] << 16) + (buffer_rx[1] << 8) + (buffer_rx[2] << 0);
	uint32_t expected_jedec;

	if (plat_is_sysc()) expected_jedec = N25Q00A_JEDEC;
	else expected_jedec = MT25QU02G_JEDEC;
	if (read_jedec == expected_jedec) printf("OK (0x%x)\n", expected_jedec);
	else printf("ERROR: read 0x%x, expected 0x%x\n", read_jedec, expected_jedec);

	/**
	 * Write test: modify a register in the QSPI memory
	 * Specifically, the test modifies the bit 7 of the STATUS register
	 */
	printf("Modify QSPI register... ");
	uint8_t reg, expected_status_reg;
	address = FLASH_READ_STATUS_REG_CMD;
	if (!adi_raw_spi_read(QSPI_0_BASE, QSPI_FLASH_CHIP_SELECT, address, &reg, 1)) {
		printf("ERROR reading initial status register\n");
		return 1;
	}
	/* Toggle the write enable bit */
	if (reg & FLASH_STATUS_REG_WRITE_ENABLE_BIT)
		reg = reg & ~FLASH_STATUS_REG_WRITE_ENABLE_BIT;
	else
		reg = reg | FLASH_STATUS_REG_WRITE_ENABLE_BIT;
	expected_status_reg = reg;
	/* Write status */
	address = FLASH_WRITE_STATUS_REG_CMD;
	if (!adi_raw_spi_write(QSPI_0_BASE, QSPI_FLASH_CHIP_SELECT, address, &reg, 1)) {
		printf("ERROR writting initial status register\n");
		return 1;
	}
	/* Read status */
	address = FLASH_READ_STATUS_REG_CMD;
	if (!adi_raw_spi_read(QSPI_0_BASE, QSPI_FLASH_CHIP_SELECT, address, &reg, 1)) {
		printf("ERROR reading status register\n");
		return 1;
	}
	/* Check with expected value */
	if ((reg & FLASH_STATUS_REG_WRITE_ENABLE_BIT) ==
	    (expected_status_reg & FLASH_STATUS_REG_WRITE_ENABLE_BIT))
		printf("OK\n");
	else
		printf("ERROR (write: %x, read-back: %x)\n", expected_status_reg, reg);

	printf("RAW SPI test done\n");

	return 0;
}
