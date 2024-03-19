/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADRV906X_TSGEN_H
#define ADRV906X_TSGEN_H

/* enable TSGEN */
void plat_tsgen_enable(const uint64_t base);

/* disable TSGEN */
void plat_tsgen_disable(const uint64_t base);

#endif /* ADRV906X_TSGEN_H */
