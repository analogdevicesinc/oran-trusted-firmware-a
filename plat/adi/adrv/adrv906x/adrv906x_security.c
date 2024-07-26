/*
 * Copyright (c) 2022, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <drivers/arm/tzc400.h>
#include <drivers/adi/adi_spu.h>

#include <adrv906x_device_profile.h>
#include <adrv906x_spu_def.h>
#include <plat_common_def.h>
#include <plat_boot.h>
#include <plat_security.h>

/* Each Adrv906x L4 TZC only covers a 2MB window of the total 6MB L4 SRAM:
 * TZC0: 0MB - 2MB
 * TZC1: 2MB - 4MB
 * TZC2: 4MB - 6MB
 */
#define SRAM_TZC_BASE U(0x00000000)
#define SRAM_TZC_TOP  U(0x00200000 - 1) /* 2MB region */
#define SRAM_TZC0_BASE U(0x00000000)
#define SRAM_TZC0_TOP U(0x00200000 - 1)
#define SRAM_TZC1_BASE U(0x00200000)
#define SRAM_TZC1_TOP U(0x00400000 - 1)
#define SRAM_TZC2_BASE U(0x00400000)
#define SRAM_TZC2_TOP U(0x00600000 - 1)
/* Converts absolute addr to TZC-relative addr */
#define SRAM_TZC_ADDR(x) ((x) & 0x1FFFFF)

#define TZC_NSAID_ANTENNA_CAL           U(0)
#define TZC_NSAID_C2C_RESPONDER         U(1)
#define TZC_NSAID_CBDDE         U(2)
#define TZC_NSAID_DAP           U(3)
#define TZC_NSAID_DEBUG_DDE             U(4)
#define TZC_NSAID_DEBUG_DDE2            U(4)
#define TZC_NSAID_ETH_TXRX              U(5)
#define TZC_NSAID_ETH_STAT              U(6)
#define TZC_NSAID_M4TOA55               U(7)
#define TZC_NSAID_MDMA0_1               U(8)
#define TZC_NSAID_PIMC_DDE              U(9)
#define TZC_NSAID_A55_PERI_MASTER               U(10)
#define TZC_NSAID_MDMA4_5               U(11)
#define TZC_NSAID_MDMA2_3               U(12)
#define TZC_NSAID_DEFAULT               U(15)

#define PLAT_TZC_NS_DEV_ACCESS  \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_ANTENNA_CAL)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_C2C_RESPONDER)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_CBDDE)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_DAP)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_DEBUG_DDE)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_DEBUG_DDE2)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_ETH_TXRX)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_ETH_STAT)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_M4TOA55)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_MDMA0_1)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_PIMC_DDE)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_A55_PERI_MASTER)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_MDMA4_5)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_MDMA2_3)) |       \
	(TZC_REGION_ACCESS_RDWR(TZC_NSAID_DEFAULT))

typedef enum spu_a55mmr_peripheral_ids spu_a55mmr_peripheral_ids_t;

/* To keep the SRAM TZC code below simple, we make some assumptions about the
 * partitioning on secure/non-secure spaces in SRAM. Test those assumptions here:
 */
/* NS_SRAM_BASE must fall within TZC0's range */
CASSERT((NS_SRAM_BASE > SRAM_TZC0_BASE) && (NS_SRAM_BASE < SRAM_TZC0_TOP), NS_SRAM_BASE_not_in_tzc0);
/* NS_SRAM_TOP must fall within TZC2's range */
CASSERT(((NS_SRAM_BASE + NS_SRAM_SIZE - 1U) > SRAM_TZC2_BASE) && ((NS_SRAM_BASE + NS_SRAM_SIZE - 1U) < SRAM_TZC2_TOP), NS_SRAM_TOP_not_in_tzc2);
/* SEC_NS_SRAM_BASE is the secondary equivalent of NS_SRAM_BASE */
CASSERT(SEC_NS_SRAM_BASE == (NS_SRAM_BASE | 0x04000000), SEC_NS_SRAM_BASE_doesnt_match);
/* SEC_NS_SRAM_SIZE is equivalent to NS_SRAM_SIZE */
CASSERT(SEC_NS_SRAM_SIZE == NS_SRAM_SIZE, SEC_NS_SRAM_SIZE_not_correct);

