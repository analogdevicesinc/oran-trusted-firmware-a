/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_C2CC_H
#define ADI_C2CC_H

#include <stdint.h>
#include <stdbool.h>

#define ADI_C2C_LANE_COUNT 4
#define ADI_C2C_TRIM_MAX 64
#define ADI_C2C_TRIM_DELAY_MAX 16
#define ADI_C2C_MIN_WINDOW_SIZE 5
#define ADI_C2C_MAX_AXI_WAIT 50
#define ADI_C2C_MAX_TRAIN_WAIT 500

#define ADI_C2C_GET_COMBINED_STAT(stats, trim)       (stats)[trim]
#define ADI_C2C_GET_LANE_STAT(stats, trim, lane)     (0xFF & (ADI_C2C_GET_COMBINED_STAT(stats, trim) >> (8 * (lane))))
#define ADI_C2C_GET_LANE_STATS(stats, trim)          { \
		ADI_C2C_GET_LANE_STAT(stats, trim, 0), ADI_C2C_GET_LANE_STAT(stats, trim, 1), \
		ADI_C2C_GET_LANE_STAT(stats, trim, 2), ADI_C2C_GET_LANE_STAT(stats, trim, 3), \
}

typedef enum {
	C2C_MODE_NORMAL,                /* Normal dual-tile operation */
	C2C_MODE_EXTERNAL_LOOPBACK,     /* External loopback mode */
	C2C_MODE_PHYDIG_LOOPBACK        /* Internal phydig loopback mode */
} c2c_mode_t;

typedef enum {
	C2C_DRIVE_LEVEL_0, /* ~700mV signal swing */
	C2C_DRIVE_LEVEL_1,
	C2C_DRIVE_LEVEL_2,
	C2C_DRIVE_LEVEL_3, /* ~1400mV signal swing */
} c2c_drive_level_t;

struct adi_c2cc_training_clock_settings {
	uint8_t rosc_div;
	uint8_t devclk_div;
	uint8_t pll_div;
	c2c_drive_level_t drive_level;
};

struct adi_c2cc_training_generator_settings {
	uint32_t pos_poly; /* golden polynomial with maximum length sequence */
	uint32_t neg_poly;
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

struct adi_c2cc_trim_settings {
	uint8_t p2s_trim;
	uint8_t p2s_trim_delays[ADI_C2C_LANE_COUNT];
	uint8_t s2p_trim;
	uint8_t s2p_trim_delays[ADI_C2C_LANE_COUNT];
};

struct adi_c2cc_calibration_settings {
	uint32_t period;
	uint32_t pattern_period;
	uint32_t pattern_size;
	uint8_t transition_delay;
	uint8_t multisample_delay;
};

void adi_c2cc_init(uintptr_t pri_base, uintptr_t sec_base, c2c_mode_t mode);
bool adi_c2cc_enable(void);
bool adi_c2cc_enable_high_speed(struct adi_c2cc_training_settings *params);
bool adi_c2cc_setup_train(struct adi_c2cc_training_settings *params);
bool adi_c2cc_run_train(uint32_t *p2s_stats, uint32_t *s2p_stats);
bool adi_c2cc_process_train_data_and_apply(size_t min_size, struct adi_c2cc_trim_settings *forced_trim, uint32_t *p2s_stats, uint32_t *s2p_stats, struct adi_c2cc_training_clock_settings *tx_clk);
bool adi_c2cc_run_loopback_test();
bool adi_c2cc_enable_hw_bg_cal(struct adi_c2cc_calibration_settings *params, struct adi_c2cc_training_generator_settings *prbs_params);

#endif /* ADI_C2CC_H */
