/*
 * Copyright (c) 2022, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <arch_helpers.h>
#include <arm_acle.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_driver.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <platform.h>
#include <plat_bootctrl.h>
#include <plat_err.h>
#include <plat_status_reg.h>

/* Header info for bootctrl structure */
#define PLAT_BOOTCTRL_CONFIG 0x0100000
#define PLAT_BOOTCTRL_MAGIC 0xAD1B007C
#define PLAT_BOOTCTRL_VERSION 0x00000001

/* Indication of invalid boot slot */
#define INVALID_BOOT_SLOT_ID '\0'

typedef struct __packed {
	/*Bootloader Control AB magic number. */
	uint32_t magic;
	/* Version of struct being used. */
	uint32_t version;
	/* Active Slot. */
	uint32_t active_slot;
	/* CRC32 of all bytes preceding this field. */
	uint32_t crc32;
} plat_bootctrl_t;

/* Local handles of boot_dev_handle and cfg_spec to read/write from/to */
static uintptr_t boot_dev_handle = (uintptr_t)NULL;
static uintptr_t cfg_spec = (uintptr_t)NULL;
static char active_boot_slot = INVALID_BOOT_SLOT_ID;

static int set_active_slot(char active_slot);
static int do_failure_detection(uint32_t reset_cause, uint32_t reset_cause_ns);
static int get_active_slot_from_disk(char *active_slot);
static int set_new_slot(char active_slot, char starting_slot);

/* Initialize bootctrl. Reads active boot slot from disk,
 * and performs boot error detection.
 */
void plat_bootctrl_init(uintptr_t dev_handle, uintptr_t spec, uint32_t reset_cause, uint32_t reset_cause_ns)
{
	char last_slot = INVALID_BOOT_SLOT_ID;
	int result = -1;
	char slot_str[] = " ";

	boot_dev_handle = dev_handle;
	cfg_spec = spec;

	active_boot_slot = INVALID_BOOT_SLOT_ID;

	/* Read active slot from bootctrl partition */
	result = get_active_slot_from_disk(&active_boot_slot);

	/* If the read failed, or the boot slot isn't in range, select a backup option */
	if (result != 0 || (active_boot_slot < BOOTCTRL_ACTIVE_SLOT_START || active_boot_slot > BOOTCTRL_ACTIVE_SLOT_LAST)) {
		/* If the last boot value (previous boot since last cold boot) is known, use it */
		last_slot = (char)plat_rd_status_reg(LAST_SLOT);
		if (last_slot != INVALID_BOOT_SLOT_ID) {
			active_boot_slot = last_slot;
			slot_str[0] = active_boot_slot;
			NOTICE("Bootctrl partition invalid value or failed to read. Setting to last known boot value of %s.\n", slot_str);
		} else {
			/* Otherwise, use the default slot */
			active_boot_slot = BOOTCTRL_ACTIVE_SLOT_START;
			slot_str[0] = active_boot_slot;
			NOTICE("Bootctrl partition invalid value or failed to read. Setting default value of %s.\n", slot_str);
		}

		/* Assuming the read failure above is a corruption issue, attempt to write it back to disk
		 * Failure here is not fatal as the bootctrl algorithm can continue to make progress.
		 */
		result = set_active_slot(active_boot_slot);
		if (result != 0)
			ERROR("Unable to write bootctrl partition\n");
	}

	/* Detect and respond to boot failure */
	result = do_failure_detection(reset_cause, reset_cause_ns);
	/* If boot failure detection results in an error all boot options have been exhausted, halt the system */
	if (result != 0)
		plat_halt_handler();

	/* Check active slot again in case boot failure detection changed it */
	active_boot_slot = plat_bootctrl_get_active_slot();
}

/*
 * Returns the cached copy of the active boot slot that was
 * initialized during plat_bootctrl_init()
 */
char plat_bootctrl_get_active_slot()
{
	assert(active_boot_slot != INVALID_BOOT_SLOT_ID);
	return active_boot_slot;
}

/* Retrieves the active boot slot from the bootctrl patition on disk */
static int get_active_slot_from_disk(char *active_slot)
{
	uint32_t crc = ~0u;
	uintptr_t handle;
	plat_bootctrl_t bootctrl;
	size_t length_read;
	int result;

	/* Check that boot_dev_handle and cfg_spec were set up to point to the handle and spec */
	if ((boot_dev_handle == (uintptr_t)NULL) || (cfg_spec == (uintptr_t)NULL))
		return -1;

	/* Read the bootctrl struct from disk */
	result = io_dev_init(boot_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(boot_dev_handle, cfg_spec, &handle);
		if (result == 0) {
			result = io_seek(handle, IO_SEEK_SET, 0);
			if (result == 0)
				result = io_read(handle, (uintptr_t)&bootctrl, sizeof(bootctrl), &length_read);
			io_close(handle);
		}
	}

	/* Check the data for validity */
	if (result == 0 && length_read == sizeof(bootctrl)) {
		if (bootctrl.magic != PLAT_BOOTCTRL_MAGIC)
			return -1;
		if (bootctrl.version != PLAT_BOOTCTRL_VERSION)
			return -1;

		/* Calculate crc */
		crc = __crc32w(crc, bootctrl.magic);
		crc = __crc32w(crc, bootctrl.version);
		crc = __crc32w(crc, bootctrl.active_slot);
		crc = ~crc;

		if (bootctrl.crc32 != crc)
			return -1;

		*active_slot = (char)(bootctrl.active_slot & 0xFF);
		return 0;
	}

	return -1;
}

/*
 * Sets the active boot slot. Updates value in
 * a) local cached copy, b) system status register
 * c) bootctrl partition on disk
 */
