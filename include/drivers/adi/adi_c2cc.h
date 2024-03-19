/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_C2CC_H
#define ADI_C2CC_H

#include <stdint.h>
#include <stdbool.h>

struct adi_c2cc_training_clock_settings {
	uint8_t rosc_div;
	uint8_t devclk_div;
	uint8_t pll_div;
};

struct adi_c2cc_training_generator_settings {
	uint32_t poly; /* golden polynomial with maximum length sequence */
	uint32_t seed;
};

struct adi_c2cc_training_delay_settings {
	uint8_t resp2cmd;       /* tx trainer */
	uint8_t cmd2txclksw;    /* tx trainer */
	uint8_t txclksw2ptrn;   /* tx trainer */
	uint8_t txptrndur;      /* tx trainer */
	uint8_t swbckclk;       /* tx trainer */
	uint8_t caldone;        /* tx trainer */
	uint8_t rxcmd2trntrm;   /* rx trainee */
	uint8_t rxcmd2ld;       /* rx trainee */
	uint8_t lddur;          /* rx trainee */
	uint8_t mtchdur;        /* rx trainee */
	uint8_t swbcktrm;       /* rx trainee */
	uint8_t blktrf;         /* rx trainee */
};

struct adi_c2cc_training_settings {
	struct adi_c2cc_training_clock_settings tx_clk;
	struct adi_c2cc_training_generator_settings generator;
	struct adi_c2cc_training_delay_settings p2s_delay;      /* primary-to-secondary */
	struct adi_c2cc_training_delay_settings s2p_delay;      /* secondary-to-primary */
};

void adi_c2cc_init(uintptr_t pri_base, uintptr_t sec_base);
bool adi_c2cc_enable(void);
bool adi_c2cc_enable_high_speed(struct adi_c2cc_training_settings *params);

#endif /* ADI_C2CC_H */
