/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_WDT_H
#define PLAT_WDT_H
/**
 * @brief      	Start watchdog timer
 */
void plat_secure_wdt_start(void);

/**
 * @brief      	Stop watchdog timer
 */
void plat_secure_wdt_stop(void);

/**
 * @brief      	Reload the watchdog timer
 *
 * @param [in]  seconds - second count to reset the watchdog timer
 */
void plat_secure_wdt_refresh(uint32_t seconds);

/**
 * @brief      	Ping/pet watchdog timer
 */
void plat_secure_wdt_ping(void);

#endif
