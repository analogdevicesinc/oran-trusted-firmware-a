/*
 * Copyright (c) 2024, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <drivers/adi/adi_otp.h>

#include <adrv906x_otp.h>
#include <plat_err.h>

/*--------------------------------------------------------
 * DEFINES
 *------------------------------------------------------*/

/* DAP/PMC settings : Below values are taken from SBPI_TB__CQ_RQ_define.v */
/* RQ_CQ_DAP_PROG = 112'h 0000_0000_0008_ee00_c000_0083_5d2a */
#define ADRV906X_SBPI_DAP_CQ               0x0000
#define ADRV906X_RQ_CQ_DAP_PROG_2          0x00000008
#define ADRV906X_RQ_CQ_DAP_PROG_1          0xee00c000
#define ADRV906X_RQ_CQ_DAP_PROG_0          0x00835d2a
/* RQ_CQ_PMC_PROG = 112'h 4000_028c_1111_5f20_5f0f_df16_5f2a */
#define ADRV906X_SBPI_PMC_CQ               0x4000
#define ADRV906X_RQ_CQ_PMC_PROG_2          0x028c1111
#define ADRV906X_RQ_CQ_PMC_PROG_1          0x5f205f0f
#define ADRV906X_RQ_CQ_PMC_PROG_0          0xdf165f2a

/*
 * Anti-rollback counter:
 * - The counter value N is stored as a sequence of N 1's in the OTP memory.
 * - The number of registers occuped by the counter depends on the maximum supported value
 */
#define ROLLBACK_COUNTER_MAX_VALUE       256
#define BITS_PER_OTP_REGISTER           (OTP_REGISTER_SIZE * 8)
#define ROLLBACK_COUNTER_NUM_REGS       ((ROLLBACK_COUNTER_MAX_VALUE / BITS_PER_OTP_REGISTER) + 1)
#define ROLLBACK_COUNTER_NUM_COPIES     3  /* Anti-rollback counter is stored in triplicate */

/*
 * MAC addresses
 */
#define MAC_ADDRESS_NUM_BYTES           6
#define MAC_ADDRESS_NUM_REGS            2
#define MAC_ADDRESS_NUM_COPIES          3       /* MAC address is stored in triplicate */
#define NUM_MAC_ADDRESSES               6       /* Number of different MAC addresses to store */

/*
 *  NOTE:
 *  The following memory map must agree the one stated in the "Adrv906x OTP Memory Map.xlsx",
 *  that can be located at https://confluence.analog.com/display/A242092596/Adrv906x+OTP+and+Related+Designs
 */

/*
 * OTP Memory Map:
 *
 *          Data                   Address
 * ------------------------------- OTP_ROLLBACK_COUNTER_BASE
 * Anti-rollback counter copy 1
 * Anti-rollback counter copy 2
 * Anti-rollback counter copy 3
 * ------------------------------- OTP_MAC_ADDRESSES_BASE (OTP_ROLLBACK_COUNTER_BASE + ROLLBACK_COUNTER_NUM_COPIES * ROLLBACK_COUNTER_NUM_REGS)
 * MAC 1 copy 1
 * MAC 1 copy 2
 * MAC 1 copy 3
 * [...]
 * MAC 6 copy 3
 * ------------------------------- OTP_MAC_ADDRESSES_END (OTP_MAC_ADDRESSES_BASE + NUM_MAC_ADDRESSES * MAC_ADDRESS_NUM_REGS * MAC_ADDRESS_NUM_COPIES)
 * Available
 * ------------------------------- OTP_OPEN_ZONE_END
 */
#define OTP_OPEN_ZONE_BASE              0x0600
#define OTP_ROLLBACK_COUNTER_BASE       OTP_OPEN_ZONE_BASE
#define OTP_ROLLBACK_COUNTER_END        (OTP_ROLLBACK_COUNTER_BASE + ROLLBACK_COUNTER_NUM_COPIES * ROLLBACK_COUNTER_NUM_REGS)
#define OTP_MAC_ADDRESSES_BASE          OTP_ROLLBACK_COUNTER_END
#define OTP_MAC_ADDRESSES_END           (OTP_MAC_ADDRESSES_BASE + NUM_MAC_ADDRESSES * MAC_ADDRESS_NUM_REGS * MAC_ADDRESS_NUM_COPIES)
#define OTP_OPEN_ZONE_END               0x0700

