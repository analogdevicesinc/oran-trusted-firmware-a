/*
 * Copyright (c) 2025, Analog Devices Incorporated - All Rights Reserved
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

static struct gpint_settings gp_settings; /* Define routing for each bit of the GPINT. 1=Route through SDEI to NS world, 0= handle here in Bl31 */

typedef struct {
	uint64_t mask;
	const char *name;
} gpint_status_t;

gpint_status_t gpint_status_lower[] = {
	{ WATCHDOG_A55_TIMEOUT_PIPED_0_MASK,	     "WATCHDOG_A55_TIMEOUT_PIPED_0"	    },
	{ WATCHDOG_A55_TIMEOUT_PIPED_1_MASK,	     "WATCHDOG_A55_TIMEOUT_PIPED_1"	    },
	{ WATCHDOG_A55_TIMEOUT_PIPED_2_MASK,	     "WATCHDOG_A55_TIMEOUT_PIPED_2"	    },
	{ WATCHDOG_A55_TIMEOUT_PIPED_3_MASK,	     "WATCHDOG_A55_TIMEOUT_PIPED_3"	    },
	{ XCORR_ECC_ERROR_IRQ_PIPED_MASK,	     "XCORR_ECC_ERROR_IRQ_PIPED"	    },
	{ XCORR_ECC_ERROR_WARNING_PIPED_MASK,	     "XCORR_ECC_ERROR_WARNING_PIPED"	    },
	{ GIC_FAULT_INT_MASK,			     "GIC_FAULT_INT"			    },
	{ GIC_ERR_INT_MASK,			     "GIC_ERR_INT"			    },
	{ O_DFI_INTERNAL_ERR_INTR_MASK,		     "O_DFI_INTERNAL_ERR_INTR"		    },
	{ O_DFI_PHYUPD_ERR_INTR_MASK,		     "O_DFI_PHYUPD_ERR_INTR"		    },
	{ O_DFI_ALERT_ERR_INTR_MASK,		     "O_DFI_ALERT_ERR_INTR"		    },
	{ O_ECC_AP_ERR_INTR_MASK,		     "O_ECC_AP_ERR_INTR"		    },
	{ O_ECC_AP_ERR_INTR_FAULT_MASK,		     "O_ECC_AP_ERR_INTR_FAULT"		    },
	{ O_ECC_UNCORRECTED_ERR_INTR_MASK,	     "O_ECC_UNCORRECTED_ERR_INTR"	    },
	{ O_ECC_UNCORRECTED_ERR_INTR_FAULT_MASK,     "O_ECC_UNCORRECTED_ERR_INTR_FAULT"	    },
	{ O_DWC_DDRPHY_INT_N_MASK,		     "O_DWC_DDRPHY_INT_N"		    },
	{ NFAULTIRQ_0_MASK,			     "NFAULTIRQ_0"			    },
	{ NFAULTIRQ_1_MASK,			     "NFAULTIRQ_1"			    },
	{ NFAULTIRQ_2_MASK,			     "NFAULTIRQ_2"			    },
	{ NFAULTIRQ_3_MASK,			     "NFAULTIRQ_3"			    },
	{ NFAULTIRQ_4_MASK,			     "NFAULTIRQ_4"			    },
	{ NERRIRQ_0_MASK,			     "NERRIRQ_0"			    },
	{ NERRIRQ_1_MASK,			     "NERRIRQ_1"			    },
	{ NERRIRQ_2_MASK,			     "NERRIRQ_2"			    },
	{ NERRIRQ_3_MASK,			     "NERRIRQ_3"			    },
	{ NERRIRQ_4_MASK,			     "NERRIRQ_4"			    },
	{ GPINT_INTERRUPT_SECONDARY_TO_PRIMARY_MASK, "GPINT_INTERRUPT_SECONDARY_TO_PRIMARY" },
	{ C2C_PINT_OUT_MASK,			     "C2C_PINT_OUT"			    },
	{ TX3_NPD_ARM_IRQ_PIPED_8_MASK,		     "TX3_NPD_ARM_IRQ_PIPED_8"		    },
	{ TX2_NPD_ARM_IRQ_PIPED_8_MASK,		     "TX2_NPD_ARM_IRQ_PIPED_8"		    },
	{ TX1_NPD_ARM_IRQ_PIPED_8_MASK,		     "TX1_NPD_ARM_IRQ_PIPED_8"		    },
	{ TX0_NPD_ARM_IRQ_PIPED_8_MASK,		     "TX0_NPD_ARM_IRQ_PIPED_8"		    },
	{ O_STREAM_PROC_ERROR_MASK,		     "O_STREAM_PROC_ERROR"		    },
	{ ORX0_ARM_IRQ_PIPED_8_MASK,		     "ORX0_ARM_IRQ_PIPED_8"		    },
	{ TX3_ARM_IRQ_PIPED_8_MASK,		     "TX3_ARM_IRQ_PIPED_8"		    },
	{ TX2_ARM_IRQ_PIPED_8_MASK,		     "TX2_ARM_IRQ_PIPED_8"		    },
	{ TX1_ARM_IRQ_PIPED_8_MASK,		     "TX1_ARM_IRQ_PIPED_8"		    },
	{ TX0_ARM_IRQ_PIPED_8_MASK,		     "TX0_ARM_IRQ_PIPED_8"		    },
	{ RX3_ARM_IRQ_PIPED_8_MASK,		     "RX3_ARM_IRQ_PIPED_8"		    },
	{ RX2_ARM_IRQ_PIPED_8_MASK,		     "RX2_ARM_IRQ_PIPED_8"		    },
	{ RX1_ARM_IRQ_PIPED_8_MASK,		     "RX1_ARM_IRQ_PIPED_8"		    },
	{ RX0_ARM_IRQ_PIPED_8_MASK,		     "RX0_ARM_IRQ_PIPED_8"		    }
};

