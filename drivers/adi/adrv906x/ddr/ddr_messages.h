/*
 * Copyright(c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDR_MESSAGES_H
#define DDR_MESSAGES_H

#include <stdint.h>

typedef struct {
	uint32_t id;
	char *message;
} ddr_message_t;

#define ONED_TRAINING_MESSAGE_STRING_COUNT 23
#define TWOD_TRAINING_MESSAGE_STRING_COUNT 16
extern const ddr_message_t ddr_1d_log_messages[ONED_TRAINING_MESSAGE_STRING_COUNT];
extern const ddr_message_t ddr_2d_log_messages[TWOD_TRAINING_MESSAGE_STRING_COUNT];
#endif /* DDR_MESSAGES_H */
