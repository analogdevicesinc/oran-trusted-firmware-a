/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_C2CC_ANALYSIS_H
#define ADI_C2CC_ANALYSIS_H

uint8_t adi_c2cc_find_optimal_trim(uint32_t *stats, size_t min_size, uint8_t *trim_delays, size_t *eye_width);

#endif /* ADI_C2CC_ANALYSIS_H */