static plat_spu_peripherals_info_t plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPHERALS_COUNT];
static plat_spu_peripherals_info_t plat_spu_oran_peripherals[SPU_ORAN_PERIPHERALS_COUNT];
static plat_spu_peripherals_info_t plat_spu_xcorr_peripherals[SPU_XCORR_PERIPHERALS_COUNT];
static plat_spu_peripherals_info_t plat_spu_dfe_peripherals[SPU_DFE_PERIPHERALS_COUNT];
static plat_spu_peripherals_info_t plat_spu_m4_peripherals[SPU_M4_PERIPHERALS_COUNT];

/*
 * TZC Region Configuration
 *
 * Primary L4 (SRAM) - TZC0, TZC1, TZC2
 *
 *      +----------------------------+      SRAM_BASE + SRAM_SIZE - 1U
 *      |        Secure              |
 *      |----------------------------|      NS_SRAM_BASE + NS_SRAM_SIZE - 1U
 *      |        Non-secure          |
 *      |----------------------------|      NS_SRAM_BASE
 *      |        Secure              |
 *      +----------------------------+      SRAM_BASE
 *
 * Primary DDR (DRAM) - TZC4
 *
 *      +----------------------------+      TEE_SHMEM_BASE + ns_dram_size - 1U
 *      |        Non-secure          |
 *      |----------------------------|      TEE_SHMEM_BASE
 *      |        Secure              |
 *      +----------------------------+      SECURE_DRAM_BASE
 *
 * Secondary L4 (SRAM): Identical layout to primary
 * Secondary DDR (DRAM): All non-secure
 */
