/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#if EL3_EXCEPTION_HANDLING
#include <services/sdei.h>
#endif
#include <adrv906x_def.h>
#include <adrv906x_gpint.h>
#include <adrv906x_sdei_events.h>

static uint64_t gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie, uintptr_t gpint_addr)
{
#if EL3_EXCEPTION_HANDLING
	int32_t status = 0;
#endif
	uint64_t ret = 0;
	uint64_t mask = 1;
	struct gpint_settings settings;

	adrv906x_gpint_get_status(gpint_addr, &settings);
	for (int i = 0; i < GPINT_INTS_PER_WORD; i++) {
		if ((settings.upper_word & mask) >> i) { /*If GPINT event is active*/
			if (adrv906x_gpint_is_nonsecure(true, mask)) {
#if EL3_EXCEPTION_HANDLING
				if (gpint_addr == DIG_CORE_BASE)
					status = sdei_dispatch_event(GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD);
				else
					status = sdei_dispatch_event(SEC_GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD);

				if (status != 0) {
					if (gpint_addr == DIG_CORE_BASE)
						ERROR("sdei_dispatch_event for event %d returned %d\n", GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD, status);
					else
						ERROR("sdei_dispatch_event for event %d returned %d\n", SEC_GPINT_DEFAULT_SDEI_EVENT + i + GPINT_INTS_PER_WORD, status);
					ret = 1;
				}
#else
				ERROR("Unhandled GPINT interrupt: %d", i);
#endif
			} else {
				ERROR("Unhandled GPINT interrupt: %d", i + GPINT_INTS_PER_WORD);
			}
		}

		if ((settings.lower_word & mask) >> i) { /*If GPINT event is active*/
			if (adrv906x_gpint_is_nonsecure(false, mask)) {
#if EL3_EXCEPTION_HANDLING
				if (gpint_addr == DIG_CORE_BASE)
					status = sdei_dispatch_event(GPINT_DEFAULT_SDEI_EVENT + i);
				else
					status = sdei_dispatch_event(SEC_GPINT_DEFAULT_SDEI_EVENT + i);

				if (status != 0) {
					if (gpint_addr == DIG_CORE_BASE)
						ERROR("sdei_dispatch_event for event %d returned %d\n", GPINT_DEFAULT_SDEI_EVENT + i, status);
					else
						ERROR("sdei_dispatch_event for event %d returned %d\n", SEC_GPINT_DEFAULT_SDEI_EVENT + i, status);
					ret = 1;
				}
#else
				ERROR("Unhandled GPINT interrupt: %d", i);
#endif
			} else {
				ERROR("Unhandled GPINT interrupt: %d", i);
			}
		}

		mask = mask << 1;
	}

	return ret;
}

uint64_t primary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	return gpint_handler(id, flags, handle, cookie, DIG_CORE_BASE);
}

uint64_t secondary_gpint_handler(uint32_t id, uint32_t flags, void *handle, void *cookie)
{
	return gpint_handler(id, flags, handle, cookie, SEC_DIG_CORE_BASE);
}
