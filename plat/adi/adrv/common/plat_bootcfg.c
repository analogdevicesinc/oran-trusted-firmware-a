/*
 * Copyright (c) 2024, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arm_acle.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <common/tf_crc32.h>
#include <libfdt.h>

#include <plat_io_storage.h>
#include <platform_def.h>
#include <plat_bootcfg.h>
#include <plat_err.h>

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
	uint32_t factory_reset = 0;
	int node = -1;
	size_t length = 0;
	size_t zero[sizeof(bootcfg)] = { 0 };

	/* Check that boot_dev_handle was set up to point to the handle */
	if (boot_dev_handle == (uintptr_t)NULL)
		return -1;

	result = plat_get_partition_spec(cfg_partition_id, &cfg_spec);
	if (result != 0) {
		plat_error_message("Unable to find the %s partition", cfg_partition_id);
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
		plat_error_message("Unable to read bootcfg partition");
		return -1;
	}
	if (bootcfg.dtb_size == 0 || bootcfg.dtb_size == 0xFFFFFFFF) {
		INFO("Bootcfg partition is empty\n");
		return -1;
	}
	if (bootcfg.dtb_size > BOOTCFG_MAX_SIZE) {
		plat_error_message("Bootcfg size is too big");
		return -1;
	}
	if (bootcfg.dtb_size % DT_BLOCK_SIZE != 0) {
		plat_error_message("Invalid dtb size");
		return -1;
	}
	if (length_read != (bootcfg.dtb_size)) {
		plat_error_message("Incorrect bootcfg dtb size read");
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
		plat_error_message("Corrupt bootcfg");
		return -1;
	}

	/* Verify bootcfg version */
	if (bootcfg.version != PLAT_BOOTCFG_VERSION) {
		plat_error_message("Bootcfg version does not match");
		return -1;
	}

	/* Verify bootcfg device tree format is valid */
	err = fdt_check_header(bootcfg_dtb);
	if (err < 0) {
		plat_error_message("Bootcfg device tree format is invalid");
		return -1;
	}

	/* Check for factory-reset node and get status */
	node = fdt_path_offset(bootcfg_dtb, "/factory-reset");
	if (node >= 0) {
		err = fdt_read_uint32(bootcfg_dtb, node, "status", &factory_reset);
		if (err < 0)
			plat_warn_message("Unable to read param `status` from node `/factory-reset` in bootcfg");
	}

	/* If factory reset is enabled, 1, clear the bootcfg partition */
	if (factory_reset == 1) {
		NOTICE("Factory reset requested. Clearing bootcfg partition.\n");

		result = io_open(boot_dev_handle, (uintptr_t)spec, &handle);
		if (result == 0) {
			result = io_write(handle, (uintptr_t)&zero, sizeof(bootcfg), &length);
			if (result != 0) {
				plat_error_message("Failure to clear bootcfg partition");
				io_close(handle);
				return -1;
			}
			io_close(handle);

			NOTICE("Bootcfg partition cleared\n");
		} else {
			plat_error_message("Failure to clear bootcfg partition");
			return -1;
		}
		return -1;
	}

	if (err == 0)
		/* Flush the image to main memory so that it can be executed
		 * later by any CPU, regardless of cache and MMU state.
		 */
		flush_dcache_range(BOOTCFG_BASE, BOOTCFG_MAX_SIZE);

	return 0;
}