static void adrv906x_tzc_setup(void)
{
	size_t ns_dram_size = 0;
	size_t dram_size = 0;
	size_t sec_dram_size = 0;
	uintptr_t sec_dram_base = 0;

	dram_size = plat_get_dram_size();
	assert(dram_size > SECURE_DRAM_SIZE);
	ns_dram_size = dram_size - SECURE_DRAM_SIZE;

	if (plat_get_dual_tile_enabled()) {
		sec_dram_base = plat_get_secondary_dram_base();
		sec_dram_size = plat_get_secondary_dram_size();
		/*If and only if there is no physical secondary DDR present, we need to add the secondary's "size" to ns_dram_size so the TZC covers both primary and secondary spaces.
		 * Otherwise, the TZC would only cover the part of the DDR portioned to the primary and error on any access to the "secondary's" DDR space. */
		if (!plat_is_secondary_phys_dram_present())
			ns_dram_size += sec_dram_size;
	}

	/* SRAM TZC0 has two regions:
	 * Secure: 0MB - NS_SRAM_BASE
	 * Non-secure: NS_SRAM_BASE - 2MB
	 */
	const plat_tzc_regions_info_t sram_tzc0_regions[] = {
		{ SRAM_TZC_ADDR(SRAM_BASE),    SRAM_TZC_ADDR(NS_SRAM_BASE - 1U),
		  TZC_REGION_S_RDWR, 0 },
		{ SRAM_TZC_ADDR(NS_SRAM_BASE), SRAM_TZC_TOP,
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ 0,			       0,				0,0 }
	};

	/* SRAM TZC1 has one region:
	 * Non-secure: 0MB - 2MB
	 */
	const plat_tzc_regions_info_t sram_tzc1_regions[] = {
		{ SRAM_TZC_BASE, SRAM_TZC_TOP,
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ 0,		 0,	      0,0 }
	};

	/* SRAM TZC2 has two regions:
	 * Non-secure: 0MB - NS_SRAM_TOP
	 * Secure: NS_SRAM_TOP - 2MB
	 */
	const plat_tzc_regions_info_t sram_tzc2_regions[] = {
		{ SRAM_TZC_BASE,			      SRAM_TZC_ADDR(NS_SRAM_BASE + NS_SRAM_SIZE - 1U),
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ SRAM_TZC_ADDR(NS_SRAM_BASE + NS_SRAM_SIZE), SRAM_TZC_ADDR(SRAM_BASE + SRAM_SIZE - 1U),
		  TZC_REGION_S_RDWR, 0 },
		{ 0,					      0,					      0,0 }
	};

	/* Secondary SRAM TZCs have the same layout, just with secondary address supplied */
	const plat_tzc_regions_info_t sram_secondary_tzc0_regions[] = {
		{ SRAM_TZC_ADDR(SEC_SRAM_BASE),	   SRAM_TZC_ADDR(SEC_NS_SRAM_BASE - 1U),
		  TZC_REGION_S_RDWR, 0 },
		{ SRAM_TZC_ADDR(SEC_NS_SRAM_BASE), SRAM_TZC_TOP,
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ 0,				   0,					0,0 }
	};

	const plat_tzc_regions_info_t sram_secondary_tzc1_regions[] = {
		{ SRAM_TZC_BASE, SRAM_TZC_TOP,
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ 0,		 0,	      0,0 }
	};

	const plat_tzc_regions_info_t sram_secondary_tzc2_regions[] = {
		{ SRAM_TZC_BASE,				      SRAM_TZC_ADDR(SEC_NS_SRAM_BASE + SEC_NS_SRAM_SIZE - 1U),
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ SRAM_TZC_ADDR(SEC_NS_SRAM_BASE + SEC_NS_SRAM_SIZE), SRAM_TZC_ADDR(SEC_SRAM_BASE + SEC_SRAM_SIZE - 1U),
		  TZC_REGION_S_RDWR, 0 },
		{ 0,						      0,						      0,0 }
	};

	const plat_tzc_regions_info_t dram_tzc_regions[] = {
		{ SECURE_DRAM_BASE, SECURE_DRAM_BASE + SECURE_DRAM_SIZE - 1U,
		  TZC_REGION_S_RDWR, 0 },
		{ TEE_SHMEM_BASE,   TEE_SHMEM_BASE + ns_dram_size - 1U,
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ 0,		    0,					     0,0 }
	};

	const plat_tzc_regions_info_t dram_secondary_tzc_regions[] = {
		{ sec_dram_base, sec_dram_base + sec_dram_size - 1U,
		  TZC_REGION_S_RDWR, PLAT_TZC_NS_DEV_ACCESS },
		{ 0,		 0,				    0,0 }
	};

	plat_tzc400_setup(TZC_0_BASE, sram_tzc0_regions);
	plat_tzc400_setup(TZC_1_BASE, sram_tzc1_regions);
	plat_tzc400_setup(TZC_2_BASE, sram_tzc2_regions);
	plat_tzc400_setup(TZC_4_BASE, dram_tzc_regions);

	if (plat_get_dual_tile_enabled()) {
		plat_tzc400_setup(SEC_TZC_0_BASE, sram_secondary_tzc0_regions);
		plat_tzc400_setup(SEC_TZC_1_BASE, sram_secondary_tzc1_regions);
		plat_tzc400_setup(SEC_TZC_2_BASE, sram_secondary_tzc2_regions);
		/*Only set up region in secondary TZC for the DDR if it actually physically exists*/
		if (plat_is_secondary_phys_dram_present())
			plat_tzc400_setup(SEC_TZC_4_BASE, dram_secondary_tzc_regions);
	}
}


