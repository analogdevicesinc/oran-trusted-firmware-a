/*
 * Copyright (c) 2023, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arm_acle.h>
#include <common/debug.h>
#include <common/tf_crc32.h>
#include <libfdt.h>

#include <plat_io_storage.h>
#include <platform_def.h>

#define PLAT_BOOTCFG_VERSION 0x00000001
#define DT_BLOCK_SIZE   4

typedef struct __packed {
	uint32_t crc32;
	uint32_t version;
	uint32_t dtb_size;
} plat_bootcfg_t;

static io_block_spec_t cfg_spec = {
	.offset = 0,
	.length = 0x200000,
};

/* Retrieves the active boot slot from the bootfg patition on disk */
int plat_bootcfg_init(void)
{
	const char *cfg_partition_id = BOOTCFG_PARTITION_NAME;
	int result;
	int err = -1;
	uint32_t crc = ~0u;
	uint32_t local_size;
	const uint32_t *local_buf;
	uintptr_t handle;
	uintptr_t boot_dev_handle = plat_get_boot_handle();
	plat_bootcfg_t bootcfg;
	size_t length_read;
	void *bootcfg_dtb;
	io_block_spec_t *spec;

	/* Check that boot_dev_handle was set up to point to the handle */
	if (boot_dev_handle == (uintptr_t)NULL)
		return -1;

	result = plat_get_partition_spec(cfg_partition_id, &cfg_spec);
	if (result != 0) {
		ERROR("Unable to find the %s partition\n", cfg_partition_id);
		return -1;
	} else {
		spec = &cfg_spec;
	}

	/* Read the bootcfg struct from disk */
	result = io_dev_init(boot_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(boot_dev_handle, (uintptr_t)spec, &handle);
		if (result == 0) {
			result = io_seek(handle, IO_SEEK_SET, 0);
			if (result == 0) {
				result = io_read(handle, (uintptr_t)&bootcfg, sizeof(bootcfg), &length_read);
				if (result == 0) {
					if (bootcfg.dtb_size != 0 && bootcfg.dtb_size <= BOOTCFG_MAX_SIZE) {
						result = io_seek(handle, IO_SEEK_SET, sizeof(bootcfg));
						if (result == 0)
							/* Bootcfg device tree copied to BOOTCFG_BASE */
							result = io_read(handle, BOOTCFG_BASE, (size_t)bootcfg.dtb_size, &length_read);
					}
				}
			}
			io_close(handle);
		}
	}
	if (result != 0) {
		ERROR("Unable to read bootcfg partition\n");
		return -1;
	}
	if (bootcfg.dtb_size == 0 || bootcfg.dtb_size == 0xFFFFFFFF) {
		INFO("Bootcfg partition is empty\n");
		return -1;
	}
	if (bootcfg.dtb_size > BOOTCFG_MAX_SIZE) {
		ERROR("Bootcfg size is too big\n");
		return -1;
	}
	if (bootcfg.dtb_size % DT_BLOCK_SIZE != 0) {
		ERROR("Invalid dtb size\n");
		return -1;
	}
	if (length_read != (bootcfg.dtb_size)) {
		ERROR("Incorrect bootcfg dtb size read\n");
		return -1;
	}

	bootcfg_dtb = (void *)(uintptr_t)BOOTCFG_BASE;

	/* Calculate crc32 */
	crc = __crc32w(crc, bootcfg.version);
	crc = __crc32w(crc, bootcfg.dtb_size);

	local_buf = (uint32_t *)bootcfg_dtb;
	local_size = bootcfg.dtb_size / DT_BLOCK_SIZE;
	while (local_size != 0UL) {
		crc = __crc32w(crc, *local_buf);
		local_buf++;
		local_size--;
	}

	crc = ~crc;

	/* Verify bootcfg crc */
	if (bootcfg.crc32 != crc) {
		ERROR("Corrupt bootcfg\n");
		return -1;
	}

	/* Verify bootcfg version */
	if (bootcfg.version != PLAT_BOOTCFG_VERSION) {
		ERROR("Bootcfg version does not match\n");
		return -1;
	}

	/* Verify bootcfg device tree format is valid */
	err = fdt_check_header(bootcfg_dtb);
	if (err < 0) {
		ERROR("Bootcfg device tree format is invalid\n");
		return -1;
	}

	return 0;
}