#if (OTP_MAC_ADDRESSES_END >= OTP_OPEN_ZONE_END)
#error "OTP Memory Map exceedes open zone"
#endif

#define OTP_PRODUCT_ID_BASE             0x0027
#define OTP_PRODUCT_ID_BITP             24U
#define OTP_PRODUCT_ID_BITM             0xFF000000


/*--------------------------------------------------------
 * GLOBALS
 *------------------------------------------------------*/

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS PROTOTYPES
 *------------------------------------------------------*/
static uint64_t get_most_common_value(uint64_t data[], size_t size, uint8_t *is_unique);
static int get_rollback_counter_n(const uintptr_t mem_ctrl_base, unsigned int n, unsigned int *nv_ctr);
static int set_rollback_counter_n(const uintptr_t mem_ctrl_base, unsigned int n, unsigned int nv_ctr);

static bool get_most_common_mac(uint8_t *macs_list, uint8_t num_macs, uint8_t *mac);
static int get_mac_addr_n(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t n, uint8_t *mac);
static int set_mac_addr_n(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t n, uint8_t *mac);

/*--------------------------------------------------------
 * INTERNAL FUNCTIONS
 *------------------------------------------------------*/
static uint64_t get_most_common_value(uint64_t data[], size_t size, uint8_t *is_unique)
{
	size_t max_count = 0;
	uint64_t most_common_value = 0;

	for (size_t i = 0; i < size; i++) {
		uint64_t value = data[i];
		unsigned int count = 1;

		for (size_t j = i + 1; j < size; j++)
			if (data[j] == value) count++;

		if (count == max_count)
			*is_unique = 0;

		if (count > max_count) {
			max_count = count;
			most_common_value = value;
			*is_unique = 1;
		}

		if (max_count > (size / 2)) break;
	}
	return most_common_value;
}

static int get_rollback_counter_n(const uintptr_t mem_ctrl_base, unsigned int n, unsigned int *nv_ctr)
{
	uint32_t rollback_counter[ROLLBACK_COUNTER_NUM_REGS] = { 0 };
	const uintptr_t counter_addr = OTP_ROLLBACK_COUNTER_BASE + n * ROLLBACK_COUNTER_NUM_REGS;
	uint32_t count = 0;
	uint32_t index = 0;

	if (otp_read_burst(mem_ctrl_base, counter_addr, rollback_counter, ROLLBACK_COUNTER_NUM_REGS, OTP_ECC_OFF) != ADI_OTP_SUCCESS) {
		plat_error_message("%s: Cannot read Rollback Counter copy %d", __func__, n);
		return -EIO;
	}

	while (rollback_counter[index]) {
		count++;
		rollback_counter[index] >>= 1;
		if (count % BITS_PER_OTP_REGISTER == 0) index++;
		if (index == ROLLBACK_COUNTER_NUM_REGS) break;
	}
	*nv_ctr = count;

	return ADI_OTP_SUCCESS;
}

static int set_rollback_counter_n(const uintptr_t mem_ctrl_base, unsigned int n, unsigned int nv_ctr)
{
	uint32_t rollback_counter[ROLLBACK_COUNTER_NUM_REGS] = { 0 };
	uint32_t index = 0;
	uintptr_t counter_addr;

	for (uint32_t i = 0; i < nv_ctr; i++) {
		rollback_counter[index] <<= 1;
		rollback_counter[index] += 1;
		if ((i + 1) % BITS_PER_OTP_REGISTER == 0) index++;
	}

	counter_addr = OTP_ROLLBACK_COUNTER_BASE + n * ROLLBACK_COUNTER_NUM_REGS;
	if (otp_write_burst(mem_ctrl_base, counter_addr, rollback_counter, ROLLBACK_COUNTER_NUM_REGS, OTP_ECC_OFF) != ADI_OTP_SUCCESS) {
		plat_error_message("%s: Cannot write Rollback Counter copy %d", __func__, n);
		return -EIO;
	}

	return ADI_OTP_SUCCESS;
}

