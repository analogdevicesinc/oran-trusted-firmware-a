/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_C2CC_TRAINING_H
#define ADI_C2CC_TRAINING_H

bool adi_c2cc_setup_train(struct adi_c2cc_training_settings *params);
bool adi_c2cc_run_train(uint32_t *p2s_stats, uint32_t *s2p_stats);
bool adi_c2cc_process_train_data_and_apply(
	size_t min_size, struct adi_c2cc_trim_settings *forced_trim,
	uint32_t *p2s_stats, uint32_t *s2p_stats,
	struct adi_c2cc_training_clock_settings *tx_clk);
bool adi_c2cc_run_loopback_test();

#endif /* ADI_C2CC_TRAINING_H */
