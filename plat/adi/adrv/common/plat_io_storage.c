/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2019, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* TODO: Need to update all copyright info in all ADI files */
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <common/tbbr/tbbr_img_def.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_storage.h>
#include <drivers/io/io_mtd.h>
#include <drivers/mmc.h>
#include <drivers/partition/partition.h>
#include <drivers/spi_nor.h>
#include <lib/mmio.h>
#include <lib/utils.h>
#include <platform.h>
#include <tools_share/firmware_image_package.h>

#include <plat_board.h>
#include <plat_boot.h>
#include <plat_bootctrl.h>
#include <plat_device_profile.h>
#include <plat_err.h>
#include <plat_io_storage.h>

#define PLAT_MMC_BUFFER_SIZE    (MMC_BLOCK_SIZE)

#define PLAT_FIP_BASE           (0)
#define PLAT_FIP_MAX_SIZE       (0x1000000)
#define PLAT_CFG_BASE           (0)
#define PLAT_CFG_MAX_SIZE       (0x200000)

/* Ensure minimum DRAM size is sufficient to hold the DRAM size required for TEE */
CASSERT(DRAM_SIZE_MIN >= TEE_DRAM_SIZE, DRAM_SIZE_MIN_smaller_than_TEE_DRAM_SIZE);

/* Ensure BL33 is located in the NS DRAM region */
CASSERT(BL33_BASE >= NS_DRAM_BASE, BL33_BASE_not_in_NS_DRAM);

static const io_dev_connector_t *fip_dev_con;
static const io_dev_connector_t *boot_dev_con;

static uint8_t mmc_block_buffer[PLAT_MMC_BUFFER_SIZE] __aligned(PLAT_MMC_BUFFER_SIZE);

static uintptr_t fip_dev_handle;
static uintptr_t boot_dev_handle;

static const io_uuid_spec_t uuid_spec[MAX_NUMBER_IDS] = {
	[BL2_IMAGE_ID] =		   { UUID_TRUSTED_BOOT_FIRMWARE_BL2   },
	[TB_FW_CONFIG_ID] =		   { UUID_TB_FW_CONFIG		      },
	[FW_CONFIG_ID] =		   { UUID_FW_CONFIG		      },
	[BL31_IMAGE_ID] =		   { UUID_EL3_RUNTIME_FIRMWARE_BL31   },
	[BL32_IMAGE_ID] =		   { UUID_SECURE_PAYLOAD_BL32	      },
	[BL33_IMAGE_ID] =		   { UUID_NON_TRUSTED_FIRMWARE_BL33   },
	[HW_CONFIG_ID] =		   { UUID_HW_CONFIG		      },
#if TRUSTED_BOARD_BOOT
	[TRUSTED_BOOT_FW_CERT_ID] =	   { UUID_TRUSTED_BOOT_FW_CERT	      },
	[TRUSTED_KEY_CERT_ID] =		   { UUID_TRUSTED_KEY_CERT	      },
	[SOC_FW_KEY_CERT_ID] =		   { UUID_SOC_FW_KEY_CERT	      },
	[TRUSTED_OS_FW_KEY_CERT_ID] =	   { UUID_TRUSTED_OS_FW_KEY_CERT      },
	[NON_TRUSTED_FW_KEY_CERT_ID] =	   { UUID_NON_TRUSTED_FW_KEY_CERT     },
	[SOC_FW_CONTENT_CERT_ID] =	   { UUID_SOC_FW_CONTENT_CERT	      },
	[TRUSTED_OS_FW_CONTENT_CERT_ID] =  { UUID_TRUSTED_OS_FW_CONTENT_CERT  },
	[NON_TRUSTED_FW_CONTENT_CERT_ID] = { UUID_NON_TRUSTED_FW_CONTENT_CERT },
#endif /* TRUSTED_BOARD_BOOT */
};
static const io_block_spec_t gpt_block_spec = {
	.offset = 0,
	/*
	 * PLAT_PARTITION_BLOCK_SIZE = 512
	 * PLAT_PARTITION_MAX_ENTRIES = 32
	 * each sector has 4 partition entries, and there are
	 * 2 reserved sectors i.e. protective MBR and primary
	 * GPT header hence length gets calculated as,
	 * length = 512 * (32/4 + 2)
	 */
	.length = PLAT_PARTITION_BLOCK_SIZE *
		  (PLAT_PARTITION_MAX_ENTRIES / 4 + 2),
};

/* Expect platform block size to match MMC block size, and
 * max partition entries to be set to 32.
 */
