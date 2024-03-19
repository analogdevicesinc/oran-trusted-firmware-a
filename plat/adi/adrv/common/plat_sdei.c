/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>

#include <plat_helpers.h>

#include <platform_def.h>

/*
 * Translate SDEI entry point to PA, and perform standard ARM entry point
 * validation on it.
 */
int plat_sdei_validate_entry_point(uintptr_t ep, unsigned int client_mode)
{
	uint64_t par, pa;
	u_register_t scr_el3;

	/* Doing Non-secure address translation requires SCR_EL3.NS set */
	scr_el3 = read_scr_el3();
	write_scr_el3(scr_el3 | SCR_NS_BIT);
	isb();

	assert((client_mode == MODE_EL2) || (client_mode == MODE_EL1));
	if (client_mode == MODE_EL2)
		/*
		 * Translate entry point to Physical Address using the EL2
		 * translation regime.
		 */
		ats1e2r(ep);
	else
		/*
		 * Translate entry point to Physical Address using the EL1&0
		 * translation regime, including stage 2.
		 */
		AT(ats12e1r, ep);
	isb();
	par = read_par_el1();

	/* Restore original SCRL_EL3 */
	write_scr_el3(scr_el3);
	isb();

	/* If the translation resulted in fault, return failure */
	if ((par & PAR_F_MASK) != 0)
		return -1;

	/* Extract Physical Address from PAR */
	pa = (par & (PAR_ADDR_MASK << PAR_ADDR_SHIFT));

	/* Perform NS entry point validation on the physical address */
	return plat_validate_ns_entrypoint(pa);
}
