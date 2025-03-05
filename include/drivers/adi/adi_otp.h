/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_OTP_H
#define ADI_OTP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define OTP_ECC_ON       0
#define OTP_ECC_OFF      1

#define ADI_OTP_SUCCESS         0

#define OTP_REGISTER_SIZE       4 // 32 Bits

struct adi_otp_dap_settings {
	uint32_t SBPI_DAP_CQ;
	uint32_t RQ_CQ_DAP_PROG_2;
	uint32_t RQ_CQ_DAP_PROG_1;
	uint32_t RQ_CQ_DAP_PROG_0;
};

struct adi_otp_pmc_settings {
	uint32_t SBPI_PMC_CQ;
	uint32_t RQ_CQ_PMC_PROG_2;
	uint32_t RQ_CQ_PMC_PROG_1;
	uint32_t RQ_CQ_PMC_PROG_0;
};

void otp_init_driver(struct adi_otp_dap_settings prog_dap_settings, struct adi_otp_pmc_settings prog_pmc_settings, struct adi_otp_dap_settings read_dap_settings, struct adi_otp_pmc_settings read_pmc_settings);
int otp_read(const uintptr_t mem_ctrl_base, const uintptr_t addr, uint32_t *data, uint8_t ecc_state);
int otp_read_burst(const uintptr_t mem_ctrl_base, const uintptr_t addr, uint32_t *buffer, size_t len, uint8_t ecc_state);
int otp_write(const uintptr_t mem_ctrl_base, const uintptr_t addr, uint32_t data, uint8_t ecc_state);
int otp_write_burst(const uintptr_t mem_ctrl_base, const uintptr_t addr, const uint32_t *buffer, size_t len, uint8_t ecc_state);

#endif /* ADI_OTP_H */
