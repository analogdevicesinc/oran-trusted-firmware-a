/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>

#include <adrv906x_sram.h>

void l4_warning_info(uintptr_t base_addr)
{
	uint32_t status, offset, mask;
	bool found = false;
	int i;

	status = mmio_read_32(base_addr + L4CTL_CFG_STAT);

	if (status & L4CTL_CFG_STAT_ERR0_MASK)
		NOTICE("Core channel warning info:0x%x\n", mmio_read_32(base_addr + L4CTL_CFG_ET0));

	if (status & L4CTL_CFG_STAT_ERR1_MASK)
		NOTICE("System channel warning info:0x%x\n", mmio_read_32(base_addr + L4CTL_CFG_ET1));

	/* Shift warning status bits to be at the bottom */
	status = status >> L4CTL_CFG_STAT_ECCWRN0_SHIFT;
	if (status) {
		mask = 0x1;
		/* Find the offset of the EWADDR register that holds the corrected error address value */
		for (i = 0; i < L4_SRAM_NUMBER_OF_BANKS; i++) {
			if (mask & status) {
				found = true;
				/* Each register for the EWADDR is 32 bits, so address is base of the warning registers + 4 bytes * bank number */
				offset = L4CTL_CFG_EWADDR0 + (0x4 * i);
				WARN("SRAM Bank%d correctable error, addr: 0x%x\n", i, mmio_read_32(base_addr + offset));
			}
			mask = mask << 1;
		}

		if (!found)
			ERROR("L4 ecc error interrupt triggered but no matching error found.\n");
		else
			/* Clear all of the warning status bits by writing a 1 since we've handled them all. */
			mmio_write_32(base_addr + L4CTL_CFG_STAT, ((L4_SRAM_BANK_CLEAR_MASK) << L4CTL_CFG_STAT_ECCWRN0_SHIFT));
	}
}

void l4_error_info(uintptr_t base_addr)
{
	uint32_t status, offset, mask;
	bool found = false;
	int i;

	status = mmio_read_32(base_addr + L4CTL_CFG_STAT);

	if (status & L4CTL_CFG_STAT_ERR0_MASK)
		ERROR("Core channel error info:0x%x\n", mmio_read_32(base_addr + L4CTL_CFG_ET0));

	if (status & L4CTL_CFG_STAT_ERR1_MASK)
		ERROR("System channel error info:0x%x\n", mmio_read_32(base_addr + L4CTL_CFG_ET1));

	/* Shift warning status bits to be at the bottom */
	status = status >> L4CTL_CFG_STAT_ECCERR0_SHIFT;
	if (status) {
		mask = 0x1;
		/* Find the offset of the ERRADDR register that holds the corrected error address value */
		offset = L4CTL_CFG_ERRADDR0;
		for (i = 0; i < L4_SRAM_NUMBER_OF_BANKS; i++) {
			if (mask & status) {
				found = true;
				/* Each register for the ERRADDR is 32 bits, so increment 4 bytes * bank number */
				offset = L4CTL_CFG_ERRADDR0 + (0x4 * i);
				ERROR("SRAM Bank%d uncorrectable error, addr: 0x%x\n", i, mmio_read_32(base_addr + offset));
			}
			mask = mask << 1;
		}

		if (!found)
			ERROR("L4 ecc error interrupt triggered but no matching error found.\n");
		else
			/* Clear all of the error status bits by writing a 1 since we've handled them all. */
			mmio_write_32(base_addr + L4CTL_CFG_STAT, ((L4_SRAM_BANK_CLEAR_MASK) << L4CTL_CFG_STAT_ECCERR0_SHIFT));
	}
}