static bool get_most_common_mac(uint8_t *macs_list, uint8_t num_macs, uint8_t *selected_mac)
{
	uint8_t max_appearances = 0;
	uint8_t *mac_with_max_appearances = 0;
	bool single_mac_with_max_appearances = false;

	for (size_t i = 0; i < num_macs; i++) {
		/* Get a MAC */
		uint8_t *mac = (uint8_t *)&macs_list[MAC_ADDRESS_NUM_BYTES * i];
		uint8_t appearances = 1;

		/* Look for other appearances of the same MAC on the list */
		for (size_t j = i + 1; j < num_macs; j++) {
			uint8_t *aux_mac = (uint8_t *)&macs_list[MAC_ADDRESS_NUM_BYTES * j];
			if (memcmp(mac, aux_mac, MAC_ADDRESS_NUM_BYTES) == 0) appearances++;
		}

		/* More that one MAC have the max appearances */
		if (appearances == max_appearances)
			single_mac_with_max_appearances = false;

		/* Just a single MAC has the max appearances */
		if (appearances > max_appearances) {
			max_appearances = appearances;
			mac_with_max_appearances = mac;
			single_mac_with_max_appearances = true;
		}

		/* Early end */
		if (max_appearances > (num_macs / 2)) break;
	}
	memcpy(selected_mac, mac_with_max_appearances, MAC_ADDRESS_NUM_BYTES);
	return single_mac_with_max_appearances;
}

static int get_mac_addr_n(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t n, uint8_t *mac)
{
	uint32_t mac32[MAC_ADDRESS_NUM_REGS] = { 0 };
	const uintptr_t addr = OTP_MAC_ADDRESSES_BASE + (mac_number - 1) * MAC_ADDRESS_NUM_REGS * MAC_ADDRESS_NUM_COPIES + n * MAC_ADDRESS_NUM_REGS;

	if (otp_read_burst(mem_ctrl_base, addr, mac32, MAC_ADDRESS_NUM_REGS, OTP_ECC_OFF) != ADI_OTP_SUCCESS) {
		plat_error_message("%s: Cannot read MAC address %d", __func__, mac_number);
		return -EIO;
	}

	mac[0] = (mac32[0] >> 0) & 0xFF;
	mac[1] = (mac32[0] >> 8) & 0xFF;
	mac[2] = (mac32[0] >> 16) & 0xFF;
	mac[3] = (mac32[0] >> 24) & 0xFF;
	mac[4] = (mac32[1] >> 0) & 0xFF;
	mac[5] = (mac32[1] >> 8) & 0xFF;

	return ADI_OTP_SUCCESS;
}

static int set_mac_addr_n(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t n, uint8_t *mac)
{
	uint32_t mac32[MAC_ADDRESS_NUM_REGS];
	const uintptr_t addr = OTP_MAC_ADDRESSES_BASE + (mac_number - 1) * MAC_ADDRESS_NUM_REGS * MAC_ADDRESS_NUM_COPIES + n * MAC_ADDRESS_NUM_REGS;

	mac32[1] = (mac[5] << 8) | mac[4];
	mac32[0] = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];

	if (otp_write_burst(mem_ctrl_base, addr, mac32, MAC_ADDRESS_NUM_REGS, OTP_ECC_OFF) != ADI_OTP_SUCCESS) {
		plat_error_message("%s: Cannot write MAC address %d", __func__, mac_number);
		return -EIO;
	}

	return ADI_OTP_SUCCESS;
}

/*--------------------------------------------------------
 * EXPORTED FUNCTIONS
 *------------------------------------------------------*/
void adrv906x_otp_init_driver(void)
{
	struct adi_otp_dap_settings dap_settings;
	struct adi_otp_pmc_settings pmc_settings;

	dap_settings.SBPI_DAP_CQ = ADRV906X_SBPI_DAP_CQ;
	dap_settings.RQ_CQ_DAP_PROG_2 = ADRV906X_RQ_CQ_DAP_PROG_2;
	dap_settings.RQ_CQ_DAP_PROG_1 = ADRV906X_RQ_CQ_DAP_PROG_1;
	dap_settings.RQ_CQ_DAP_PROG_0 = ADRV906X_RQ_CQ_DAP_PROG_0;

	pmc_settings.SBPI_PMC_CQ = ADRV906X_SBPI_PMC_CQ;
	pmc_settings.RQ_CQ_PMC_PROG_2 = ADRV906X_RQ_CQ_PMC_PROG_2;
	pmc_settings.RQ_CQ_PMC_PROG_1 = ADRV906X_RQ_CQ_PMC_PROG_1;
	pmc_settings.RQ_CQ_PMC_PROG_0 = ADRV906X_RQ_CQ_PMC_PROG_0;

	otp_init_driver(dap_settings, pmc_settings);
}

