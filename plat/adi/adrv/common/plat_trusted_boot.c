/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <plat/common/platform.h>
#include <platform_def.h>
#include <plat_boot.h>
#include <plat_device_profile.h>
#include <plat_setup.h>
#include <plat_trusted_boot.h>

extern unsigned char adi_rotpk_hash[];
extern unsigned char adi_rotpk_hash_end[];

static uint32_t cert_nv_ctr = 0;

int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
	/* Return embedded ROTPK hash */
	*key_ptr = adi_rotpk_hash;
	*key_len = adi_rotpk_hash_end - adi_rotpk_hash;
	*flags = ROTPK_IS_HASH;

	return 0;
}

/* Get anti-rollback enforcement counter from OTP */
int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
	return plat_get_enforcement_counter(nv_ctr);
}

/* Save nv counter field from FIP certificate, this value needs to be written to HW_CONFIG
 * This function is only implemented to gain access to the FIP certificate nv counter,
 * not to actually update the enforcement counter */
int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
	cert_nv_ctr = nv_ctr;
	return 0;
}

/* Get nv counter field, value from FIP certificate nv counter field */
unsigned int plat_get_cert_nv_ctr(void)
{
	return cert_nv_ctr;
}

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}