gpint_status_t gpint_status_upper[] = {
	{ L4_ECC_WRN_INTR_0_MASK,		  "L4_ECC_WRN_INTR_0"		      },
	{ L4_ECC_ERR_INTR_0_MASK,		  "L4_ECC_ERR_INTR_0"		      },
	{ L4_ECC_WRN_INTR_1_MASK,		  "L4_ECC_WRN_INTR_1"		      },
	{ L4_ECC_ERR_INTR_1_MASK,		  "L4_ECC_ERR_INTR_1"		      },
	{ L4_ECC_WRN_INTR_2_MASK,		  "L4_ECC_WRN_INTR_2"		      },
	{ L4_ECC_ERR_INTR_2_MASK,		  "L4_ECC_ERR_INTR_2"		      },
	{ EAST_RFPLL_PLL_LOCKED_SYNC_MASK,	  "EAST_RFPLL_PLL_LOCKED_SYNC"	      },
	{ WEST_RFPLL_PLL_LOCKED_SYNC_MASK,	  "WEST_RFPLL_PLL_LOCKED_SYNC"	      },
	{ CLKPLL_PLL_LOCKED_SYNC_MASK,		  "CLKPLL_PLL_LOCKED_SYNC"	      },
	{ TE_FAULT_GP_INTR_PIPED_MASK,		  "TE_FAULT_GP_INTR_PIPED"	      },
	{ CLK_PLL_CP_OVER_RANGE_MASK,		  "CLK_PLL_CP_OVER_RANGE"	      },
	{ ETHPLL_LOCKED_SYNC_MASK,		  "ETHPLL_LOCKED_SYNC"		      },
	{ ARM0_MEMORY_ECC_ERROR_MASK,		  "ARM0_MEMORY_ECC_ERROR"	      },
	{ ARM1_MEMORY_ECC_ERROR_MASK,		  "ARM1_MEMORY_ECC_ERROR"	      },
	{ NVMB_ERR_FLAG_BOOT_MASK,		  "NVMB_ERR_FLAG_BOOT"		      },
	{ NVMB_ERR_FLAG_TIMEOUT_MASK,		  "NVMB_ERR_FLAG_TIMEOUT"	      },
	{ ERROR_SPI0_PAGING_MASK,		  "ERROR_SPI0_PAGING"		      },
	{ SOURCE_REDUCER_ERROR_INDICATION_0_MASK, "SOURCE_REDUCER_ERROR_INDICATION_0" },
	{ EAST_RFPLL_CP_OVER_RANGE_MASK,	  "EAST_RFPLL_CP_OVER_RANGE"	      },
	{ WEST_RFPLL_CP_OVER_RANGE_MASK,	  "WEST_RFPLL_CP_OVER_RANGE"	      },
	{ SPI0_ABORT_MASK,			  "SPI0_ABORT"			      },
	{ SCAN_MUXED_I_TX3_GP_INTERRUPT_0_MASK,	  "SCAN_MUXED_I_TX3_GP_INTERRUPT_0"   },
	{ SCAN_MUXED_I_TX3_GP_INTERRUPT_1_MASK,	  "SCAN_MUXED_I_TX3_GP_INTERRUPT_1"   },
	{ SCAN_MUXED_I_TX2_GP_INTERRUPT_0_MASK,	  "SCAN_MUXED_I_TX2_GP_INTERRUPT_0"   },
	{ SCAN_MUXED_I_TX2_GP_INTERRUPT_1_MASK,	  "SCAN_MUXED_I_TX2_GP_INTERRUPT_1"   },
	{ SCAN_MUXED_I_TX1_GP_INTERRUPT_0_MASK,	  "SCAN_MUXED_I_TX1_GP_INTERRUPT_0"   },
	{ SCAN_MUXED_I_TX1_GP_INTERRUPT_1_MASK,	  "SCAN_MUXED_I_TX1_GP_INTERRUPT_1"   },
	{ SCAN_MUXED_I_TX0_GP_INTERRUPT_0_MASK,	  "SCAN_MUXED_I_TX0_GP_INTERRUPT_0"   },
	{ SCAN_MUXED_I_TX0_GP_INTERRUPT_1_MASK,	  "SCAN_MUXED_I_TX0_GP_INTERRUPT_1"   },
	{ SPI_REG_ARM0_FORCE_GP_INTERRUPT_MASK,	  "SPI_REG_ARM0_FORCE_GP_INTERRUPT"   },
	{ SPI_REG_ARM0_ERROR_MASK,		  "SPI_REG_ARM0_ERROR"		      },
	{ SPI_REG_ARM0_CALIBRATION_ERROR_MASK,	  "SPI_REG_ARM0_CALIBRATION_ERROR"    },
	{ SPI_REG_ARM0_SYSTEM_ERROR_MASK,	  "SPI_REG_ARM0_SYSTEM_ERROR"	      },
	{ SPI_REG_ARM1_FORCE_GP_INTERRUPT_MASK,	  "SPI_REG_ARM1_FORCE_GP_INTERRUPT"   },
	{ SPI_REG_ARM1_ERROR_MASK,		  "SPI_REG_ARM1_ERROR"		      },
	{ SPI_REG_ARM1_CALIBRATION_ERROR_MASK,	  "SPI_REG_ARM1_CALIBRATION_ERROR"    },
	{ SPI_REG_ARM1_SYSTEM_ERROR_MASK,	  "SPI_REG_ARM1_SYSTEM_ERROR"	      },
	{ RADIO_SQR_ERROR_MASK,			  "RADIO_SQR_ERROR"		      },
	{ SW_PINT_0_MASK,			  "SW_PINT_0"			      },
	{ SW_PINT_1_MASK,			  "SW_PINT_1"			      },
	{ SW_PINT_2_MASK,			  "SW_PINT_2"			      },
	{ SW_PINT_3_MASK,			  "SW_PINT_3"			      },
	{ SW_PINT_4_MASK,			  "SW_PINT_4"			      },
	{ SW_PINT_5_MASK,			  "SW_PINT_5"			      },
	{ SW_PINT_6_MASK,			  "SW_PINT_6"			      },
	{ SW_PINT_7_MASK,			  "SW_PINT_7"			      }
};

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

