/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_RUNTIME_LOG_H
#define PLAT_RUNTIME_LOG_H

void write_to_runtime_buffer(const char *message);
void read_from_runtime_buffer(char *message, int size);

#endif /* PLAT_RUNTIME_LOG_H */
