/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <lib/mmio.h>

#include <adrv906x_clkrst_def.h>
#include <adrv906x_core.h>
#include <adrv906x_gpint.h>
#include <platform_def.h>

#define GPINT_BYTES     (12)
#define GPINT_NUM       (2)
#define NUM_BYTES_PER_WORD              (6)
#define BYTE_SIZE       (8)
#define BYTE_MASK       (0xFF)

static struct gpint_settings gp_settings; /*Define routing for each bit of the GPINT. 1=Route through SDEI to NS world, 0= handle here in Bl31*/

/* Register offsets for GPINT0 and GPINT1 mask bits */
static uintptr_t gpint_mask_offset[GPINT_BYTES][GPINT_NUM] = {
	{ GP_INTERRUPT_MASK_PIN_0_BYTE0_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE0_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE1_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE1_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE2_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE2_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE3_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE3_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE4_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE4_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE5_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE5_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE6_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE6_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE7_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE7_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE8_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE8_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE9_OFFSET,	 GP_INTERRUPT_MASK_PIN_1_BYTE9_OFFSET  },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE10_OFFSET, GP_INTERRUPT_MASK_PIN_1_BYTE10_OFFSET },
	{ GP_INTERRUPT_MASK_PIN_0_BYTE11_OFFSET, GP_INTERRUPT_MASK_PIN_1_BYTE11_OFFSET },
};

/* Register offsets for GPINT status bits */
static uintptr_t gpint_status_offset[GPINT_BYTES] = {
	GP_INTERRUPT_STATUS_READ_BYTE0_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE1_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE2_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE3_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE4_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE5_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE6_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE7_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE8_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE9_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE10_OFFSET,
	GP_INTERRUPT_STATUS_READ_BYTE11_OFFSET
};

/* Register offsets for GPINT level/pulse configuration bits */
static uintptr_t gpint_level_pulse_offset[GPINT_BYTES] = {
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE0_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE1_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE2_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE3_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE4_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE5_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE6_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE7_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE8_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE9_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE10_OFFSET,
	GP_INTERRUPT_LEVEL_PULSE_B_BYTE11_OFFSET
};

static void adrv906x_gpint_clear_status(uintptr_t gpint_base_addr)
{
	int byte;

	for (byte = 0; byte < GPINT_BYTES; byte++)
		/* Clear each interrupt status byte, write 1 to clear */
		mmio_write_8(gpint_base_addr + gpint_status_offset[byte], BYTE_MASK);
}

static void adrv906x_gpint_level_pulse(uintptr_t gpint_base_addr)
{
	uint8_t byte_set;
	int byte;

	/*
	 * Set GPINT level/pulse configuration based on upper and lower word configurations
	 * Set high for level and low for pulse. Default is assumed to be pulse
	 */
	for (byte = 0; byte < GPINT_BYTES; byte++) {
		if (byte < NUM_BYTES_PER_WORD)
			byte_set = (GPINT_LEVEL_PULSE_LOWER_CONFIG >> (BYTE_SIZE * byte)) & BYTE_MASK;
		else
			byte_set = (GPINT_LEVEL_PULSE_UPPER_CONFIG >> (BYTE_SIZE * (byte - NUM_BYTES_PER_WORD))) & BYTE_MASK;

		mmio_write_8(gpint_base_addr + gpint_level_pulse_offset[byte], byte_set);
	}
}

void adrv906x_gpint_init(uintptr_t base_addr)
{
	/* Clear status bytes */
	adrv906x_gpint_clear_status(base_addr);

	/* Initialize all pulse/level configs */
	adrv906x_gpint_level_pulse(base_addr);
}

/* Gets the current status bits for the upper and lower words */
void adrv906x_gpint_get_status(uintptr_t gpint_base_addr, struct gpint_settings *settings)
{
	uint8_t byte_current;
	int byte;

	settings->lower_word = 0;
	settings->upper_word = 0;

	/* Read GPINT status byte by byte and store into corresponding upper and lower words */
	for (byte = 0; byte < GPINT_BYTES; byte++) {
		byte_current = mmio_read_8(gpint_base_addr + gpint_status_offset[byte]);

		if (byte < NUM_BYTES_PER_WORD)
			settings->lower_word |= (((uint64_t)byte_current) << (BYTE_SIZE * byte));
		else
			settings->upper_word |= (((uint64_t)byte_current) << (BYTE_SIZE * (byte - NUM_BYTES_PER_WORD)));
	}
}

