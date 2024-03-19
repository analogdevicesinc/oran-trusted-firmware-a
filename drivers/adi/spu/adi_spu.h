/*
 * Copyright (c) 2023, Analog Devices Incorporated, All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ADI_SPU_INTERNAL_H
#define ADI_SPU_INTERNAL_H

#define SPU_CTL_OFFSET 0x000
#define SPU_STAT_OFFSET 0x004
#define SPU_MSG_OFFSET 0x100
#define SPU_WP_OFFSET 0x400
#define SPU_SECURECTL_OFFSET 0x840
#define SPU_SECURECHK_OFFSET 0x84c
#define SPU_SECUREMSG_OFFSET 0x900
#define SPU_SECUREC_OFFSET 0x980
#define SPU_SECUREP_OFFSET 0xa00

#define SPU_CTL_GLOCK_MASK 0xFF
#define SPU_CTL_GLOCK_SHIFT 0
#define SPU_CTL_GLOCK_DISABLED 0xAD
#define SPU_CTL_PINTEN_MASK 0x1
#define SPU_CTL_PINTEN_SHIFT 14
#define SPU_CTL_WPLCK_MASK 0x1
#define SPU_CTL_WPLCK_SHIFT 16

#define SPU_STAT_GLOCK_MASK 0x1
#define SPU_STAT_GLOCK_SHIFT 0
#define SPU_STAT_VIRQ_MASK 0x1
#define SPU_STAT_VIRQ_SHIFT 12
#define SPU_STAT_ADRERR_MASK 0x1
#define SPU_STAT_ADRERR_SHIFT 30
#define SPU_STAT_LWERR_MASK 0x1
#define SPU_STAT_LWERR_SHIFT 31

#define SPU_WP_CORE_MASK 0xFFFF
#define SPU_WP_CORE_SHIFT 0
#define SPU_WP_SYS_MASK 0xFFFF
#define SPU_WP_SYS_SHIFT 16

#define SPU_SECURECTL_SSECCLR_MASK 0x1
#define SPU_SECURECTL_SSECCLR_SHIFT 4
#define SPU_SECURECTL_MSECCLR_MASK 0x1
#define SPU_SECURECTL_MSECCLR_SHIFT 5
#define SPU_SECURECTL_SINTEN_MASK 0x1
#define SPU_SECURECTL_SINTEN_SHIFT 14
#define SPU_SECURECTL_SCRLCK_MASK 0x1
#define SPU_SECURECTL_SCRLCK_SHIFT 16

#define SPU_SECUREC_CSEC_MASK 0xFF
#define SPU_SECUREC_CSEC_SHIFT 0

#define SPU_SECUREP_SSEC_MASK 0x1
#define SPU_SECUREP_SSEC_SHIFT 0
#define SPU_SECUREP_MSEC_MASK 0x1
#define SPU_SECUREP_MSEC_SHIFT 1

#define SPU_REG_FIELD_GET(reg, mask, shift) \
	(reg & (mask << shift) >> shift)
#define SPU_REG_FIELD_SET(reg, mask, shift, x) \
	((reg & ~(mask << shift)) | ((x & mask) << shift))

/* SPU Control Register (RW) */
#define SPU_CTL_REG(base)               ((base) + SPU_CTL_OFFSET)
/* SPU Status Register (RW) */
#define SPU_STAT_REG(base)              ((base) + SPU_STAT_OFFSET)
/* Message Registers 0-1 are a non-secure version of the SPU_SECUREMSGn register
 * that can be read by non-secure masters.
 */
#define SPU_MSG_REG(base, n)            ((base) + SPU_MSG_OFFSET + (4 * n))
/* SPU Write Protect Register(s) (RW) */
#define SPU_WP_REG(base, n)             ((base) + SPU_WP_OFFSET + (4 * (n)))
/* Secure Control Register. (RW) */
#define SPU_SECURECTL_REG(base)         ((base) + SPU_SECURECTL_OFFSET)
/* Secure Check Register is special security register that allows a master to
 * check its own security status. Reads of this register by a secure master will
 * return 0xFFFFFFFF while reads by a non-secure master will return 0x00000000.
 */
#define SPU_SECURECHK_REG(base)         ((base) + SPU_SECURECHK_OFFSET)
/* Secure Message Registers 0-1 are used to pass a message to the debug host.
 * This register can be read or written by secure masters only. The contents are
 * available for read-only by non-secure masters through the non-secure version.
 */