static int set_active_slot(char active_slot)
{
	plat_bootctrl_t bootctrl;
	unsigned long crc = ~0u;
	uintptr_t handle;
	size_t length_write = 0;
	int result;

	if ((active_slot < BOOTCTRL_ACTIVE_SLOT_START) ||
	    (active_slot > BOOTCTRL_ACTIVE_SLOT_LAST))
		return -1;

	/* Update cached copy */
	active_boot_slot = active_slot;

	/* Set "last known slot" value in system status register */
	plat_wr_status_reg(LAST_SLOT, (uint32_t)active_boot_slot);

	/* Check that boot_dev_handle and cfg_spec were set up to point to the handle and spec */
	if ((boot_dev_handle == (uintptr_t)NULL) || (cfg_spec == (uintptr_t)NULL))
		return -1;

	/* Setup bootctrl structure */
	bootctrl.magic = PLAT_BOOTCTRL_MAGIC;
	bootctrl.version = PLAT_BOOTCTRL_VERSION;
	bootctrl.active_slot = (uint32_t)active_slot;

	/* Calculate and set crc */
	crc = __crc32w(crc, bootctrl.magic);
	crc = __crc32w(crc, bootctrl.version);
	crc = __crc32w(crc, bootctrl.active_slot);
	crc = ~crc;
	bootctrl.crc32 = crc;

	/* Write boot partition block to disk */
	result = io_dev_init(boot_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(boot_dev_handle, cfg_spec, &handle);
		if (result == 0) {
			result = io_seek(handle, IO_SEEK_SET, 0);
			if (result == 0)
				result = io_write(handle, (uintptr_t)&bootctrl, sizeof(bootctrl), &length_write);
			io_close(handle);
		}
	}

	if (result == 0 && length_write == sizeof(bootctrl))
		return 0;

	return -1;
}

/* Set up new bootctrl slot.
 * Returns -1 if unable to advance to next boot slot
 */
static int set_new_slot(char active_slot, char starting_slot)
{
	uint32_t result;

	active_slot = active_slot + 1;

	/* If active slot is above threshold, reset to first slot */
	if (active_slot > BOOTCTRL_ACTIVE_SLOT_LAST)
		active_slot = BOOTCTRL_ACTIVE_SLOT_START;

	/* Update active slot ID in boot control block.
	 * Failure here is not fatal as the bootctrl algorithm can
	 * continue to make progress.
	 */
	result = set_active_slot(active_slot);
	if (result != 0)
		ERROR("Unable to write bootctrl partition\n");

	/* Clear BOOT_CNT */
	plat_wr_status_reg(BOOT_CNT, 0);

	/* If new slot ID is the same as STARTING_SLOT, try recovery boot */
	if (active_slot == starting_slot) {
		if (plat_rd_status_reg(RECOVERY_BOOT_ACTIVE) == 1)
			return -1;
		else
			plat_wr_status_reg(RECOVERY_BOOT_ACTIVE, 1);
	}
	return 0;
}

/* BL1 boot failure detection mechanism */
static int do_failure_detection(uint32_t reset_cause, uint32_t reset_cause_ns)
{
	char active_slot;
	uint32_t boot_cnt;
	char starting_slot;
	int result = 0;

	/* Get active slot */
	active_slot = plat_bootctrl_get_active_slot();

	/* Get starting slot */
	starting_slot = plat_rd_status_reg(STARTING_SLOT);

	/* If STARTING_SLOT register is 0, save active slot ID from boot control block to STARTING_SLOT register */
	if (starting_slot == 0)
		plat_wr_status_reg(STARTING_SLOT, active_slot);

	/* If RESET_CAUSE is set, perform failure detection/recovery */
	if ((reset_cause != RESET_VALUE) || (reset_cause_ns != RESET_VALUE)) {
		if (reset_cause != RESET_VALUE)
			NOTICE("Boot failure detected. Reset cause %d.\n", reset_cause);
		else
			NOTICE("Boot failure detected. Reset cause %d.\n", reset_cause_ns);

		/* Calculate current boot count and save it */
		boot_cnt = plat_rd_status_reg(BOOT_CNT) + 1;
		plat_wr_status_reg(BOOT_CNT, boot_cnt);

		/* If saved RESET_CAUSE register is IMG_VERIFY_FAIL, setup for new boot slot */
		result = 0;
		if ((reset_cause == IMG_VERIFY_FAIL) || (reset_cause_ns == IMG_VERIFY_FAIL)) {
			NOTICE("Image verification failure detected. Switching boot slot.\n");
			result = set_new_slot(active_slot, starting_slot);
		} else {
			/* If BOOT_CNT greater than threshold, setup for new boot slot */
			if (boot_cnt >= BOOTCTRL_BOOT_CNT_THRESHOLD) {
				NOTICE("Boot count threshold exceeded. Switching boot slot.\n");
				result = set_new_slot(active_slot, starting_slot);
			}
		}

		if (result == 0) {
			/* Either we're performing another boot attempt, or we're on the
			 * first attempt of a new slot. Print the current status.
			 */
			boot_cnt = plat_rd_status_reg(BOOT_CNT);
			NOTICE("Boot attempt %d of %d\n", boot_cnt + 1, BOOTCTRL_BOOT_CNT_THRESHOLD);
			if (plat_rd_status_reg(RECOVERY_BOOT_ACTIVE) == 1)
				NOTICE("Recovery boot is active\n");
		} else {
			/* result != 0 here indicates that set_new_slot() above failed to advance to
			 * the next slot
			 */
			ERROR("All boot options, including recovery, have been exhausted\n");
		}
	}

	return result;
}
