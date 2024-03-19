#include <drivers/adi/adi_te_interface.h>
#include <lib/mmio.h>
#include <lib/utils.h>
#include <stdint.h>
#include <stdio.h>

#include <platform_def.h>

/************************** Test INSTRUCTIONS ***************************/
/* By default, this framework is not included in TF-A build. Do the following
 * before attempting to test the driver:
 * 1. Add the following in drivers/adi/test/test_framework.c
 *    -> extern int adi_te_mailbox_test(void); or define the prototype of
 *      in a test_framework.h header and include the header in test_framework.c
 *    -> Call adi_te_mailbox_test() inside test_main()
 * 2. Incude this file in plat/adi/adrv/adrv906x/plat_adrv906x.mk
 ************************************************************************/
uint32_t appSecVer = 0;
void print_data(uint8_t *buf, uint8_t len)
{
	for (uint8_t i = 0; i < len; i++)
		printf("%x ", buf[i]);
	printf("\n");
}

int adi_te_mailbox_test(void)
{
	uint8_t output_buf[16] = { 0 };
	uint32_t output_buf_len = sizeof(output_buf);
	int status;

	printf("Initializing te mailbox\n");
	adi_enclave_mailbox_init(TE_MAILBOX_BASE);

	/* Mailbox #1 */
	printf("Enclave Version: ");
	while (adi_enclave_get_enclave_version(TE_MAILBOX_BASE, output_buf, &output_buf_len) != 0);
	print_data(output_buf, 16);

	/* Mailbox #2 */
	memset(output_buf, 0, 16);
	printf("Enclave Serial Version: ");
	status = adi_enclave_get_serial_number(TE_MAILBOX_BASE, output_buf, &output_buf_len);
	if (status == 0)
		print_data(output_buf, 12);
	else
		printf("Status get serial version: %d\n", status);

	/* Mailbox #3 */
	memset(output_buf, 0, 16);
	printf("Enclave API Version: ");
	status = adi_enclave_get_api_version(TE_MAILBOX_BASE, output_buf, &output_buf_len);
	if (status == 0)
		print_data(output_buf, 12);
	else
		printf("Status get api version: %d\n", status);

	/* Mailbox #5 */
	uint8_t challenge_buf[16] = { 0 };
	uint32_t chal_buf_len = sizeof(challenge_buf);
	/* Expected challenge_buf on Protium for this response_buf is d7 47 d1 55 58 d4 7c 55 5 96 1a 88 9c 3c ab a2
	 *
	 * If challenge_buf does not match, response_buf needs to be updated:
	 * Save the challenge_buf in a binary file, challenge.bin
	 * Navigate to dv_tools/RAM/pysef/scripts and call:
	 * 		gen_signature_kmi.py -i challenge.bin -o resp.bin -c ..\..\..\chal-resp\kmi_sign_tiny_dev_host_secure_debug_cfg.yml
	 * The signature/response should be stored in resp.bin, replace response_buf with those values
	 */
	uint8_t response_buf[64] = { 0xA8, 0x80, 0xDC, 0x7D, 0xD6, 0xF9, 0x1D, 0xD6, 0x6F, 0x7A, 0x55, 0xEC, 0x50, 0xA6, 0x18, 0xB5, 0x02, 0xB5, 0xF9, 0x64, 0x08, 0x81, 0xE5, 0x1E, 0x1F, 0xFF, 0x44, 0x9E, 0x21, 0x42, 0x39, 0x4E, 0xD7, 0x64, 0xB5, 0x9F, 0xC3, 0x16, 0x63, 0xD9, 0x4F, 0x92, 0x46, 0xED, 0x36, 0x0C, 0xBB, 0x95, 0xCB, 0x35, 0xE9, 0xC5, 0xB5, 0x93, 0xE8, 0x14, 0x00, 0x16, 0x69, 0xCD, 0xA9, 0x4F, 0xEF, 0x06 };

	printf("Enclave Get debug access before Challenge api: ");
	status = adi_enclave_priv_secure_debug_access(TE_MAILBOX_BASE, response_buf, 64);
	printf("Status of Get debug access before challenge response api : %d\n", status);

	printf("Enclave Request Challenge : ");
	status = adi_enclave_request_challenge(TE_MAILBOX_BASE, ADI_ENCLAVE_CHAL_SECURE_DEBUG_ACCESS, challenge_buf, &chal_buf_len);
	if (status == 0)
		print_data(challenge_buf, 16);
	else
		printf("Status of challenge response api : %d\n", status);

	printf("Enclave Get debug access after Challenge api: ");
	status = adi_enclave_priv_secure_debug_access(TE_MAILBOX_BASE, response_buf, 64);
	printf("Status of Get debug access after challenge response api : %d\n", status);

	/* Mailbox #6 */
	/* To test this api follow the steps:
	 * 1. Go to dv_tools/FCER folder
	 * 2. Make changes to the fcercfg_dv_v3.yml  file.
	 * 3. Run the following command python3 fcer_sign.py fcercfg_dv_v3.yml. The output will be generated in fcer.crt file.
	 * 4. Parse the file (xxd -C -g 1 fcer.crt) and paste the values here.
	 * 5. Sync output directory.*/
	uint8_t enable_features_fcer[] = { 0x46, 0x43, 0x45, 0x52, 0x5f, 0x41, 0x44, 0x49, 0x03, 0x00, 0x00, 0x00, 0x20, 0x23, 0x07, 0x11,
					   0x00, 0x00, 0x00, 0x00, 0x20, 0x33, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x27, 0x06, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x58, 0x01, 0x00, 0x00,
					   0xbd, 0xbe, 0x93, 0x23, 0xe9, 0xd6, 0x0d, 0xe0, 0xf3, 0xf5, 0x38, 0xa3, 0xbf, 0xe3, 0x1b, 0x36,
					   0x96, 0x6a, 0xd6, 0x56, 0x5e, 0x08, 0x2e, 0x4b, 0xa3, 0xf0, 0x33, 0x85, 0xa4, 0xd8, 0x9b, 0x83,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x01, 0x00, 0x08, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
					   0xff, 0xff, 0xff, 0xff, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc,
					   0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x66, 0x42, 0x4f, 0xcc, 0x62, 0x87, 0x03, 0xfa,
					   0xfe, 0xcb, 0x21, 0xe6, 0xbf, 0x21, 0x50, 0x87, 0x72, 0x11, 0x36, 0x1d, 0x53, 0x51, 0x32, 0xf2,
					   0xcb, 0xf0, 0x24, 0xa0, 0xe3, 0x90, 0xd5, 0x08, 0x9d, 0x66, 0x8e, 0x92, 0xa2, 0xd4, 0x21, 0x76,
					   0x70, 0x6d, 0xac, 0x62, 0x2c, 0x0d, 0x71, 0xde, 0xe8, 0xe4, 0xae, 0x9f, 0x61, 0x6e, 0xf9, 0x55,
					   0x31, 0xd1, 0xec, 0x90, 0xde, 0xe0, 0x24, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t enabled_features_fcer[50] = { 0 };
	uint32_t enable_buf_len = sizeof(enable_features_fcer);
	uint32_t enabled_buf_len = sizeof(enabled_features_fcer);
	printf("Enclave Enable Features: ");
	status = adi_enclave_enable_feature(TE_MAILBOX_BASE, enable_features_fcer, enable_buf_len);
	printf("Status enable features: %d\n", status);

	/* Mailbox #7 */
	printf("Enclave Enable Features: ");
	status = adi_enclave_get_enabled_features(TE_MAILBOX_BASE, enabled_features_fcer, &enabled_buf_len);
	if (status == 0)
		print_data(enabled_features_fcer, 50);
	else
		printf("Status get enabled features: %d\n", status);

	/* Mailbox #8 */
	memset(output_buf, 0, 16);
	printf("Enclave Device Identity: ");
	status = adi_enclave_get_device_identity(TE_MAILBOX_BASE, output_buf, &output_buf_len);
	if (status == 0)
		print_data(output_buf, 12);
	else
		printf("Status get device identity: %d\n", status);

	/* Mailbox #9 */
	printf("Anti-rollback counter: ");
	status = adi_enclave_update_otp_app_anti_rollback(TE_MAILBOX_BASE, &appSecVer);
	if (status == 0)
		printf("Rollback counter = %d\n", appSecVer);
	else
		printf("Status rollback api: %d\n", status);

	printf("Anti-rollback counter: ");
	status = adi_enclave_update_otp_app_anti_rollback(TE_MAILBOX_BASE, &appSecVer);
	if (status == 0)
		printf("Rollback counter = %d\n", appSecVer);
	else
		printf("Status rollback api: %d\n", status);

	/* Mailbox #10 */
	uint8_t huk_buf[16] = { 0 };
	uint32_t huk_buf_len = sizeof(huk_buf);

	printf("Get enclave HUK: ");
	status = adi_enclave_get_huk(TE_MAILBOX_BASE, huk_buf, &huk_buf_len);
	if (status == 0)
		print_data(huk_buf, 16);
	else
		printf("Status Get HUK: %d\n", status);

	/* Mailbox #11 */
	printf("Get random bytes: ");
	uint32_t seed;
	status = adi_enclave_random_bytes(TE_MAILBOX_BASE, &seed, sizeof(seed));
	if (status == 0)
		printf("Seed: %d\n", seed);
	else
		printf("Status get random bytes: %d\n", status);

	/* Mailbox #12 */
	/* To test this create a new otp binary with the following steps:
	 * 1. Lifecycle ADI_PROV_ENC
	 * 2. Comment out HST_HUK_file in the adi_adrv906x_dv.yml file under dv_tools/OTP folder
	 * 3. Sync output directory.*/
	uint8_t static_pubkey[] = { 0xf0, 0x6d, 0xa4, 0xfe, 0x25, 0x82, 0xea, 0xf5, 0xcf, 0xaf, 0x39,
				    0x83, 0x8c, 0xfb, 0x64, 0x54, 0x26, 0xd4, 0x34, 0x9b, 0xc6, 0x2d,
				    0x67, 0x05, 0x25, 0x4c, 0x81, 0x88, 0x83, 0x46, 0x05, 0x34 };
	host_keys_t hst_keys[] =
	{
		{
			.hst_key_id = HST_IPK,
			.key_len = 16,
			.key = static_pubkey,
		},
		{
			.hst_key_id = HST_SEC_BOOT,
			.key_len = 32,
			.key = static_pubkey,
		},
	};
	printf("Size of host_keys_t = %zu\n", sizeof(host_keys_t));
	status = adi_enclave_provision_host_keys(TE_MAILBOX_BASE, (uintptr_t)hst_keys, (uint32_t)(sizeof(hst_keys) / sizeof(*hst_keys)), (uint32_t)sizeof(hst_keys));
	printf("Status of provision host keys = %d\n", status);

	status = adi_enclave_provision_prepare_finalize(TE_MAILBOX_BASE);
	printf("Status prepare finalize = %d\n", status);

	status = adi_enclave_provision_finalize(TE_MAILBOX_BASE);
	printf("Status provision finalize  = %d\n", status);

	return 0;
}
