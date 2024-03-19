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
	const char *lifecycle_state_str;
	int node = -1;
	uint32_t ns_dram_size = 0;
	int parent = -1;
	const char *te_boot_slot;

	/* Get the boot device */
	boot_device = plat_get_boot_device_str(plat_get_boot_device());

	/* Get the boot slot */
	boot_slot = plat_get_boot_slot();

	/* Get lifecycle and TE-related info */
	te_boot_slot = adi_enclave_get_active_boot_slot(TE_MAILBOX_BASE);
	lifecycle_state_str = adi_enclave_get_lifecycle_state_str(TE_MAILBOX_BASE);
	if (adi_enclave_get_lifecycle_state(TE_MAILBOX_BASE) == ADI_LIFECYCLE_DEPLOYED)
		is_deployed = 1;
	else
		is_deployed = 0;

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

	parent = fdt_path_offset(hw_config_dtb, "/");
	if (parent < 0) {
		ERROR("Failed to find root node in HW_CONFIG %d\n", node);
		plat_error_handler(node);
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

	err = fdt_appendprop_addrrange(hw_config_dtb, parent, node, "reg", NS_DRAM_BASE, ns_dram_size);
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

	/* plat specific hw config fixup */
	err = plat_fixup_hw_config(hw_config_dtb);
	if (err < 0) {
		ERROR("Failed to fixup plat HW_CONFIG %d\n", err);
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
	console_switch_state(CONSOLE_FLAG_RUNTIME);

	/* Initialize the runtime console */
	plat_console_runtime_init();
}