#define SPU_SECUREMSG_REG(base, n)      ((base) + SPU_SECUREMSG_OFFSET + (4 * n))
/* Secure Core Register(s) */
#define SPU_SECUREC_REG(base, n)        ((base) + SPU_SECUREC_OFFSET + (4 * (n)))
/* Secure Peripheral Register(s) */
#define SPU_SECUREP_REG(base, n)        ((base) + SPU_SECUREP_OFFSET + (4 * (n)))

/* Slave Secure: SSEC controls whether the peripheral is protected from non-
 * secure transactions. When clear (=0), the security status of the transaction
 * is ignored. When set (=1), only secure transactions are allowed to access the
 * address space of the peripheral and non-secure transactions are blocked.
 */
#define SPU_SECUREP_REG_GET_SSEC(reg) \
	SPU_REG_FIELD_GET(reg, SPU_SECUREP_SSEC_MASK, SPU_SECUREP_SSEC_SHIFT)
#define SPU_SECUREP_REG_SET_SSEC(reg) \
	SPU_REG_FIELD_SET(reg, SPU_SECUREP_SSEC_MASK, SPU_SECUREP_SSEC_SHIFT, 1)
#define SPU_SECUREP_REG_CLR_SSEC(reg) \
	SPU_REG_FIELD_SET(reg, SPU_SECUREP_SSEC_MASK, SPU_SECUREP_SSEC_SHIFT, 0)

/* Master Secure: MSEC controls whether the peripheral generates secure
 * transactions as a master. When clear (=0), the peripheral generates
 * non-secure transactions as a master (if applicable). When set (=1), the
 * peripheral generates secure transactions as a master.
 */
#define SPU_SECUREP_REG_GET_MSEC(reg) \
	SPU_REG_FIELD_GET(reg, SPU_SECUREP_MSEC_MASK, SPU_SECUREP_MSEC_SHIFT)
#define SPU_SECUREP_REG_SET_MSEC(reg) \
	SPU_REG_FIELD_SET(reg, SPU_SECUREP_MSEC_MASK, SPU_SECUREP_MSEC_SHIFT, 1)
#define SPU_SECUREP_REG_CLR_MSEC(reg) \
	SPU_REG_FIELD_SET(reg, SPU_SECUREP_MSEC_MASK, SPU_SECUREP_MSEC_SHIFT, 0)

/* Global Lock: The value in the Global Lock Field determines whether glob_lock
 * is enabled or not. Global Lock is disabled if the value of the field is 0xAD
 * (default value), otherwise it is enabled.
 */
#define SPU_CTL_GET_GLOCK(reg) \
	SPU_REG_FIELD_GET(reg, SPU_CTL_GLOCK_MASK, SPU_CTL_GLOCK_SHIFT)
#define SPU_CTL_SET_GLOCK(reg) \
	SPU_REG_FIELD_SET(reg, SPU_CTL_GLOCK_MASK, SPU_CTL_GLOCK_SHIFT, 0xAD)
#define SPU_CTL_CLR_GLOCK(reg) \
	SPU_REG_FIELD_SET(reg, SPU_CTL_GLOCK_MASK, SPU_CTL_GLOCK_SHIFT, 0x00)

/* Secure Lock: When set in combination with Global Lock, writes to the Security
 * Configuration Registers (SPU_SECURECFGn) will be blocked and will return an
 * error.
 */
#define SPU_SECURECTL_GET_SCRLCK(reg) \
	SPU_REG_FIELD_GET(reg, SPU_SECURECTL_SCRLCK_MASK, SPU_SECURECTL_SCRLCK_SHIFT)
#define SPU_SECURECTL_SET_SCRLCK(reg) \
	SPU_REG_FIELD_SET(reg, SPU_SECURECTL_SCRLCK_MASK, SPU_SECURECTL_SCRLCK_SHIFT, 1)
#define SPU_SECURECTL_CLR_SCRLCK(reg) \
	SPU_REG_FIELD_SET(reg, SPU_SECURECTL_SCRLCK_MASK, SPU_SECURECTL_SCRLCK_SHIFT, 0)

#endif /* ADI_SPU_INTERNAL_H */
