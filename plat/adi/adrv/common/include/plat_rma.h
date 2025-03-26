/*
 * Copyright (c) 2025, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_RMA_H
#define PLAT_RMA_H

int plat_rma_function(uint8_t *command_buffer, bool help);
int plat_secure_debug_access_function(uint8_t *command_buffer, bool help);
int plat_challenge_request_function(uint8_t *command_buffer, bool help);
int plat_prepare_mailboxes_function(uint8_t *command_buffer, bool help);

int plat_prepare_rma_mailboxes(bool secondary);
int plat_rma_challenge_request(chal_type_e chal, bool secondary);
int plat_rma_perform_rma(uint8_t *response_buf, uint32_t size, bool secondary);
int plat_rma_secure_debug_access(uint8_t *response_buf, uint32_t size, bool secondary);


#endif /* PLAT_RMA_H */
