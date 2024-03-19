/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <drivers/delay_timer.h>
#include <drivers/adi/adi_otp.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include "adi_otp.h"

/*--------------------------------------------------------
 * DEFINES
 *------------------------------------------------------*/

#define DEFAULT_TIMEOUT_US              3U
#define DISABLE_IF_TIMEOUT_US           DEFAULT_TIMEOUT_US
#define WR_DAP_RQ_REQ_TIMEOUT_US        DEFAULT_TIMEOUT_US
#define RD_REQ_TIMEOUT_US               DEFAULT_TIMEOUT_US
#define RD_DATA_VALID_TIMEOUT_US        DEFAULT_TIMEOUT_US
#define WR_DAP_RQ_CQ_REQ_TIMEOUT_US     5U
#define WR_PMC_RQ_CQ_REQ_TIMEOUT_US     5U
#define WR_PMC_CC_REQ_TIMEOUT_US        3U
#define PGM_REQ_TIMEOUT_US              (10U * 1000U)

/* Redundancy modes */
#define REDN_SE                         0x00
#define REDN_R                          0x01
#define REDN_D                          0x10
#define REDN_DR                         0x11
#define REDN_MODE                       REDN_R

/* Default DAP/PMC settings */
/* Below values are taken from SBPI_TB__CQ_RQ_define.v */
/* RQ_CQ_DAP_PROG = 112'h 0000_0000_0008_ee00_c000_0083_5d2a */
#define DEFAULT_SBPI_DAP_CQ             0x0000
#define DEFAULT_RQ_CQ_DAP_PROG_2        0x00000008
#define DEFAULT_RQ_CQ_DAP_PROG_1        0xee00c000
#define DEFAULT_RQ_CQ_DAP_PROG_0        0x00835d2a
/* RQ_CQ_PMC_PROG = 112'h 4000_028c_1111_5f0a_5f0f_df16_5f2a */
#define DEFAULT_SBPI_PMC_CQ             0x4000
#define DEFAULT_RQ_CQ_PMC_PROG_2        0x028c1111
#define DEFAULT_RQ_CQ_PMC_PROG_1        0x5f0a5f0f
#define DEFAULT_RQ_CQ_PMC_PROG_0        0xdf165f2a

/* ECC */
#define ECC_TYPE_ADI_SECDED             0
#define ECC_TYPE_SYNOPSIS_SECSED        1
#define ADI_OTP_ECC_TYPE                ECC_TYPE_SYNOPSIS_SECSED

/* Program status PGM_STATUS_n*/
/* With ECC: 0x40 -> status ok*/
#define PGM_WITH_ECC_STATUS_OK          0x40
/* Without ECC: bits 7:6: 01 -> operation done */
#define PGM_WITHOUT_ECC_STATUS_BITP     6
#define PGM_WITHOUT_ECC_STATUS_MASK     0xC0
#define PGM_WITHOUT_ECC_STATUS_OK       1

#define READ_BF_32BIT_REG read32bf
#define WRITE_BF_32BIT_REG write32bf

/* Debug */
/* #define OTP_DEBUG_ENABLE */
#ifdef OTP_DEBUG_ENABLE
#define OTP_DEBUG(...)          printf(__VA_ARGS__)
#else
#define OTP_DEBUG(...)
#endif

/*--------------------------------------------------------
 * GLOBALS
 *------------------------------------------------------*/

static uint32_t SBPI_DAP_CQ = DEFAULT_SBPI_DAP_CQ;
static uint32_t RQ_CQ_DAP_PROG_2 = DEFAULT_RQ_CQ_DAP_PROG_2;
static uint32_t RQ_CQ_DAP_PROG_1 = DEFAULT_RQ_CQ_DAP_PROG_1;
static uint32_t RQ_CQ_DAP_PROG_0 = DEFAULT_RQ_CQ_DAP_PROG_0;

static uint32_t SBPI_PMC_CQ = DEFAULT_SBPI_PMC_CQ;
static uint32_t RQ_CQ_PMC_PROG_2 = DEFAULT_RQ_CQ_PMC_PROG_2;
static uint32_t RQ_CQ_PMC_PROG_1 = DEFAULT_RQ_CQ_PMC_PROG_1;
static uint32_t RQ_CQ_PMC_PROG_0 = DEFAULT_RQ_CQ_PMC_PROG_0;

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS PROTOTYPES
 *------------------------------------------------------*/

static uint32_t read32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask);
static void write32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask, uint32_t val);