CASSERT(PLAT_PARTITION_BLOCK_SIZE == MMC_BLOCK_SIZE, part_block_and_mmc_block_mismatch);
CASSERT(PLAT_PARTITION_MAX_ENTRIES == 32, plat_part_max_entries_unexpected_size);

static int check_fip(const uintptr_t spec);
static int check_dev(const uintptr_t spec);

static io_block_dev_spec_t boot_dev_spec;
static int (*register_io_dev)(const io_dev_connector_t **);
static io_mtd_dev_spec_t spi_nor_dev_spec;

static io_block_spec_t fip_spec = {
	.offset = PLAT_FIP_BASE,
	.length = PLAT_FIP_MAX_SIZE,
};

static io_block_spec_t ctrl_spec = {
	.offset = PLAT_CFG_BASE,
	.length = PLAT_CFG_MAX_SIZE,
};

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const struct plat_io_policy policies[] = {
	[GPT_IMAGE_ID] =		   {
		&boot_dev_handle,
		(uintptr_t)&gpt_block_spec,
		check_dev
	},
	[FIP_IMAGE_ID] =		   {
		&boot_dev_handle,
		(uintptr_t)&fip_spec,
		check_dev
	},
	[FW_CONFIG_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[FW_CONFIG_ID],
		check_fip
	},
	[BL2_IMAGE_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[BL2_IMAGE_ID],
		check_fip
	},
	[BL31_IMAGE_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[BL31_IMAGE_ID],
		check_fip
	},
	[BL32_IMAGE_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[BL32_IMAGE_ID],
		check_fip
	},
	[BL33_IMAGE_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[BL33_IMAGE_ID],
		check_fip
	},
	[HW_CONFIG_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[HW_CONFIG_ID],
		check_fip
	},
#if TRUSTED_BOARD_BOOT
	[TRUSTED_BOOT_FW_CERT_ID] =	   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[TRUSTED_BOOT_FW_CERT_ID],
		check_fip
	},
	[TRUSTED_KEY_CERT_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[TRUSTED_KEY_CERT_ID],
		check_fip
	},
	[SOC_FW_KEY_CERT_ID] =		   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[SOC_FW_KEY_CERT_ID],
		check_fip
	},
	[TRUSTED_OS_FW_KEY_CERT_ID] =	   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[TRUSTED_OS_FW_KEY_CERT_ID],
		check_fip
	},
	[NON_TRUSTED_FW_KEY_CERT_ID] =	   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[NON_TRUSTED_FW_KEY_CERT_ID],
		check_fip
	},
	[SOC_FW_CONTENT_CERT_ID] =	   {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[SOC_FW_CONTENT_CERT_ID],
		check_fip
	},
	[TRUSTED_OS_FW_CONTENT_CERT_ID] =  {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[TRUSTED_OS_FW_CONTENT_CERT_ID],
		check_fip
	},
	[NON_TRUSTED_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&uuid_spec[NON_TRUSTED_FW_CONTENT_CERT_ID],
		check_fip
	},
#endif /* TRUSTED_BOARD_BOOT */
};

static int check_dev(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(boot_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(boot_dev_handle, spec, &local_handle);
		if (result == 0)
			io_close(local_handle);
	}
	return result;
}

static int check_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_handle);
		if (result == 0)
			io_close(local_handle);
	}
	return result;
}

int plat_get_partition_spec(const char *partition_id, io_block_spec_t *spec)
{
	plat_boot_device_t boot_device;
	const partition_entry_t *entry = NULL;

	boot_device = plat_get_boot_device();
	if (boot_device == PLAT_BOOT_DEVICE_QSPI_0) {
		entry = plat_get_nor_part_entry(partition_id);
	} else {
		partition_init(GPT_IMAGE_ID);
		entry = get_partition_entry(partition_id);
	}

	if (entry != NULL) {
		spec->offset = entry->start;
		spec->length = entry->length;
	} else {
		return -1;
	}

	return 0;
}

