/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/interrupt_props.h>
#include <drivers/arm/gicv3.h>
#include <lib/utils.h>
#include <plat/common/platform.h>

#include <adrv906x_device_profile.h>
#include <plat_err.h>
#include <plat_int_gicv3.h>
#include <platform_def.h>

static interrupt_prop_t adrv906x_interrupt_props[] = {
	PLAT_IRQ_PROPS                          // Common configuration
	PLAT_CONFIGURABLE_IRQ_PROPS             // Room for platform specific configuration
};

/* This function configures the GIC lanes routing for the adrv906x platform.
 * It includes the common configuration for all platforms defined at build time
 * (PLAT_IRQ_PROPS), and platform-specific configuration dynamically built in
 * runtime.
 * Note: 'adrv906x_interrupt_props' shall include only the lanes routed to the
 * secure world
 *
 * Plaform-specific configuration is based on the secure state of the exposed
 * peripherls. All the lanes belonging to secure exposed secure peripherals will
 * be configured in this list to be routed to the group1S (op-tee).
 */
int plat_specific_gic_driver_init(gicv3_driver_data_t *plat_gic_data)
{
	uint32_t len;
	bool *secure_peripherals;
	unsigned int props_size;
	int periph_gic_num[][6] = {
		{ IRQ_PL011_UART_INTR_1,	 0,				0,			0,			0,			 0			  },
		{ IRQ_PL011_UART_INTR_2,	 0,				0,			0,			0,			 0			  },
		{ IRQ_PL011_UART_INTR_3,	 0,				0,			0,			0,			 0			  },
		{ IRQ_IRQ_SPI_1_RX_DDEERR_PIPED, IRQ_IRQ_SPI_1_TX_DDEERR_PIPED, IRQ_IRQ_SPI_1_TX_PIPED, IRQ_IRQ_SPI_1_RX_PIPED, IRQ_IRQ_SPI_1_ERR_PIPED, IRQ_IRQ_SPI_1_STAT_PIPED },
		{ IRQ_IRQ_SPI_2_RX_DDEERR_PIPED, IRQ_IRQ_SPI_2_TX_DDEERR_PIPED, IRQ_IRQ_SPI_2_TX_PIPED, IRQ_IRQ_SPI_2_RX_PIPED, IRQ_IRQ_SPI_2_ERR_PIPED, IRQ_IRQ_SPI_2_STAT_PIPED },
		{ IRQ_IRQ_SPI_3_RX_DDEERR_PIPED, IRQ_IRQ_SPI_3_TX_DDEERR_PIPED, IRQ_IRQ_SPI_3_TX_PIPED, IRQ_IRQ_SPI_3_RX_PIPED, IRQ_IRQ_SPI_3_ERR_PIPED, IRQ_IRQ_SPI_3_STAT_PIPED },
		{ IRQ_IRQ_SPI_4_RX_DDEERR_PIPED, IRQ_IRQ_SPI_4_TX_DDEERR_PIPED, IRQ_IRQ_SPI_4_TX_PIPED, IRQ_IRQ_SPI_4_RX_PIPED, IRQ_IRQ_SPI_4_ERR_PIPED, IRQ_IRQ_SPI_4_STAT_PIPED },
		{ IRQ_IRQ_SPI_5_RX_DDEERR_PIPED, IRQ_IRQ_SPI_5_TX_DDEERR_PIPED, IRQ_IRQ_SPI_5_TX_PIPED, IRQ_IRQ_SPI_5_RX_PIPED, IRQ_IRQ_SPI_5_ERR_PIPED, IRQ_IRQ_SPI_5_STAT_PIPED },
		{ IRQ_IRQ_SPI_6_RX_DDEERR_PIPED, IRQ_IRQ_SPI_6_TX_DDEERR_PIPED, IRQ_IRQ_SPI_6_TX_PIPED, IRQ_IRQ_SPI_6_RX_PIPED, IRQ_IRQ_SPI_6_ERR_PIPED, IRQ_IRQ_SPI_6_STAT_PIPED },
		{ IRQ_I2C_IRQ_S2F_PIPED_0,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_1,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_2,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_3,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_4,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_5,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_6,	 0,				0,			0,			0,			 0			  },
		{ IRQ_I2C_IRQ_S2F_PIPED_7,	 0,				0,			0,			0,			 0			  }
	};

	secure_peripherals = plat_get_secure_peripherals(&len);

	/* Sanity checks */
	if (secure_peripherals == NULL) {
		plat_error_message("Invalid pointer to the secure_peripheral list");
		return -1;
	}

	if (len != ARRAY_SIZE(periph_gic_num)) {
		plat_error_message("Missmatch in secure periherpal list size");
		return -1;
	}

	props_size = plat_gic_data->interrupt_props_num;

	for (uint32_t i = 0; i < len; i++) {
		if (secure_peripherals[i]) {
			for (uint32_t j = 0; j < ARRAY_SIZE(periph_gic_num[0]); j++) {
				if (periph_gic_num[i][j] != 0) {
					if (props_size > ARRAY_SIZE(adrv906x_interrupt_props)) {
						plat_error_message("Too many interrupts. User GIC configuration ignored");
						return -1;
					}

					adrv906x_interrupt_props[props_size].intr_num = periph_gic_num[i][j];
					adrv906x_interrupt_props[props_size].intr_pri = PLAT_IRQ_NORMAL_PRIORITY;
					adrv906x_interrupt_props[props_size].intr_grp = INTR_GROUP1S;
					adrv906x_interrupt_props[props_size].intr_cfg = GIC_INTR_CFG_LEVEL;
					props_size++;

					INFO("GIC lane %d routed to secure-world\n", periph_gic_num[i][j]);
				}
			}
		}
	}

	/* Update GIC lanes configuration */
	plat_gic_data->interrupt_props = adrv906x_interrupt_props;
	plat_gic_data->interrupt_props_num = props_size;

	return 0;
}
