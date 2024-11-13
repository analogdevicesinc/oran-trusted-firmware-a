/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <arch.h>
#include <arch_helpers.h>
#include <bl31/ehf.h>
#include <common/bl_common.h>
#include <common/fdt_wrappers.h>
#include <drivers/adi/adi_te_interface.h>
#include <drivers/generic_delay_timer.h>
#include <lib/libfdt/libfdt.h>
#include <lib/mmio.h>
#include <platform.h>

#include <platform_def.h>
#include <plat_boot.h>
#include <plat_console.h>
#include <plat_device_profile.h>
#include <plat_interrupts.h>
#include <plat_fixup_hw_config.h>
#include <plat_int_gicv3.h>
#include <plat_pintmux.h>
#include <plat_mmap.h>
#include <plat_setup.h>
#include <plat_wdt.h>

/*
 * Memory region mappings
 */
#define MAP_BL31_TOTAL           MAP_REGION_FLAT(                        \
		bl31_tzram_layout.total_base,    \
		bl31_tzram_layout.total_size,    \
		MT_MEMORY | MT_RW | MT_SECURE)

/* Data structure which holds the extents of the trusted SRAM for BL31 */
static meminfo_t bl31_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

/*
 * Check that BL31_BASE is above FW_CONFIG_LIMIT. This reserved page is
 * for `meminfo_t` data structure and fw_configs passed from BL1.
 */
CASSERT(BL31_BASE >= FW_CONFIG_LIMIT, assert_bl31_base_overflows);

/*
 * Placeholder variables for copying the arguments that have been passed to
 * BL31 from BL2.
 */
static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

static int plat_eth_fixup(void *hw_config_dtb)
{
	int err;
	int node;
	int i;
	char name[MAX_NODE_NAME_LENGTH];
	char *node_name;
	uint8_t *mac;
	int num_macs = plat_get_num_macs();

	/* Update Ethernet MACs */
	for (i = 0; i < num_macs; i++) {
		/* Get U-Boot eth node name */
		if (i == 0) snprintf(name, MAX_NODE_NAME_LENGTH, "/ethernet@%08x", EMAC_1G_BASE);
		else snprintf(name, MAX_NODE_NAME_LENGTH, "/emac@%d", i + 1);
		node_name = name;

		/* There is a separate node for SystemC 1G ethernet (different IP and driver) */
		if (plat_is_sysc() == true)
			if (i == 0) node_name = "/ethernet@2";

		/* Do not ovwrride an existing U-Boot device tree config */
		node = fdt_path_offset(hw_config_dtb, node_name);
		if ((node >= 0) &&
		    (NULL != fdt_getprop(hw_config_dtb, node, "mac-address", NULL)))
			continue;

		err = plat_get_mac_setting(i, &mac);
		if (err < 0) {
			/* Do not propagate.
			 * If the error is MAC not found, just do nothing
			 */
			if (err != ENOATTR)
				return err;
		} else {
			/* Propagate forward the MAC info to the U-Boot DTB (HW_CONFIG) */
			if (node < 0) {
				node = fdt_add_subnode(hw_config_dtb, 0, node_name + 1); /* +1 to use 'emacx' instead of '/emacx' */
				if (node < 0)
					return node;
			}

			err = fdt_setprop(hw_config_dtb, node, "mac-address", mac, ETH_LEN);
			if (err < 0)
				return err;
		}
	}

	return 0;
}

/*******************************************************************************
 * Makes any necessary modifications to the HW_CONFIG file before it is passed
 * to BL33
 ******************************************************************************/