static void adrv906x_override_secure_spi_spu(uint32_t index)
{
	/*  SPI secure configuration is a bit more complex than for the other
	 *  peripherals, so let's isolate this configuration
	 */
	const spu_a55mmr_peripheral_ids_t spu_slave_periph_map[][3] = {
		{ SPU_A55MMR_PERIPH_SPICONFIG0, SPU_A55MMR_PERIPH_SPI0DDE0, SPU_A55MMR_PERIPH_SPI0DDE1 },
		{ SPU_A55MMR_PERIPH_SPICONFIG1, SPU_A55MMR_PERIPH_SPI1DDE0, SPU_A55MMR_PERIPH_SPI1DDE1 },
		{ SPU_A55MMR_PERIPH_SPICONFIG2, SPU_A55MMR_PERIPH_SPI2DDE0, SPU_A55MMR_PERIPH_SPI2DDE1 },
		{ SPU_A55MMR_PERIPH_SPICONFIG3, SPU_A55MMR_PERIPH_SPI3DDE0, SPU_A55MMR_PERIPH_SPI3DDE1 },
		{ SPU_A55MMR_PERIPH_SPICONFIG4, SPU_A55MMR_PERIPH_SPI4DDE0, SPU_A55MMR_PERIPH_SPI4DDE1 },
		{ SPU_A55MMR_PERIPH_SPICONFIG5, SPU_A55MMR_PERIPH_SPI5DDE0, SPU_A55MMR_PERIPH_SPI5DDE1 },
	};
	const spu_a55mmr_peripheral_ids_t spu_master_periph_map[][2] = {
		{ SPU_A55MMR_PERIPH_SPI0DDE0, SPU_A55MMR_PERIPH_SPI0DDE1 },
		{ SPU_A55MMR_PERIPH_SPI1DDE0, SPU_A55MMR_PERIPH_SPI1DDE1 },
		{ SPU_A55MMR_PERIPH_SPI2DDE0, SPU_A55MMR_PERIPH_SPI2DDE1 },
		{ SPU_A55MMR_PERIPH_SPI3DDE0, SPU_A55MMR_PERIPH_SPI3DDE1 },
		{ SPU_A55MMR_PERIPH_SPI4DDE0, SPU_A55MMR_PERIPH_SPI4DDE1 },
		{ SPU_A55MMR_PERIPH_SPI5DDE0, SPU_A55MMR_PERIPH_SPI5DDE1 },
	};

	/* Sanity check */
	if (index > (FW_CONFIG_PERIPH_SPI5 - FW_CONFIG_PERIPH_SPI0)) {
		ERROR("SPI entry too large");
		return;
	}

	/* Mark slave peripherals as secure */
	plat_spu_a55mmr_peripherals[spu_slave_periph_map[index][0]].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[spu_slave_periph_map[index][1]].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[spu_slave_periph_map[index][2]].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* Mark master peripherals as secure */
	plat_spu_a55mmr_peripherals[spu_master_periph_map[index][0]].flags |= ADI_SPU_PERIPHERAL_FLAGS_MSEC;
	plat_spu_a55mmr_peripherals[spu_master_periph_map[index][1]].flags |= ADI_SPU_PERIPHERAL_FLAGS_MSEC;
}

static void adrv906x_customer_override_spu(void)
{
	/* Security configuration for some peripherals is exposed to the customer
	 * Note: only a few A55MMR peripherals are exposed
	 *
	 * This is a mapping between the exposed peripherals and their
	 * corresponding SPU entry index.
	 */
	const enum spu_a55mmr_peripheral_ids spu_slave_periph_map[] = {
		SPU_A55MMR_PERIPH_UART_1,
		SPU_A55MMR_PERIPH_UART_3,
		SPU_A55MMR_PERIPH_UART_4,
		SPU_A55MMR_PERIPH_SPICONFIG0,
		SPU_A55MMR_PERIPH_SPICONFIG1,
		SPU_A55MMR_PERIPH_SPICONFIG2,
		SPU_A55MMR_PERIPH_SPICONFIG3,
		SPU_A55MMR_PERIPH_SPICONFIG4,
		SPU_A55MMR_PERIPH_SPICONFIG5,
		SPU_A55MMR_PERIPH_I2C0,
		SPU_A55MMR_PERIPH_I2C1,
		SPU_A55MMR_PERIPH_I2C2,
		SPU_A55MMR_PERIPH_I2C3,
		SPU_A55MMR_PERIPH_I2C4,
		SPU_A55MMR_PERIPH_I2C5,
		SPU_A55MMR_PERIPH_I2C6,
		SPU_A55MMR_PERIPH_I2C7,
	};
	bool *secure_peripherals;
	uint32_t len;

	/* Sanity check */
	if (FW_CONFIG_PERIPH_NUM_MAX != sizeof(spu_slave_periph_map) / sizeof(enum spu_a55mmr_peripheral_ids)) {
		ERROR("sou_periph_map list size is not correct");
		return;
	}

	secure_peripherals = plat_get_secure_peripherals(&len);
	if (secure_peripherals == NULL) {
		ERROR("Invalid pointer to the secure_peripheral list");
		return;
	}

	for (uint32_t i = 0; i < len; i++)
		if (secure_peripherals[i]) {
			if ((i >= FW_CONFIG_PERIPH_SPI0) && (i <= FW_CONFIG_PERIPH_SPI5))
				/* Mark all related peripherals as secure */
				adrv906x_override_secure_spi_spu(i - FW_CONFIG_PERIPH_SPI0);
			else
				/* Mark the peripheral as secure */
				plat_spu_a55mmr_peripherals[spu_slave_periph_map[i]].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
		}
}

