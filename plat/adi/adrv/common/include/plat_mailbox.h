/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_MAILBOX_H
#define PLAT_MAILBOX_H

/*
 * Mailbox to control the secondary cores. All secondary cores are held in a wait
 * loop in cold boot. To release them perform the following steps (plus any
 * additional barriers that may be needed):
 *
 *     uint64_t *entrypoint = (uint64_t *)PLAT_TM_ENTRYPOINT;
 *     *entrypoint = ADDRESS_TO_JUMP_TO;
 *
 *     uint64_t *mbox_entry = (uint64_t *)PLAT_TM_HOLD_BASE;
 *     mbox_entry[cpu_id] = PLAT_TM_HOLD_STATE_GO;
 *
 *     sev();
 */

/* Place mailbox at the beginning of trusted SRAM (location is platform-defined) */
#define PLAT_TRUSTED_MAILBOX_BASE  SHARED_RAM_BASE

/* The secure entry point to be used on warm reset by all CPUs. */
#define PLAT_TM_ENTRYPOINT         PLAT_TRUSTED_MAILBOX_BASE
#define PLAT_TM_ENTRYPOINT_SIZE    ULL(8)

/* Hold entries for each CPU. */
#define PLAT_TM_HOLD_BASE          (PLAT_TM_ENTRYPOINT + \
				    PLAT_TM_ENTRYPOINT_SIZE)
#define PLAT_TM_HOLD_ENTRY_SIZE    ULL(8)
#define PLAT_TM_HOLD_SIZE          (PLAT_TM_HOLD_ENTRY_SIZE * \
				    PLATFORM_CORE_COUNT)

#define PLAT_TRUSTED_MAILBOX_SIZE  (PLAT_TM_ENTRYPOINT_SIZE + \
				    PLAT_TM_HOLD_SIZE)

#define PLAT_TM_HOLD_STATE_WAIT    ULL(0)
#define PLAT_TM_HOLD_STATE_GO      ULL(1)
#define PLAT_TM_HOLD_STATE_BSP_OFF ULL(2)

#endif /* PLAT_MAILBOX_H */
