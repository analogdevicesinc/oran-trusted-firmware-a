/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_BOARD_H
#define ADRV906X_BOARD_H

/* Include board-specific interfaces required by the common layer */
#include <plat_board.h>

/* Enable external sysref signal from clock chip.
 * Returns true if successfully enabled
 */
bool plat_sysref_enable(void);

/* Disable external sysref signal from clock chip */
void plat_sysref_disable(void);

/* Initialize external clock chip and enable 245.76MHz device clock signal.
 * Returns true on success
 */
bool plat_clkdev_init(void);

/* Perform board-specific SPI NOR flash reset */
void plat_do_spi_nor_reset(void);

/* Perform board-specific setup */
void plat_board_bl1_setup(void);
void plat_board_bl2_setup(void);
void plat_board_bl31_setup(void);

#endif /* ADRV906X_BOARD_H */
