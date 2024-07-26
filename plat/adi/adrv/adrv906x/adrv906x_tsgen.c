/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>

#include <adrv906x_tsgen.h>
#include <adrv906x_tsgen_def.h>

void plat_tsgen_enable(const uint64_t base)
{
	mmio_setbits_32(base + TSGEN_CNTCR_OFFSET, TSGEN_CNTEN_MASK);
}

void plat_tsgen_disable(const uint64_t base)
{
	mmio_clrbits_32(base + TSGEN_CNTCR_OFFSET, TSGEN_CNTEN_MASK);
}
