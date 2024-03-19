/*
 * Copyright (c) 2023, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <plat_interrupts.h>

/*TODO: Find a better way to store and lookup the interrupt handlers*/
interrupt_type_handler_t type_el3_interrupt_table[MAX_INTR_EL3];

int plat_request_intr_type_el3(uint32_t id, interrupt_type_handler_t handler)
{
	/* Validate 'handler' and 'id' parameters */
	if (!handler || id >= MAX_INTR_EL3)
		return -EINVAL;

	/* Check if a handler has already been registered */
	if (type_el3_interrupt_table[id])
		return -EALREADY;

	type_el3_interrupt_table[id] = handler;

	return 0;
}

#if EL3_EXCEPTION_HANDLING
int plat_interrupt_handler(uint32_t intr_raw, uint32_t flags, void *handle, void *cookie)
{
	int ret = 0;
	interrupt_type_handler_t handler;

	handler = type_el3_interrupt_table[intr_raw];
	if (handler != NULL)
		ret = handler(intr_raw, flags, handle, cookie);

	plat_ic_end_of_interrupt(intr_raw);
	return ret;
}
#else
uint64_t plat_interrupt_handler(uint32_t intr_raw, uint32_t flags, void *handle, void *cookie)
{
	int ret = 0;
	uint32_t int_id;
	interrupt_type_handler_t handler;

	int_id = plat_ic_get_pending_interrupt_id();
	handler = type_el3_interrupt_table[int_id];
	if (handler != NULL)
		ret = handler(int_id, flags, handle, cookie);

	plat_ic_clear_interrupt_pending(int_id);
	plat_ic_end_of_interrupt(int_id);
	return ret;
}
#endif