/*
 * Enables GPINT (GPINT0 or GPINT1) for the desired interrupt signals based on the lower word and upper word of the settings parameter
 * Set the bits in the upper and lower words using the bit masks in adrv906x_gpint_def.h to enable the interrupt signals
 */
void adrv906x_gpint_enable(uintptr_t gpint_base_addr, uint32_t gpint, struct gpint_settings *settings)
{
	uint8_t byte_current;
	uint8_t byte_set;
	int byte;

	assert(gpint < GPINT_NUM);

	/*
	 * Set desired GPINT masking for upper and lower words.
	 * Setting the bit low allows the GPINT pin to go active when a source interrupt occurs
	 */
	for (byte = 0; byte < GPINT_BYTES; byte++) {
		byte_current = mmio_read_8(gpint_base_addr + gpint_mask_offset[byte][gpint]);

		if (byte < NUM_BYTES_PER_WORD)
			byte_set = (settings->lower_word >> (BYTE_SIZE * byte)) & BYTE_MASK;
		else
			byte_set = (settings->upper_word >> (BYTE_SIZE * (byte - NUM_BYTES_PER_WORD))) & BYTE_MASK;

		mmio_write_8(gpint_base_addr + gpint_mask_offset[byte][gpint], (byte_current & ~byte_set));
	}
}

/*
 * Disables GPINT (GPINT0 or GPINT1) for the desired interrupt signals based on the lower word and upper word of the settings parameter
 * Set the bits in the upper and lower words using the bit masks in adrv906x_gpint_def.h to disable the interrupt signals
 */
void adrv906x_gpint_disable(uintptr_t gpint_base_addr, uint32_t gpint, struct gpint_settings *settings)
{
	uint8_t byte_current;
	uint8_t byte_set;
	int byte;

	assert(gpint < GPINT_NUM);

	/*
	 * Set desired GPINT masking for upper and lower words.
	 * Setting the bit high masks the GPINT pin from going active when a source interrupt occurs
	 */
	for (byte = 0; byte < GPINT_BYTES; byte++) {
		byte_current = mmio_read_8(gpint_base_addr + gpint_mask_offset[byte][gpint]);

		if (byte < NUM_BYTES_PER_WORD)
			byte_set = (settings->lower_word >> (BYTE_SIZE * byte)) & BYTE_MASK;
		else
			byte_set = (settings->upper_word >> (BYTE_SIZE * (byte - NUM_BYTES_PER_WORD))) & BYTE_MASK;

		mmio_write_8(gpint_base_addr + gpint_mask_offset[byte][gpint], (byte_current | byte_set));
	}
}

void adrv906x_gpint_warm_reset_enable(void)
{
	/* Set a55_sys_cfg gpint_warm_rst_en bit to enable warm reset when GPINT0 gets asserted due to a catastrophic event */
	mmio_write_32(A55_SYS_CFG + CLK_RST_WARM_RST_CTRL, mmio_read_32(A55_SYS_CFG + CLK_RST_WARM_RST_CTRL) | GPINT_WARM_RST_EN_MASK);
}


bool adrv906x_gpint_is_nonsecure(bool upper_word, uint64_t mask)
{
	if (upper_word) {
		if (mask & gp_settings.upper_word_route_nonsecure)
			return true;
		else
			return false;
	} else {
		if (mask & gp_settings.lower_word_route_nonsecure)
			return true;
		else
			return false;
	}
}

void adrv906x_gpint_set_routing(struct gpint_settings *settings)
{
	gp_settings.upper_word_route_nonsecure = settings->upper_word_route_nonsecure;
	gp_settings.lower_word_route_nonsecure = settings->lower_word_route_nonsecure;
}