/* Takes the current status bits for upper and lower words and returns status removing non enabled GPINT sources */
void adrv906x_gpint_get_masked_status(uintptr_t gpint_base_addr, struct gpint_settings *settings, uint32_t gpint)
{
	uint8_t byte_current;
	int byte;

	settings->lower_word = 0;
	settings->upper_word = 0;

	/* Read GPINT status byte by byte and store into corresponding upper and lower words */
	for (byte = 0; byte < GPINT_BYTES; byte++) {
		byte_current = mmio_read_8(gpint_base_addr + gpint_status_offset[byte]);
		byte_current &= ~(mmio_read_8(gpint_base_addr + gpint_mask_offset[byte][gpint]));

		if (byte < NUM_BYTES_PER_WORD)
			settings->lower_word |= (((uint64_t)byte_current) << (BYTE_SIZE * byte));
		else
			settings->upper_word |= (((uint64_t)byte_current) << (BYTE_SIZE * (byte - NUM_BYTES_PER_WORD)));
	}
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

/* Print out which pin interrupts were detected */
void adrv906x_gpint_print_status(const struct gpint_settings *settings)
{
	for (int i = 0; i < sizeof(gpint_status_lower) / sizeof(gpint_status_lower[0]); i++)
		if (settings->lower_word & gpint_status_lower[i].mask)
			NOTICE("Pin Interrupt Detected: %s\n", gpint_status_lower[i].name);
	for (int i = 0; i < sizeof(gpint_status_upper) / sizeof(gpint_status_upper[0]); i++)
		if (settings->upper_word & gpint_status_upper[i].mask)
			NOTICE("Pin Interrupt Detected: %s\n", gpint_status_upper[i].name);
}