int adrv906x_otp_get_product_id(const uintptr_t mem_ctrl_base, uint8_t *id)
{
	uint32_t data;

	int ret = otp_read(mem_ctrl_base, OTP_PRODUCT_ID_BASE, &data, OTP_ECC_ON);

	if (ret != 0) return ret;

	*id = (data & OTP_PRODUCT_ID_BITM) >> OTP_PRODUCT_ID_BITP;

	return ADI_OTP_SUCCESS;
}

int adrv906x_otp_get_rollback_counter(const uintptr_t mem_ctrl_base, unsigned int *nv_ctr)
{
	uint64_t counters[ROLLBACK_COUNTER_NUM_COPIES];
	unsigned int num_valid_counters = 0;
	unsigned int aux;
	uint8_t is_unique;

	/* Gather valid counters */
	for (unsigned int i = 0; i < ROLLBACK_COUNTER_NUM_COPIES; i++)
		if (get_rollback_counter_n(mem_ctrl_base, i, &aux) == ADI_OTP_SUCCESS) counters[num_valid_counters++] = (uint64_t)aux;

	if (num_valid_counters == 0) {
		plat_error_message("%s: Rollback Counter read error. No valid counter read", __func__);
		return -EIO;
	}

	is_unique = 0;
	*nv_ctr = get_most_common_value(counters, num_valid_counters, &is_unique);
	if (!is_unique) {
		plat_error_message("%s: Rollback Counter read error. Counter is corrupted", __func__);
		return -EIO;
	}

	return ADI_OTP_SUCCESS;
}

int adrv906x_otp_set_rollback_counter(const uintptr_t mem_ctrl_base, unsigned int nv_ctr)
{
	if (nv_ctr > ROLLBACK_COUNTER_MAX_VALUE) {
		plat_error_message("%s: Rollback counter %d out of bounds (0 .. %d)", __func__, nv_ctr, ROLLBACK_COUNTER_MAX_VALUE);
		return -EINVAL;
	}

	for (unsigned int i = 0; i < ROLLBACK_COUNTER_NUM_COPIES; i++) {
		int ret = set_rollback_counter_n(mem_ctrl_base, i, nv_ctr);
		if (ret != ADI_OTP_SUCCESS) return ret;
	}

	return ADI_OTP_SUCCESS;
}

int adrv906x_otp_set_mac_addr(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t *mac)
{
	if (mac_number <= 0 || mac_number > NUM_MAC_ADDRESSES) {
		plat_error_message("%s: MAC number %d out of bounds (1 .. %d)", __func__, mac_number, NUM_MAC_ADDRESSES);
		return -EINVAL;
	}

	for (unsigned int i = 0; i < MAC_ADDRESS_NUM_COPIES; i++) {
		int ret = set_mac_addr_n(mem_ctrl_base, mac_number, i, mac);
		if (ret != ADI_OTP_SUCCESS) return ret;
	}

	return ADI_OTP_SUCCESS;
}

int adrv906x_otp_get_mac_addr(const uintptr_t mem_ctrl_base, uint8_t mac_number, uint8_t *mac)
{
	uint8_t macs[MAC_ADDRESS_NUM_COPIES][MAC_ADDRESS_NUM_BYTES];
	unsigned int num_macs = 0;

	if (mac_number <= 0 || mac_number > NUM_MAC_ADDRESSES) {
		plat_error_message("%s: MAC number %d out of bounds (1 .. %d)", __func__, mac_number, NUM_MAC_ADDRESSES);
		return -EINVAL;
	}

	/* Gather valid counters */
	for (unsigned int i = 0; i < MAC_ADDRESS_NUM_COPIES; i++)
		if (get_mac_addr_n(mem_ctrl_base, mac_number, i, (uint8_t *)&macs[num_macs]) == ADI_OTP_SUCCESS) num_macs++;

	if (num_macs == 0) {
		plat_error_message("%s: MAC %d read error. No valid MAC read", __func__, mac_number);
		return -EIO;
	}

	if (!get_most_common_mac((uint8_t *)&macs[0], num_macs, mac)) {
		plat_error_message("%s: MAC %d read error. MAC is corrupted", __func__, mac_number);
		return -EIO;
	}

	return ADI_OTP_SUCCESS;
}