/* Disable SPU, for development/debug purposes.
 * To enable, insert before all calls to plat_spu_setup()
 * in adrv906x_spu_setup()
 */
static __unused void adrv906x_spu_disable(void)
{
	unsigned int i;

	/* Set NO_SSEC for all SPU regions, opening them to NS accesses */
	for (i = 0; i < SPU_A55MMR_PERIPHERALS_COUNT; i++)
		plat_spu_a55mmr_peripherals[i].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	for (i = 0; i < SPU_ORAN_PERIPHERALS_COUNT; i++)
		plat_spu_oran_peripherals[i].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	for (i = 0; i < SPU_XCORR_PERIPHERALS_COUNT; i++)
		plat_spu_xcorr_peripherals[i].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	for (i = 0; i < SPU_DFE_PERIPHERALS_COUNT; i++)
		plat_spu_dfe_peripherals[i].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	for (i = 0; i < SPU_M4_PERIPHERALS_COUNT; i++)
		plat_spu_m4_peripherals[i].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
}

/*
 * The SPU (System Protection Unit) is an access control system for hardware
 * peripherals. Each peripheral can be configured to respond only to certain
 * masters and only in certain modes (secure/non-secure). Adrv906x has five SPUs.
 *
 *     +-----------------------------------------------------------------+
 *     |                        central axi fabric                       |
 *     |                        (A55 main fabric)                        |
 *     +-----------------------------------------------------------------+
 *          |             |             |             |             |
 *     +---------+   +---------+   +---------+   +---------+   +---------+
 *     |   A55   |   |    M4   |   |  ORAN   |   |  XCORR  |   |   DFE   |
 *     +---------+   +---------+   +---------+   +---------+   +---------+
 *     / / /|\ \ \   / / /|\ \ \   / / /|\ \ \   / / /|\ \ \   / / /|\ \ \
 *    + | + | + | + + | + | + | + + | + | + | + + | + | + | + + | + | + | +
 *      +   +   +     +   +   +     +   +   +     +   +   +     +   +   +
 *
 */
