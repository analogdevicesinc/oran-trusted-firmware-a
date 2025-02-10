#include <lib/mmio.h>
#include <stdint.h>
#include <stdio.h>
#include <lib/utils.h>
#include <drivers/adi/adi_c2cc.h>

#include <platform_def.h>

/************************** Test INSTRUCTIONS ***************************/
/* By default, this framework is not included in TF-A build. Do the following
 * before attempting to test the driver:
 * 1. Add the following in drivers/adi/test/test_framework.c
 *    -> extern int adi_c2cc_test(void); or define the prototype of
 *      in a test_framework.h header and include the header in test_framework.c
 *    -> Call adi_c2cc_test() inside test_main()
 * 2. Incude this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 ************************************************************************/
int adi_c2cc_test(void)
{
	printf("initializing c2c\n");
	adi_c2cc_init(C2CC_BASE, SEC_C2CC_BASE, C2C_MODE_NORMAL);

	/* read the secondary SPU's config register */
	uint32_t reg = mmio_read_32(0x24250000U);
	printf("value: 0x%08x\n", reg);

	return 0;
}
