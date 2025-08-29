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
#define ADI_C2C_MAX_TRIM_CENTER 50
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

typedef enum {
	C2C_ERR_ECC_1B = 0,
	C2C_ERR_ECC_1B_COUNT_THRESHOLD,
	C2C_ERR_START_BIT_1_BIT,
	C2C_ERR_START_BIT_1B_COUNT_THRESHOLD,
	C2C_ERR_ECC_2B,
	C2C_ERR_START_BIT_2_BIT,
	C2C_ERR_INT_TX_OL,
	C2C_ERR_INT_RX_OL,
	C2C_ERR_INVALID_HEADER,
	C2C_ERR_TX_INTERRUPT_OL_COUNT_SATURATED,
	C2C_ERR_RX_INTERRUPT_OL_COUNT_SATURATED,
	C2C_ERR_BACKGROUND_CAL_SW_ERR,
	C2C_ERR_BACKGROUND_CAL_SW_WARN,
	C2C_ERR_BACKGROUND_CAL_HW_ERR,
	C2C_ERR_BACKGROUND_CAL_HW_UPD,
	C2C_ERR_PWR_UP_CAL_TX_IRQ,
	C2C_ERR_PWR_UP_CAL_RX_IRQ,
	C2C_ERR_TXN_IN_C2C_DISABLED,
	C2C_ERR_TYPE_MAX /* keep at end of list */
} c2c_err_type_t;

typedef enum {
	C2C_HANDLER_NONE		= 0,
	C2C_HANDLER_NON_CRITICAL_INT	= 1,    /* Lower-priority ISR */
	C2C_HANDLER_CRITICAL_INT	= 2,    /* Higher-priority ISR */
	C2C_HANDLER_PIN_INT		= 4,    /* Routed to external GPIO */
} c2c_err_handler_t;

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
bool adi_c2cc_enable_hw_bg_cal(struct adi_c2cc_calibration_settings *params, struct adi_c2cc_training_generator_settings *prbs_params);

/* used by BL31 */
const char *adi_c2cc_get_err_name(c2c_err_type_t type);
const char *adi_c2cc_get_err_description(c2c_err_type_t type);
bool adi_c2cc_error_handler(c2c_err_handler_t type, uint32_t *errors);
bool adi_c2cc_enable_error_handling(c2c_err_handler_t *params);

/* used by adrv906x_cli.c */
bool adi_c2cc_setup_train(struct adi_c2cc_training_settings *params);
bool adi_c2cc_run_train(uint32_t *p2s_stats, uint32_t *s2p_stats, uint8_t *p2s_delays, uint8_t *s2p_delays);
bool adi_c2cc_analyze_train_data(uint32_t *p2s_stats, uint32_t *s2p_stats, size_t min_size, size_t max_spread, size_t max_center, uint8_t *p2s_trim_delays, uint8_t *s2p_trim_delays, uint8_t *p2s_trim, uint8_t *s2p_trim);
bool adi_c2cc_apply_training(uint8_t p2s_trim, uint8_t s2p_trim, struct adi_c2cc_training_clock_settings *tx_clk);
bool adi_c2cc_run_loopback_test();

#endif /* ADI_C2CC_H */