static void adrv906x_spu_setup(void)
{
	plat_boot_device_t boot_device;

	memset(plat_spu_a55mmr_peripherals, 0, SPU_A55MMR_PERIPHERALS_COUNT * sizeof(plat_spu_peripherals_info_t));
	memset(plat_spu_oran_peripherals, 0, SPU_ORAN_PERIPHERALS_COUNT * sizeof(plat_spu_peripherals_info_t));
	memset(plat_spu_xcorr_peripherals, 0, SPU_XCORR_PERIPHERALS_COUNT * sizeof(plat_spu_peripherals_info_t));
	memset(plat_spu_dfe_peripherals, 0, SPU_DFE_PERIPHERALS_COUNT * sizeof(plat_spu_peripherals_info_t));
	memset(plat_spu_m4_peripherals, 0, SPU_M4_PERIPHERALS_COUNT * sizeof(plat_spu_peripherals_info_t));

	/* A55MMR */
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_A55_TIMER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_A55TOCORE0MAILBOXSRC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_A55TOCORE0MAILBOXDST].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_A55TOCORE1MAILBOXSRC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_A55TOCORE1MAILBOXDST].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_CORE0TOA55MAILBOXSRC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_CORE0TOA55MAILBOXDST].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_CORE1TOA55MAILBOXSRC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_CORE1TOA55MAILBOXDST].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_4].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_5].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_CAPBUFDDE].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_TRU].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_UART_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYSCFG_BOOT_NS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_FEATURE_FILTER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMAC1G].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMMC0SLV].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMMC1SLV].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_PWMSLV].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSP_RX_INTRF_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSP_RX_INTRF_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSP_TX_INTRF_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSP_TX_INTRF_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QUAD_SPI_DMA_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QUAD_SPI_DMA_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QSPI_CONFIG].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSDDE0RX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSDDE1TX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSDDE2RX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MSDDE3TX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_STATDDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_STATDDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_DEBUGDDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_DEBUGDDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_ANTENNACALDDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_PIMCDDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_CAPBUF].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_V_UART0_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_V_UART0_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_V_UART1_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_V_UART1_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_NS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_PERI_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_MDMA_PERI_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_DEBUG_DDE2_CAPTURE].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_ML_DPD_DDE_CFG].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_CLOCK_CONTROL_2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_SCRATCH_NS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_GPIO_CAP].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_GPIO_RFFE].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SUBSYS_CFG_PERIF_NS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_GPIO_NSEC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_GPIO_CROSSBAR_CONTROL].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_UART_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_UART_3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_UART_4].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPICONFIG0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPICONFIG1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPICONFIG2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPICONFIG3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPICONFIG4].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPICONFIG5].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI0DDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI0DDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI1DDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI1DDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI2DDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI2DDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI3DDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI3DDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI4DDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI4DDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI5DDE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_SPI5DDE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C4].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C5].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C6].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_I2C7].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* For SystemC, enable UART4 (PL011 UART3) which will be used instead of the internal virtual UARTs */
	/* TODO: Consider removing this if/when SystemC support is removed */
	if (plat_is_sysc())
		plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_UART_4].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* ORAN */
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_ANTENNA_CAL].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_RESERVED_SPACE_PPI].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_LOWPHY_DEBUG_SHARED].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_LOWPHY_DEBUG_CAPT_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_LOWPHY_DEBUG_CAPT_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_LOWPHY_DEBUG_DBG_INJECT].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_ORAN_CDUC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_RADIO_CONTROL].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_COMMON].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_XGMII_PCS_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_XGMII_PCS_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_XGMII_EMAC_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_XGMII_EMAC_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_SWITCH].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_MACSEC_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_MACSEC_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_TOD].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_SERDES_PHY].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_EMAC_TOP_ETH_PLL].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_oran_peripherals[SPU_ORAN_PERIPH_ORAN_IF].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* XCORR */
	plat_spu_xcorr_peripherals[SPU_XCORR_PERIPH_XCORR_MMR].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_xcorr_peripherals[SPU_XCORR_PERIPH_XCORR_INPUT_MEM].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_xcorr_peripherals[SPU_XCORR_PERIPH_XCORR_OUTPUT_MEM].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* DFE */
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE4].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE5].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE6].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_dfe_peripherals[SPU_DFE_PERIPH_DFE7].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* M4 */
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_0_TIMER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_0__WATCHDOG_TIMER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_0_SPI_PM_KEY].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_0_INTR_AGGREGATOR].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_0_SPI0_CMD_MAILBOX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_0_SPI1_CMD_MAILBOX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_C2C_MAILBOX_0_SOURCE].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_C2C_MAILBOX_1_DEST].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_1_TIMER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_1__WATCHDOG_TIMER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_1_SPI_PM_KEY].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_1_INTR_AGGREGATOR].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_1_SPI0_CMD_MAILBOX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_1_SPI1_CMD_MAILBOX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_C2C_MAILBOX_1_SOURCE].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_C2C_MAILBOX_0_DEST].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_UARTFIFO].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_SPI_MASTER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_ITM].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_TELEMETRY].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_SNOOPER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_INTERRUPT_TRANSMUTER].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_AHB_ARM_DAP_REGS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_DEBUG_KEY_REGS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_TELEMETRY_MEMORY].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_UART].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_SEMAPHORE].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_CORE_STREAM_PROC_MEM].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_MAIN_STREAM_PROC].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_SPI_ONLY_REGS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_EAST_RFPLL].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_WEST_RFPLL].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_RC_TUNER_ANALOG].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_KFA_STREAM_PROC_REGS].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_KFA_TOP_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_KFA_STREAM_PROC_MEM_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_KFA_STREAM_PROC_MEM_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_MPU_NCO_SUBSYS_REGS_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_MPU_NCO_SUBSYS_REGS_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_MPU_NCO_SUBSYS_REGS_2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_MPU_NCO_SUBSYS_REGS_3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_A55_SPI1_CMD_MAILBOX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_A55_SPI0_CMD_MAILBOX].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_SEQUENCER_REG].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_SEQUENCER_MEM].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_RX0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_RX1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_RX2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_RX3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_ORX0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_TX0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_TX1].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_TX2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_TX3].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_M4_MEMORY_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;
	plat_spu_m4_peripherals[SPU_M4_PERIPH_M4_MEMORY_2].flags |= ADI_SPU_PERIPHERAL_FLAGS_NO_SSEC;

	/* BL2 interacts with the boot device over DMA, so the boot device needs
	 * to be a secure master. these permissions get removed before BL31. */
	boot_device = plat_get_boot_device();
	if (boot_device == PLAT_BOOT_DEVICE_EMMC_0) {
		plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMMC0SLV].flags |= ADI_SPU_PERIPHERAL_FLAGS_MSEC;
	} else if (boot_device == PLAT_BOOT_DEVICE_SD_0) {
		plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMMC1SLV].flags |= ADI_SPU_PERIPHERAL_FLAGS_MSEC;
	} else if (boot_device == PLAT_BOOT_DEVICE_QSPI_0) {
		plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QUAD_SPI_DMA_0].flags |= ADI_SPU_PERIPHERAL_FLAGS_MSEC;
		plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QUAD_SPI_DMA_1].flags |= ADI_SPU_PERIPHERAL_FLAGS_MSEC;
	}

	/* Security customization by customer */
	adrv906x_customer_override_spu();

	plat_spu_setup(SPU_A55MMR_BASE, plat_spu_a55mmr_peripherals,
		       SPU_A55MMR_PERIPHERALS_COUNT);
	plat_spu_setup(SPU_ORAN_BASE, plat_spu_oran_peripherals,
		       SPU_ORAN_PERIPHERALS_COUNT);
	plat_spu_setup(SPU_XCORR_BASE, plat_spu_xcorr_peripherals,
		       SPU_XCORR_PERIPHERALS_COUNT);
	plat_spu_setup(SPU_DFE_BASE, plat_spu_dfe_peripherals,
		       SPU_DFE_PERIPHERALS_COUNT);
	plat_spu_setup(SPU_M4_BASE, plat_spu_m4_peripherals,
		       SPU_M4_PERIPHERALS_COUNT);

	if (plat_get_dual_tile_enabled()) {
		boot_device = plat_get_boot_device(); /* Clear msec flags since we don't use it on the secondary, all of the other settings are identical between primary and secondary*/
		if (boot_device == PLAT_BOOT_DEVICE_EMMC_0) {
			plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMMC0SLV].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_MSEC;
		} else if (boot_device == PLAT_BOOT_DEVICE_SD_0) {
			plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_EMMC1SLV].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_MSEC;
		} else if (boot_device == PLAT_BOOT_DEVICE_QSPI_0) {
			plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QUAD_SPI_DMA_0].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_MSEC;
			plat_spu_a55mmr_peripherals[SPU_A55MMR_PERIPH_QUAD_SPI_DMA_1].flags &= ~ADI_SPU_PERIPHERAL_FLAGS_MSEC;
		}
		plat_spu_setup(SEC_SPU_A55MMR_BASE, plat_spu_a55mmr_peripherals,
			       SPU_A55MMR_PERIPHERALS_COUNT);
		plat_spu_setup(SEC_SPU_ORAN_BASE, plat_spu_oran_peripherals,
			       SPU_ORAN_PERIPHERALS_COUNT);
		plat_spu_setup(SEC_SPU_XCORR_BASE, plat_spu_xcorr_peripherals,
			       SPU_XCORR_PERIPHERALS_COUNT);
		plat_spu_setup(SEC_SPU_DFE_BASE, plat_spu_dfe_peripherals,
			       SPU_DFE_PERIPHERALS_COUNT);
		plat_spu_setup(SEC_SPU_M4_BASE, plat_spu_m4_peripherals,
			       SPU_M4_PERIPHERALS_COUNT);
	}
}

void plat_security_setup(void)
{
	INFO("Configuring TrustZone Controllers\n");
	adrv906x_tzc_setup();

	INFO("Configuring System Protection Units\n");
	adrv906x_spu_setup();
}