static void set_WR_PMC_RQ_CQ_request(const uintptr_t base);
static int wait_for_OTP_read_request_clear(const uintptr_t base, uint64_t timeout_us);
static int wait_for_OTP_program_request_clear(const uintptr_t base, uint64_t timeout_us);
static int wait_for_WR_DAP_RQ_request_clear(const uintptr_t base, uint64_t timeout_us);
static int wait_for_WR_DAP_RQ_CQ_request_clear(const uintptr_t base, uint64_t timeout_us);
static int wait_for_WR_PMC_RQ_CQ_request_clear(const uintptr_t base, uint64_t timeout_us);
static int wait_for_WR_PMC_CC_request_clear(const uintptr_t base, uint64_t timeout_us);
static int wait_for_status_idle(const uintptr_t base, uint64_t timeout_us);
static int program_without_ECC_done(const uintptr_t base);

static int op_disable_interface(const uintptr_t base);

/*
 *  NOTE:
 *  The following operation functions implement the operations sequences stated in the "ADI OPT RD WR Operation Sequence.xlsx",
 *  that can be located at https://confluence.analog.com/display/A242092596/Adrv906x+OTP+and+Related+Designs
 */
static int op_read_setup(const uintptr_t base, uint8_t ecc_state);
static int op_read(const uintptr_t base, const uintptr_t addr, uint32_t *value, uint8_t ecc_state);
static int op_program_setup(const uintptr_t base, uint8_t ecc_state);
static int op_program(const uintptr_t base, const uintptr_t addr, uint32_t value, uint8_t ecc_state);

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS
 *------------------------------------------------------*/
static uint32_t read32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask)
{
	return (mmio_read_32((uintptr_t)addr) & mask) >> position;
}

static void write32bf(volatile uint32_t *addr, uint8_t position, uint32_t mask, uint32_t val)
{
	uint32_t reg = mmio_read_32((uintptr_t)addr) & ~mask;

	reg |= (val << position) & mask;
	mmio_write_32((uintptr_t)addr, reg);
}

static void set_WR_PMC_RQ_CQ_request(const uintptr_t base)
{
	uint32_t value;

	value = *pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_PMC_REQ(base);
	value |= BITM_MEM_CTRL_REGMAP_MC_CMD_WR_PMC_RQ_REQ | BITM_MEM_CTRL_REGMAP_MC_CMD_WR_PMC_CQ_REQ;
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_PMC_REQ(base) = value;
}

static int wait_for_OTP_read_request_clear(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_OTP_RD_REQ(base))
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int wait_for_OTP_program_request_clear(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_PGM_REQ(base))
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int wait_for_WR_DAP_RQ_request_clear(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_RQ_REQ(base))
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int wait_for_WR_DAP_RQ_CQ_request_clear(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_RQ_REQ(base) || READ_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_CQ_REQ(base))
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int wait_for_WR_PMC_RQ_CQ_request_clear(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_WR_PMC_RQ_REQ(base) || READ_MEM_CTRL_REGMAP_MC_CMD_WR_PMC_CQ_REQ(base))
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int wait_for_WR_PMC_CC_request_clear(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_WR_PMC_CC_REQ(base))
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int wait_for_status_idle(const uintptr_t base, uint64_t timeout_us)
{
	uint64_t timeout = timeout_init_us(timeout_us);

	while (READ_MEM_CTRL_REGMAP_MC_CMD_STATUS_IDLE(base) != 1)
		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;

	return ADI_OTP_SUCCESS;
}

static int program_without_ECC_done(const uintptr_t base)
{
	return (READ_MEM_CTRL_REGMAP_MC_CMD_PGM_STATUS_0(base) & PGM_WITHOUT_ECC_STATUS_MASK) >> PGM_WITHOUT_ECC_STATUS_BITP == PGM_WITHOUT_ECC_STATUS_OK;
}

