/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_SPU_H
#define ADI_SPU_H

#include <stdint.h>

// each peripheral's write-protect register is 32 bits wide
#define ADI_SPU_MAX_MASTERS 32

void adi_spu_disable_ssec(uintptr_t base, unsigned int peripheral_id);
void adi_spu_enable_ssec(uintptr_t base, unsigned int peripheral_id);
void adi_spu_disable_msec(uintptr_t base, unsigned int peripheral_id);
void adi_spu_enable_msec(uintptr_t base, unsigned int peripheral_id);
void adi_spu_enable_write_protect(uintptr_t base, unsigned int peripheral_id, uint32_t master_id);
void adi_spu_disable_write_protect(uintptr_t base, unsigned int peripheral_id, uint32_t master_id);
void adi_spu_init(uintptr_t base, unsigned int peripherals_count);

#endif /* ADI_SPU_H */
