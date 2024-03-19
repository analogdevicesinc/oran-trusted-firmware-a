/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UTILS_H
#define UTILS_H

#define     POWERUP                               (0u)
#define     POWERDOWN                             (1u)
#define     RESET                                 (1u)
#define     CLEAR                                 (0u)
#define     RUN                                   (1u)
#define     DISABLE                               (0u)
#define     ENABLE                                (1u)

uint32_t utils_floorLog2U32(uint32_t x);


#endif /* UTILS_H */