static int op_disable_interface(const uintptr_t base)
{
	/* Disable */
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_EN_CTRL(base) = 0;

	/* Wait status idle */
	if (wait_for_status_idle(base, DISABLE_IF_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s:  Idle status timeout\n", __func__);
		return -ETIMEDOUT;
	}
	return ADI_OTP_SUCCESS;
}

static int op_read_setup(const uintptr_t base, uint8_t ecc_state)
{
	OTP_DEBUG("op_read_setup...\n");

	/* Disable interface if USER is enabled */
	if (READ_MEM_CTRL_REGMAP_MC_CMD_EN_USER(base)) {
		int ret = op_disable_interface(base);
		if (ret != ADI_OTP_SUCCESS) return ret;
	}

	/* 1	Enable values from QSR/QRR to be used for DAP RQ regs and keep SBPI interface enabled */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_EN_SBPI(base, 1);
	WRITE_MEM_CTRL_REGMAP_MC_LOAD_QSR_QRR(base, 1);

	/* 2	Load DAP RQ with MR/MRR settings from BOOT */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_RQ_REQ(base, 1);

	/* 3	Wait for self-clear on request */
	if (wait_for_WR_DAP_RQ_request_clear(base, WR_DAP_RQ_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s: Load DAP RQ timeout\n", __func__);
		return -ETIMEDOUT;
	}

	if (ecc_state == OTP_ECC_OFF) {
		/* 4	Disable ECC and BRP in DAP Registers */
		/* 4.1	ECC and BRP disabled. DAP RQ[80]  => ECC disable;   DAP RQ[83] => BRP disable */
		WRITE_MEM_CTRL_REGMAP_MC_SBPI_DAP_RQ_REGS_10(base, 9);
		/* 4.2	Write DAP RQ values into Sidense Controller */
		WRITE_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_RQ_REQ(base, 1);
		/* 4.3	Wait for self-clear on request */
		if (wait_for_WR_DAP_RQ_request_clear(base, WR_DAP_RQ_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
			ERROR("%s: Disable ECC: Load DAP RQ timeout\n", __func__);
			return -ETIMEDOUT;
		}
	}

	/* Disable interface if SBPI is enabled */
	if (READ_MEM_CTRL_REGMAP_MC_CMD_EN_SBPI(base)) {
		int ret = op_disable_interface(base);
		if (ret != ADI_OTP_SUCCESS) return ret;
	}

	/* 4	Enable USER interface and use QSR/QRR in mem_ctrl */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_EN_USER(base, 1);
	WRITE_MEM_CTRL_REGMAP_MC_LOAD_QSR_QRR(base, 1);

	/* 5	Redn mode to be used: 00=SE, 01=R, 10=D, 11=DR */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_REDN(base, REDN_MODE);

	/* 6	ecc bypass: Value set to 0 if ADI ECC engine is used for NVM reads */
#if (ADI_OTP_ECC_TYPE == ECC_TYPE_ADI_SECDED)
	WRITE_MEM_CTRL_REGMAP_MC_OTP_RD_ECC_BYPASS(base, 0);
#elif (ADI_OTP_ECC_TYPE == ECC_TYPE_SYNOPSIS_SECSED)
	WRITE_MEM_CTRL_REGMAP_MC_OTP_RD_ECC_BYPASS(base, 1);
#endif

	/* 7	Controls for ECC engine for NVM reads (ecc_disable, ecc_gen, ecc_test) */
	WRITE_MEM_CTRL_REGMAP_MC_OTP_RD_ECC_DISABLE(base, 0);
	WRITE_MEM_CTRL_REGMAP_MC_OTP_RD_ECC_GEN(base, 0);
	WRITE_MEM_CTRL_REGMAP_MC_OTP_RD_ECC_TEST(base, 0);

	if (ecc_state == OTP_ECC_OFF)
		/* 8	ECC disable*/
		WRITE_MEM_CTRL_REGMAP_MC_OTP_RD_ECC_DISABLE(base, 1);

	return ADI_OTP_SUCCESS;
}

static int op_read(const uintptr_t base, const uintptr_t addr, uint32_t *value, uint8_t ecc_state)
{
	OTP_DEBUG("op_read...\n");

	/* 1	Read address */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_ADDR(base, addr);

	/* 4	OTP Read request */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_OTP_RD_REQ(base, 1);

	/* 5	Wait for self-clear on request */
	if (wait_for_OTP_read_request_clear(base, RD_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s: Read request timeout\n", __func__);
		return -ETIMEDOUT;
	}

	/* 6	Read data */
	*value = READ_MEM_CTRL_REGMAP_MC_OTP_RD_DATA(base);

	if (ecc_state == OTP_ECC_ON) {
#if (ADI_OTP_ECC_TYPE == ECC_TYPE_ADI_SECDED)
		/* 7	Read ECC if needed */
		uint32_t ecc;
		ecc = READ_MEM_CTRL_REGMAP_MC_OTP_RD_ECC(base);

		/* 8	Read ECC engine flags. Not valid when Sidense ECC is used */
		uint32_t no_err_flag, sec_flag, ded_flag;
		no_err_flag = READ_MEM_CTRL_REGMAP_MC_OTP_RD_NO_ERR_FLAG(base);
		sec_flag = READ_MEM_CTRL_REGMAP_MC_OTP_RD_SEC_FLAG(base);
		ded_flag = READ_MEM_CTRL_REGMAP_MC_OTP_RD_DED_FLAG(base);
		if (!no_err_flag) {
			printf("%s: ECC error: ECC %x    FLAGS no_err %x sec %x ded %x\n", __func__, ecc, no_err_flag, sec_flag, ded_flag);
			return -ELAST;
		}
#endif
	}

	return ADI_OTP_SUCCESS;
}

static int op_program_setup(const uintptr_t base, uint8_t ecc_state)
{
	OTP_DEBUG("op_program_setup...\n");

	/* Disable interface if USER is enabled */
	if (READ_MEM_CTRL_REGMAP_MC_CMD_EN_USER(base)) {
		int ret = op_disable_interface(base);
		if (ret != ADI_OTP_SUCCESS) return ret;
	}

	/* 1	Enable SBPI interface */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_EN_SBPI(base, 1);

	/* 2	[ONLY FOR n OTP] All OTP should be enabled for PROG setup	Not needed since only 1 OTP instance in Adrv906x */

	/* 3 */
	/* 3.1	Write DAP CQ values to mem ctrl regmap */
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_DAP_CQ(base) = SBPI_DAP_CQ;

	/* 3.2	Write DAP RQ values to mem ctrl regmap */
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_DAP_RQ0(base) = RQ_CQ_DAP_PROG_0;
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_DAP_RQ1(base) = RQ_CQ_DAP_PROG_1;
	if (ecc_state == OTP_ECC_ON)
		*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_DAP_RQ2(base) = RQ_CQ_DAP_PROG_2;
	else
		*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_DAP_RQ2(base) = RQ_CQ_DAP_PROG_2 | 0x00090000;

	/* 3.3	Write PMC CQ values to mem ctrl regmap */
	if (ecc_state == OTP_ECC_ON)
		*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_PMC_CQ(base) = SBPI_PMC_CQ;
	else
		*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_PMC_CQ(base) = SBPI_PMC_CQ | 0x000A;

	/* 3.4	Write PMC RQ values to mem ctrl regmap */
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_PMC_RQ0(base) = RQ_CQ_PMC_PROG_0;
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_PMC_RQ1(base) = RQ_CQ_PMC_PROG_1;
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_PMC_RQ2(base) = RQ_CQ_PMC_PROG_2;

	/* 3.5	Write DAP RQ/CQ values into Sidense Controller */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_RQ_REQ(base, 1);
	WRITE_MEM_CTRL_REGMAP_MC_CMD_WR_DAP_CQ_REQ(base, 1);

	/* 3.6	Wait for self-clear on request */
	if (wait_for_WR_DAP_RQ_CQ_request_clear(base, WR_DAP_RQ_CQ_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s: Write DAP RQ/CQ timeout\n", __func__);
		return -ETIMEDOUT;
	}

	/* 3.7	Write PMC RQ/CQ values into Sidense Controller */
	set_WR_PMC_RQ_CQ_request(base);

	/* 3.8	Wait for self-clear on request */
	if (wait_for_WR_PMC_RQ_CQ_request_clear(base, WR_PMC_RQ_CQ_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s: Write PMC RQ/CQ timeout\n", __func__);
		return -ETIMEDOUT;
	}

	/* 4	PMC Ctrl for PROG operation - write to mem ctrl regmap */
	WRITE_MEM_CTRL_REGMAP_MC_SBPI_PMC_CC_REGS_0(base, 0x000A);

	/* 5	Write PMC Ctrl values into Sidense Controller */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_WR_PMC_CC_REQ(base, 1);

	/* 6	Wait for self-clear on request */
	if (wait_for_WR_PMC_CC_request_clear(base, WR_PMC_CC_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s: Write PMC Ctrl timeout\n", __func__);
		return -ETIMEDOUT;
	}

	/* 7	Redn mode to be used: 00=SE, 01=R, 10=D, 11=DR	Can be done per prog op if needed */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_REDN(base, REDN_MODE);

	/* 8	Set high if ADI controller is generating the ECC word */
#if (ADI_OTP_ECC_TYPE == ECC_TYPE_ADI_SECDED)
	WRITE_MEM_CTRL_REGMAP_MC_CMD_PGM_ECC_GEN(base, 1);
#elif (ADI_OTP_ECC_TYPE == ECC_TYPE_SYNOPSIS_SECSED)
	WRITE_MEM_CTRL_REGMAP_MC_CMD_PGM_ECC_GEN(base, 0);
#endif

	return ADI_OTP_SUCCESS;
}

static int op_program(const uintptr_t base, const uintptr_t addr, uint32_t value, uint8_t ecc_state)
{
	OTP_DEBUG("op_program...\n");

	/* 1	Logical Address */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_ADDR(base, addr);

	/* 2	Data to be programmed */
	*pREG_MEM_CTRL_REGMAP_MEM_CTRL_REGMAP_MC_SBPI_DAP_DATA(base) = value;

	if (ecc_state == OTP_ECC_ON) {
		/* 3	ECC to be programmed (optional). Not needed for Sidense ECC in Redundant mode */
#if (ADI_OTP_ECC_TYPE != ECC_TYPE_SYNOPSIS_SECSED) || (REDN_MODE != REDN_R)
#error "ECC to be programmed"
#endif
	}

	/* 4	OTP Program request */
	WRITE_MEM_CTRL_REGMAP_MC_CMD_PGM_REQ(base, 1);

	/* 5	Wait for self-clear on request */
	if (wait_for_OTP_program_request_clear(base, PGM_REQ_TIMEOUT_US) != ADI_OTP_SUCCESS) {
		ERROR("%s: Program request timeout\n", __func__);
		return -ETIMEDOUT;
	}

	if (ecc_state == OTP_ECC_ON) {
		/* 6	Read status of programmed data. Should be 0x40 */
		if (READ_MEM_CTRL_REGMAP_MC_CMD_PGM_STATUS_0(base) != PGM_WITH_ECC_STATUS_OK) {
			ERROR("%s: Program status NOT OK\n", __func__);
			return -EAGAIN;
		}
	} else {
		/* 5	bit 7:6: 01 operation done */
		if (!program_without_ECC_done(base)) {
			ERROR("%s: Program status NOT OK with ECC disabled\n", __func__);
			return -EAGAIN;
		}
	}

	return ADI_OTP_SUCCESS;
}


/*--------------------------------------------------------
 * EXPORTED FUNCTIONS
 *------------------------------------------------------*/
void otp_init_driver(struct adi_otp_dap_settings dap_settings, struct adi_otp_pmc_settings pmc_settings)
{
	SBPI_DAP_CQ = dap_settings.SBPI_DAP_CQ;
	RQ_CQ_DAP_PROG_2 = dap_settings.RQ_CQ_DAP_PROG_2;
	RQ_CQ_DAP_PROG_1 = dap_settings.RQ_CQ_DAP_PROG_1;
	RQ_CQ_DAP_PROG_0 = dap_settings.RQ_CQ_DAP_PROG_0;

	SBPI_PMC_CQ = pmc_settings.SBPI_PMC_CQ;
	RQ_CQ_PMC_PROG_2 = pmc_settings.RQ_CQ_PMC_PROG_2;
	RQ_CQ_PMC_PROG_1 = pmc_settings.RQ_CQ_PMC_PROG_1;
	RQ_CQ_PMC_PROG_0 = pmc_settings.RQ_CQ_PMC_PROG_0;
}

int otp_read(const uintptr_t base, const uintptr_t addr, uint32_t *value, uint8_t ecc_state)
{
	int ret = ADI_OTP_SUCCESS;

	ret = op_read_setup(base, ecc_state);
	if (ret != ADI_OTP_SUCCESS) return ret;

	ret = op_read(base, addr, value, ecc_state);
	if (ret != ADI_OTP_SUCCESS) return ret;

	return ADI_OTP_SUCCESS;
}

int otp_read_burst(const uintptr_t base, const uintptr_t addr, uint32_t *buffer, size_t len, uint8_t ecc_state)
{
	int ret = ADI_OTP_SUCCESS;
	size_t i;

	ret = op_read_setup(base, ecc_state);
	if (ret != ADI_OTP_SUCCESS) return ret;

	/* Do N op_reads */
	for (i = 0; i < len; i++) {
		uint32_t value;
		ret = op_read(base, addr + i, &value, ecc_state);
		if (ret != ADI_OTP_SUCCESS) return ret;
		buffer[i] = value;
	}

	return ADI_OTP_SUCCESS;
}

int otp_write_burst(const uintptr_t base, const uintptr_t addr, const uint32_t *buffer, size_t len, uint8_t ecc_state)
{
	int ret = ADI_OTP_SUCCESS;
	size_t i;

	ret = op_program_setup(base, ecc_state);
	if (ret != ADI_OTP_SUCCESS) return ret;

	for (i = 0; i < len; i++) {
		ret = op_program(base, addr + i, buffer[i], ecc_state);
		if (ret != ADI_OTP_SUCCESS) return ret;
	}

	return ADI_OTP_SUCCESS;
}

int otp_write(const uintptr_t base, const uintptr_t addr, uint32_t data, uint8_t ecc_state)
{
	return otp_write_burst(base, addr, &data, 1, ecc_state);
}
