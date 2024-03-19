/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

.global plat_secondary_image
.global plat_secondary_image_end

.align 4
plat_secondary_image:
	.incbin TOSTRING(SECONDARY_IMAGE_BIN)
plat_secondary_image_end:
