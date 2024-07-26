/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_INT_GICV3_H
#define PLAT_INT_GICV3_H

void plat_gic_driver_init(void);
void plat_gic_init(void);
void plat_gic_cpuif_enable(void);
void plat_gic_cpuif_disable(void);
void plat_gic_redistif_on(void);
void plat_gic_redistif_off(void);
void plat_gic_pcpu_init(void);
int plat_specific_gic_driver_init(gicv3_driver_data_t *plat_gic_data);

#endif /* PLAT_INT_GICV3_H */