void plat_io_setup(bool use_bootctrl, uint32_t reset_cause, uint32_t reset_cause_ns)
{
	char *active_slot;
	plat_boot_device_t boot_device;
	const char *ctrl_partition_id = BOOTCTRL_PARTITION_NAME;
	char fip_partition_id[FIP_PARTITION_NAME_SIZE];
	io_block_spec_t *p_ctrl_spec;
	int result = -1;
	char slot_data[] = " ";

	boot_device = plat_get_boot_device();
	NOTICE("Active boot device: %s\n", plat_get_boot_device_str(boot_device));

	switch (boot_device) {
	case PLAT_BOOT_DEVICE_SD_0:
	case PLAT_BOOT_DEVICE_EMMC_0:
		register_io_dev = &register_io_dev_block;
		boot_dev_spec.buffer.offset = (size_t)mmc_block_buffer;
		boot_dev_spec.buffer.length = PLAT_MMC_BUFFER_SIZE;
		boot_dev_spec.ops.read = mmc_read_blocks;
		boot_dev_spec.ops.write = mmc_write_blocks;
		boot_dev_spec.block_size = MMC_BLOCK_SIZE;
		break;

	case PLAT_BOOT_DEVICE_QSPI_0:
		register_io_dev = &register_io_dev_mtd;
		spi_nor_dev_spec.ops.init = spi_nor_init;
		spi_nor_dev_spec.ops.read = spi_nor_read;
		spi_nor_dev_spec.ops.write = spi_nor_write;
		break;

	case PLAT_BOOT_DEVICE_HOST:
		register_io_dev = &register_io_dev_memmap;
		break;

	default:
		ERROR("Unsupported boot device\n");
		/* Manage error depending on the boot stage:
		 *  This plat_io_setup function is called from both BL1 and BL2.
		 *  BL1 calls plat_io_setup with "use_bootctrl" enabled, and BL2 with "use_bootctrl" disabled:
		 */
		if (use_bootctrl)
			/* BL1: we do not have a proper reset/recovery mechanism, so we just call system_reset */
			plat_board_system_reset();
		else
			/* BL2: we already have the reset/recovery mechanishm, so we can run the error_handler */
			plat_error_handler(-EINVAL);
		break;
	}

	/* Initialize the boot device */
	plat_init_boot_device();

	/* These register() and open() calls will only fail due to
	 * static misconfiguration (e.g. MAX_IO_DEVICES is wrong), not
	 * transient failures. So an assert() is acceptable here.
	 */

	result = (*register_io_dev)(&boot_dev_con);
	assert(result == 0);

	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);

	if (boot_device == PLAT_BOOT_DEVICE_QSPI_0)
		result = io_dev_open(boot_dev_con, (uintptr_t)&spi_nor_dev_spec, &boot_dev_handle);
	else if (boot_device == PLAT_BOOT_DEVICE_HOST)
		result = io_dev_open(boot_dev_con, (uintptr_t)NULL, &boot_dev_handle);
	else
		result = io_dev_open(boot_dev_con, (uintptr_t)&boot_dev_spec, &boot_dev_handle);
	assert(result == 0);

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);

	/* Locate the FIP */
	if (boot_device == PLAT_BOOT_DEVICE_HOST) {
		/* For host boot, this is specified by the host boot addr */
		fip_spec.offset = plat_get_host_boot_addr();

		/* Bootctrl init to set active boot slot to default */
		plat_bootctrl_init(boot_dev_handle, (uintptr_t)NULL, reset_cause, reset_cause_ns);
	} else {
		/* Determine the active boot slot */
		if (use_bootctrl) {
			/* Get a handle to the bootctrl partition */
			result = plat_get_partition_spec(ctrl_partition_id, &ctrl_spec);
			if (result != 0) {
				ERROR("Unable to find the %s partition\n", ctrl_partition_id);
				p_ctrl_spec = NULL;
			} else {
				p_ctrl_spec = &ctrl_spec;
			}

			/* If using bootctrl, initialize it here and use it to get the active boot slot.
			 * Even if we fail to get a handle to the bootctrl partition, continue to initialize
			 * bootctrl as it will select a fallback option for us.
			 */
			plat_bootctrl_init(boot_dev_handle, (uintptr_t)p_ctrl_spec, reset_cause, reset_cause_ns);
			active_slot = &(slot_data[0]);
			active_slot[0] = plat_bootctrl_get_active_slot();
		} else {
			/* If not using bootctrl, query the platform for the active boot slot */
			active_slot = (char *)plat_get_boot_slot();
		}
		NOTICE("Active boot slot: %s\n", active_slot);

		/* Find the FIP partition on disk */
		snprintf(fip_partition_id, sizeof(fip_partition_id), "%s%s", FIP_PARTITION_BASE_NAME, active_slot);
		result = plat_get_partition_spec(fip_partition_id, &fip_spec);
		if (result != 0) {
			ERROR("Unable to find the %s partition\n", fip_partition_id);
			plat_error_handler(-ENOENT);
		}
	}
}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result = -1;
	const struct plat_io_policy *policy = NULL;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];

	assert(policy->check != 0);
	result = policy->check(policy->image_spec);

	if (result == 0) {
		*image_spec = policy->image_spec;
		*dev_handle = *(policy->dev_handle);
	}

	return result;
}

uintptr_t plat_get_boot_handle(void)
{
	return boot_dev_handle;
}
