#include <adrv906x_def.h>
#include <drivers/adi/adi_twi_i2c.h>
#include <drivers/adi/adrv906x/clk.h>
#include <platform_def.h>
#include <stdint.h>
#include <stdio.h>

#define I2C_LOW_SPEED                (40 * 1000)        /*  40 KHz */
#define I2C_HIGH_SPEED               (400 * 1000)       /* 400 KHz */

/* Devices connected to I2C0 in Denali board */
#define ADM1266_DEV_ADDR             0x41
#define EEPROM_24XX16_DEV_ADDR       0x50 /* 2KB memory */
#define MAX20860A_DEV_ADDR           0x24

/************************** Test INSTRUCTIONS ***************************/
/* This is a test framework to verify the functionality of I2C driver.
 * By default, this framework is not included in TF-A build. Do the following
 * before attempting to test the driver.
 * 1. Add the following in drivers/adi/test/test_framework.c
 *    -> extern int adi_i2c_test(void)
 *    -> Call adi_i2c_test() inside test_main()
 * 2. Include this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 * 3. Remove I2C0 from the secure-partitioning list in FW_CONFIG (denali-fw-config.dtsi)
 *
 ************************************************************************/
int adi_i2c_test()
{
	int ret;
	struct adi_i2c_handle hi2c;
	uint8_t data[300];
	uint8_t data_read[300];
	uint8_t dev_addr;
	uint16_t addr;
	uint8_t block_addr;
	uint32_t size;
	int i, j;

	/* I2C0 uses dedicated IO pins (no need for pinctrl configuration) */

	hi2c.base = I2C_0_BASE;
	hi2c.sclk = clk_get_freq(CLK_CTL, CLK_ID_SYSCLK);
	hi2c.twi_clk = I2C_LOW_SPEED;
	adi_twi_i2c_init(&hi2c);

	// Write/read 'size' bytes to/from address 'addr' */
	for (j = 0; j < 2; j++) {
		size = 200 * (j + 1);                                   /* 200 and 400 byte tests (transfer sizes larger than 254 bytes require manual stop */
		addr = 10;                                              /* Internal memory address inside the selected block */
		block_addr = addr & 0xFF;
		dev_addr = EEPROM_24XX16_DEV_ADDR | (addr >> 8);        /* Memory 256-bytes block (range 0-7) */
		for (i = 0; i < sizeof(data); i++) {
			data[i] = i;
			data_read[i] = 0;
		}

		ret = adi_twi_i2c_write(&hi2c, dev_addr, block_addr, 1, data, size);
		if (ret < 0) {
			printf("Error Writing addr=0x%x, size=%d", addr, size);
			return -1;
		}

		/* Read-back data */
		ret = adi_twi_i2c_read(&hi2c, dev_addr, block_addr, 1, data_read, size);
		if (ret < 0) {
			printf("Error Reading addr=0x%x, size=%d", addr, size);
			return -1;
		}

		/* Check read-back data */
		for (i = 0; i < size; i++) {
			if (data[i] != data_read[i]) {
				printf("Byte %d is not correct (%d != %d)\n", i, data[i], data_read[i]);
				return -1;
			}
			/* clean date for the next test */
			data_read[i] = 0;
		}

		/* Read-back again the same data, specifying the address for the
		 * first half and without specifying for the second half */
		ret = adi_twi_i2c_read(&hi2c, dev_addr, block_addr, 1, data_read, size / 2);
		if (ret < 0) {
			printf("Error Reading addr=0x%x, size=%d", addr, size / size);
			return -1;
		}

		ret = adi_twi_i2c_read(&hi2c, dev_addr, 0, 0, data_read, size / 2);
		if (ret < 0) {
			printf("Error Reading addr=0x%x, size=%d", addr + (size / 2), size / 2);
			return -1;
		}

		/* Check read-back data */
		for (i = 0; i < size; i++)
			if (data[i] != data_read[i]) {
				printf("Byte %d is not correct (%d != %d)\n", i, data[i], data_read[i]);
				return -1;
			}
	}

	return 0;
}
