#include <stdio.h>

#include <drivers/adi/adi_otp.h>
#include <lib/utils.h>

#include <adrv906x_otp.h>
#include <platform_def.h>
#include <plat_device_profile.h>

/************************** Test INSTRUCTIONS ***************************/
/* This is a test framework to verify the functionality of OTP driver which
 * supports read and write the anti-rollback counter stored in the OTP memory.
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
 *   +extern int adi_otp_test(void);
 *   +
 *    int test_main(void)
 *    {
 *   @@ -19,5 +21,7 @@ int test_main(void)
 *
 *           printf("Inside test main\n");
 *
 *   +       adi_otp_test();
 *   +
 *           return 0;
 *    }
 *
 * 2. Incude this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 *
 *   --- a/plat/adi/adrv/adrv906x/plat_adrv906x.mk
 *   +++ b/plat/adi/adrv/adrv906x/plat_adrv906x.mk
 *   @@ -23,7 +23,8 @@ PLAT_BL_COMMON_SOURCES        +=      drivers/adi/mmc/systemc_pl180_mmc.c \
 *   +                               drivers/adi/test/adi_otp_test.c \
 *                                   plat/adi/adrv/adrv906x/adrv906x_pinctrl_init.c \
 *                                   plat/adi/adrv/adrv906x/adrv906x_pinmux_source_def.c \
 *                                   plat/adi/adrv/adrv906x/adrv906x_status_reg.c \
 *                                   plat/adi/adrv/adrv906x/adrv906x_tsgen.c
 *
 ************************************************************************/
int adi_otp_test(void)
{
	uint32_t rCnt, wCnt;

	const uintptr_t mem_ctrl_base = OTP_BASE;

	printf("\nOTP test on mem_ctrl_base 0x%x...\n", (uint32_t)mem_ctrl_base);

	adrv906x_otp_init_driver();

	printf("\nTest MACs...\n");
	uint8_t MACs[6][ETH_LEN] = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 },
				     { 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC },
				     { 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22 },
				     { 0x44, 0x55, 0x66, 0x11, 0x22, 0x33 },
				     { 0xAA, 0xBB, 0xCC, 0x77, 0x88, 0x99 },
				     { 0x01, 0x12, 0x2D, 0xDE, 0xEF, 0xF0 } };
	for (int i = 0; i < 6; i++) {
		printf("Set MAC %d 0x%02x%02x%02x%02x%02x%02x... ", i, MACs[i][0], MACs[i][1], MACs[i][2], MACs[i][3], MACs[i][4], MACs[i][5]);
		if (adrv906x_otp_set_mac_addr(mem_ctrl_base, i + 1, MACs[i]) != ADI_OTP_SUCCESS) {
			printf("ERROR: Cannot set MAC %d\n", i);
			return -1;
		}
		uint8_t mac[ETH_LEN];
		if (adrv906x_otp_get_mac_addr(mem_ctrl_base, i + 1, mac) != ADI_OTP_SUCCESS) {
			printf("ERROR: Cannot get MAC %d\n", i);
			return -1;
		}
		for (int j = 0; j < ETH_LEN; j++)
			if (mac[j] != MACs[i][j]) {
				printf("ERROR: Read MAC %d: 0x%02x%02x%02x%02x%02x%02x (expected 0x%02x%02x%02x%02x%02x%02x)\n", i,
				       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
				       MACs[i][0], MACs[i][1], MACs[i][2], MACs[i][3], MACs[i][4], MACs[i][5]);
				return -1;
			}
		printf("OK\n");
	}

	printf("\nTest Anti-rollback...\n");
	if (adrv906x_otp_get_rollback_counter(mem_ctrl_base, &rCnt) != ADI_OTP_SUCCESS) {
		printf("ERROR: Cannot get anti-rollback counter\n");
		return -1;
	}
	printf("Initial anti-rollback counter: %d\n", rCnt);

	/* Increase counter test */
	printf("Set new anti-rollback counter (initial + 32)...   ");
	wCnt = rCnt + 32;
	if (adrv906x_otp_set_rollback_counter(mem_ctrl_base, wCnt) != ADI_OTP_SUCCESS) {
		printf("ERROR: Cannot set anti-rollback counter\n");
		return -1;
	}
	if (adrv906x_otp_get_rollback_counter(mem_ctrl_base, &rCnt) != ADI_OTP_SUCCESS) {
		printf("ERROR: Cannot get anti-rollback counter\n");
		return -1;
	}
	if (rCnt == wCnt)
		printf("OK: anti-rollback counter correctly set to %d\n", wCnt);
	else
		printf("ERROR: cannot set counter to %d. Current value %d\n", wCnt, rCnt);

	/* Decrease counter test */
	printf("Try to decrease the anti-rollback counter by 1... ");
	wCnt = rCnt - 1;
	if (adrv906x_otp_set_rollback_counter(mem_ctrl_base, wCnt) != ADI_OTP_SUCCESS) {
		printf("ERROR: Cannot set anti-rollback counter\n");
		return -1;
	}
	if (adrv906x_otp_get_rollback_counter(mem_ctrl_base, &rCnt) != ADI_OTP_SUCCESS) {
		printf("ERROR: Cannot get anti-rollback counter\n");
		return -1;
	}
	if (rCnt == wCnt)
		printf("ERROR: the anti-rollback counter has decreased to %d\n", rCnt);
	else
		printf("OK: the anti-rollback counter has not decreased. Current value %d\n", rCnt);

	return 0;
}