static void fixup_hw_config(void)
{
	const char *boot_device;
	const char *boot_slot;
	uint32_t dram_size = 0;
	int err = -1;
	void *hw_config_dtb = (void *)(uintptr_t)HW_CONFIG_BASE;
	int is_deployed;
	bool kaslr_valid;
	uint64_t kaslr_seed;
	const char *lifecycle_state_str;
	int node = -1;
	uint32_t ns_dram_size = 0;
	int root_node = -1;
	const char *te_boot_slot;
	uint32_t nv_ctr, app_sec_ver, cert_nv_ctr, enforcement_ctr, te_enforcement_ctr;
	char node_name[MAX_NODE_NAME_LENGTH];

	/* Get the boot device */
	boot_device = plat_get_boot_device_str(plat_get_boot_device());

	/* Get the boot slot */
	boot_slot = plat_get_boot_slot();

	if (plat_is_bootrom_bypass_enabled()) {
		te_boot_slot = "N/A";
		lifecycle_state_str = "Unknown";
		is_deployed = 0;
		kaslr_valid = false;
		kaslr_seed = 0xDEADBEEF;
		nv_ctr = 0;
		app_sec_ver = 0;
		enforcement_ctr = 0;
		te_enforcement_ctr = 0;

		err = fdt_open_into(hw_config_dtb, hw_config_dtb, HW_CONFIG_MAX_SIZE);
		if (err < 0) {
			ERROR("Failed to open HW_CONFIG %d\n", err);
			plat_error_handler(err);
		}

		node = fdt_path_offset(hw_config_dtb, "/boot");
		if (node < 0) {
			ERROR("Failed to find '/boot' node in HW_CONFIG %d\n", node);
			plat_error_handler(node);
		}

		err = fdt_setprop_empty(hw_config_dtb, node, "bootrom_bypass");
		if (err != 0) {
			ERROR("Failed to set bootrom_bypass property in boot node\n");
			plat_error_handler(err);
		}
	} else {
		/* Get lifecycle and TE-related info */
		te_boot_slot = adi_enclave_get_active_boot_slot(TE_MAILBOX_BASE);
		lifecycle_state_str = adi_enclave_get_lifecycle_state_str(TE_MAILBOX_BASE);
		if (adi_enclave_get_lifecycle_state(TE_MAILBOX_BASE) == ADI_LIFECYCLE_DEPLOYED)
			is_deployed = 1;
		else
			is_deployed = 0;

		/* Populate the KASLR seed with a value from TE's RNG */
		err = adi_enclave_random_bytes(TE_MAILBOX_BASE, (void *)&kaslr_seed, sizeof(kaslr_seed));
		if (err != 0) {
			WARN("Failed to set KASLR seed. TE error %d.\n", err);
			kaslr_valid = false;
		} else {
			kaslr_valid = true;
		}

		/* Anti-rollback fixup
		 * Get enforcement counter from OTP */
		err = plat_get_enforcement_counter(&enforcement_ctr);
		if (err < 0) {
			ERROR("Failed to get anti-rollback enforcement counter\n");
			plat_error_handler(err);
		}

		/* Get TE enforcement counter from OTP */
		err = adi_enclave_get_otp_app_anti_rollback(TE_MAILBOX_BASE, &te_enforcement_ctr);
		if (err < 0) {
			ERROR("Failed get TE anti-rollback counter from OTP\n");
			plat_error_handler(err);
		}

		/* Get FIP certificate anti-rollback counter from FW_CONFIG
		 * If FIP certificate anti-rollback counter value is not set,
		 * 0, use enforcement counter from OTP.
		 * FIP certificate anti-rollback counter is only set when the
		 * nv counter is greater than the enforcement counter.
		 * If they are equal, the FIP certificate anti-rollback counter
		 * will be 0, and the enforcement counter can be used as the
		 * FIP anti-rollback counter.
		 */
		cert_nv_ctr = plat_get_fw_config_rollback_ctr();
		if (cert_nv_ctr != 0) {
			nv_ctr = cert_nv_ctr;
		} else {
			err = plat_get_enforcement_counter(&nv_ctr);
			if (err < 0) {
				ERROR("Failed to get anti-rollback enforcement counter\n");
				plat_error_handler(err);
			}
		}

		/* Get TE anti-rollback counter from FW_CONFIG */
		app_sec_ver = plat_get_fw_config_te_rollback_ctr();
	}

	/* Calculate the size of NS DRAM
	 * This is the remaining DRAM that hasn't been allocated for the TEE
	 */
	dram_size = plat_get_dram_size();
	assert(dram_size != 0);
	assert(dram_size > TEE_DRAM_SIZE);
	ns_dram_size = dram_size - TEE_DRAM_SIZE;

	/* Then edit the 'memory' node in the HW_CONFIG to use the correct
	 * DRAM address/size
	 */

	/* TODO: Might not need this level of robustness / error handling in this
	 * function. Inability to set the memory size is really a programming error,
	 * and shouldn't be something that will be encountered in the field. HW_CONFIG
	 * will either be valid and loaded, or not loaded at all by BL2. So a simple
	 * assert() here might suffice.
	 */

	err = fdt_open_into(hw_config_dtb, hw_config_dtb, HW_CONFIG_MAX_SIZE);
	if (err < 0) {
		ERROR("Failed to open HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	root_node = fdt_path_offset(hw_config_dtb, "/");
	if (root_node < 0) {
		ERROR("Failed to find root node in HW_CONFIG %d\n", root_node);
		plat_error_handler(root_node);
	}

	node = fdt_path_offset(hw_config_dtb, "/memory");
	if (node < 0) {
		ERROR("Failed to find 'memory' node in HW_CONFIG %d\n", node);
		plat_error_handler(node);
	}

	err = fdt_delprop(hw_config_dtb, node, "reg");
	if (err < 0) {
		ERROR("Failed to delete 'reg' prop in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	err = fdt_appendprop_addrrange(hw_config_dtb, root_node, node, "reg", NS_DRAM_BASE, ns_dram_size);
	if (err < 0) {
		ERROR("Failed to add 'reg' prop in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	/* Update boot-related parameters */

	node = fdt_path_offset(hw_config_dtb, "/boot");
	if (node < 0) {
		ERROR("Failed to find 'boot' node in HW_CONFIG %d\n", node);
		plat_error_handler(node);
	}

	err = fdt_setprop_string(hw_config_dtb, node, "slot", boot_slot);
	if (err < 0) {
		ERROR("Failed to set 'slot' property in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	err = fdt_setprop_string(hw_config_dtb, node, "device", boot_device);
	if (err < 0) {
		ERROR("Failed to set 'device' property in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	err = fdt_setprop_string(hw_config_dtb, node, "te-slot", te_boot_slot);
	if (err < 0) {
		ERROR("Failed to set 'te-slot' property in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	node = fdt_path_offset(hw_config_dtb, "/boot/lifecycle-state");
	if (node < 0) {
		ERROR("Failed to find '/boot/lifecycle-state' node in HW_CONFIG %d\n", node);
		plat_error_handler(node);
	}

	err = fdt_setprop_string(hw_config_dtb, node, "description", lifecycle_state_str);
	if (err < 0) {
		ERROR("Failed to set 'description' property in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	err = fdt_setprop_u32(hw_config_dtb, node, "deployed", is_deployed);
	if (err < 0) {
		ERROR("Failed to set 'deployed' property in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	/* Set KASLR seed parameter */
	if (kaslr_valid) {
		node = fdt_path_offset(hw_config_dtb, "/boot");
		if (node < 0) {
			ERROR("Failed to find '/boot' node in HW_CONFIG %d\n", node);
			plat_error_handler(node);
		}
		err = fdt_appendprop_u64(hw_config_dtb, node, "kaslr-seed", kaslr_seed);
		if (err < 0) {
			ERROR("Failed to set 'kaslr-seed' property in HW_CONFIG %d\n", err);
			plat_error_handler(err);
		}
	}

	err = plat_eth_fixup(hw_config_dtb);
	if (err < 0) {
		ERROR("Failed to fixup eth MACs in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	/* plat specific hw config fixup */
	err = plat_fixup_hw_config(hw_config_dtb);
	if (err < 0) {
		ERROR("Failed to fixup plat HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	node = fdt_path_offset(hw_config_dtb, "/boot");
	if (node < 0) {
		ERROR("Failed to find '/boot' node in HW_CONFIG %d\n", node);
		plat_error_handler(node);
	}

	/* Add anti-rollback subnode under boot node */
	snprintf(node_name, sizeof(node_name), "anti-rollback");
	node = fdt_add_subnode(hw_config_dtb, node, node_name);
	if (node < 0) {
		ERROR("Failed to add 'anti-rollback' subnode in HW_CONFIG %d\n", node);
		plat_error_handler(node);
	}

	/* Add TE enforcement counter to HW_CONFIG */
	err = fdt_setprop_u32(hw_config_dtb, node, "te-enforcement-counter", te_enforcement_ctr);
	if (err < 0) {
		ERROR("Failed to set te-enforcement-counter in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	/* Add enforcement counter to HW_CONFIG */
	err = fdt_setprop_u32(hw_config_dtb, node, "enforcement-counter", enforcement_ctr);
	if (err < 0) {
		ERROR("Failed to set enforcement-counter in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	/* Add TE OTP anti-rollback counter to HW_CONFIG */
	err = fdt_setprop_u32(hw_config_dtb, node, "te-anti-rollback-counter", app_sec_ver);
	if (err < 0) {
		ERROR("Failed to set te-anti-rollback-counter in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	/* Add FIP anti-rollback counter to HW_CONFIG */
	err = fdt_setprop_u32(hw_config_dtb, node, "anti-rollback-counter", nv_ctr);
	if (err < 0) {
		ERROR("Failed to set anti-rollback-counter in HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	err = fdt_pack(hw_config_dtb);
	if (err < 0) {
		ERROR("Failed to pack HW_CONFIG %d\n", err);
		plat_error_handler(err);
	}

	clean_dcache_range((uintptr_t)hw_config_dtb, fdt_blob_size(hw_config_dtb));
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for the
 * security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
struct entry_point_info *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	next_image_info = (type == NON_SECURE)
			  ? &bl33_image_ep_info : &bl32_image_ep_info;

	/* None of the images can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1, u_register_t arg2, u_register_t arg3)
{
	/* Perform early setup */
	plat_bl31_early_setup();

	/* Enable watchdog */
	plat_secure_wdt_start();

	/* Enable boot console */
	plat_console_boot_init();

	/* Enable runtime console before OP-TEE is launched below, as OP-TEE expects it to be initialized already */
	plat_console_runtime_init();

	/* Setup BL31 RAM region */
	bl31_tzram_layout.total_base = BL_RAM_BASE;
	bl31_tzram_layout.total_size = BL31_END - BL_RAM_BASE;

	/*
	 * Check params passed from BL2 should not be NULL,
	 */
	bl_params_t *params_from_bl2 = (bl_params_t *)(void *)(arg0);

	assert(params_from_bl2 != NULL);
	assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
	assert(params_from_bl2->h.version >= VERSION_2);

	bl_params_node_t *bl_params = params_from_bl2->head;

	/*
	 * Copy BL33 and BL32 (if present), entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	while (bl_params != NULL) {
		if (bl_params->image_id == BL32_IMAGE_ID)
			bl32_image_ep_info = *bl_params->ep_info;

		if (bl_params->image_id == BL33_IMAGE_ID)
			bl33_image_ep_info = *bl_params->ep_info;

		bl_params = bl_params->next_params_info;
	}

	if (bl33_image_ep_info.pc == 0U)
		panic();
}

void bl31_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_BL31_TOTAL,
		PLAT_MAP_BL_RO,
		{ 0 }
	};

	/* Setup and enable MMU */
	setup_page_tables(bl_regions, plat_get_mmap());
	enable_mmu_el3(0);
}

void bl31_platform_setup(void)
{
	/* Initialize the GIC driver, cpu and distributor interfaces */
	plat_gic_driver_init();
	plat_gic_init();
#if EL3_EXCEPTION_HANDLING
	ehf_register_priority_handler(PLAT_IRQ_NORMAL_PRIORITY, plat_interrupt_handler);
#endif
	plat_pintmux_init();

	/* Enable system timer */
	generic_delay_timer_init();

	/* Do platform-specific setup */
	plat_bl31_setup();

	/* Update the HW_CONFIG file */
	fixup_hw_config();
}

/*******************************************************************************
 * Perform any BL31 platform runtime setup prior to BL31 exit
 ******************************************************************************/
void bl31_plat_runtime_setup(void)
{
	/* Initialize the runtime console */
	plat_console_runtime_init();

	/* Switch to runtime console */
	console_switch_state(CONSOLE_FLAG_RUNTIME);

	/* Stop using the boot console */
	plat_console_boot_end();
}
