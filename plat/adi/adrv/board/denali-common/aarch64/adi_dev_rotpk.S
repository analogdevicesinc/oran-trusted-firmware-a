/*
 * Copyright (c) 2024, Analog Devices Incorporated. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

	/* Development key for ADRV906X reference designs only */
	/* DO NOT USE IN PRODUCTION SOFTWARE */
	.global adi_rotpk_hash
	.global adi_rotpk_hash_end
adi_rotpk_hash:
	/* DER header */
	.byte 0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48
	.byte 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
	/* SHA256 */
        .incbin "plat/adi/adrv/board/denali-common/rotpk_sha256.bin"
adi_rotpk_hash_end:
