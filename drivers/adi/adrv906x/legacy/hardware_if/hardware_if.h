/*
 * Copyright (c) 2022, Analog Devices Incorporated - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HARDWARE_IF_H
#define HARDWARE_IF_H

#include <stdbool.h>

/**
 *******************************************************************************
 * Function: READ_BF_8BIT_REG
 *
 * @brief       Read 8 bit H/W reg
 *
 * @details     This inline function reads a bit field from an 8 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  shift - Number of bits to shift result down
 * @param [in]  mask - Mask for final result
 *
 * @return      Data read back from H/W
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline uint8_t READ_BF_8BIT_REG(volatile const uint8_t *addr,
				       uint8_t shift,
				       uint8_t mask
				       )
{
	return (*addr & mask) >> shift;
}

/**
 *******************************************************************************
 * Function: READ_BF_16BIT_REG
 *
 * @brief       Read 16 bit H/W reg
 *
 * @details     This inline function reads a bit field from an 16 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  shift - Number of bits to shift result down
 * @param [in]  mask - Mask for final result
 *
 * @return      Data read back from H/W
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline uint16_t READ_BF_16BIT_REG(volatile const uint16_t *addr,
					 uint8_t shift,
					 uint16_t mask
					 )
{
	return (*addr & mask) >> shift;
}


/**
 *******************************************************************************
 * Function: READ_BF_32BIT_REG
 *
 * @brief       Read 32 bit H/W reg
 *
 * @details     This inline function reads a bit field from an 32 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  shift - Number of bits to shift result down
 * @param [in]  mask - Mask for final result
 *
 * @return      Data read back from H/W
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline uint32_t READ_BF_32BIT_REG(volatile const uint32_t *addr,
					 uint8_t shift,
					 uint32_t mask
					 )
{
	return (*addr & mask) >> shift;
}

/**
 *******************************************************************************
 * Function: WRITE_BF_8BIT_REG
 *
 * @brief       Write 8 bit H/W reg
 *
 * @details     This inline function write a bit field to an 8 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  shift - Number of bits to shift result down
 * @param [in]  mask - Mask for final result
 * @param [in]  val - Data to be written
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline void WRITE_BF_8BIT_REG(volatile uint8_t *addr,
				     uint8_t shift,
				     uint8_t mask,
				     uint8_t val
				     )
{
	*addr = (uint8_t)(*addr & (uint8_t)(~mask)) | (uint8_t)((uint8_t)(val << shift) & mask);
}

/**
 *******************************************************************************
 * Function: WRITE_BF_16BIT_REG
 *
 * @brief       Write 16 bit H/W reg
 *
 * @details     This inline function write a bit field to an 16 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  shift - Number of bits to shift result down
 * @param [in]  mask - Mask for final result
 * @param [in]  val - Data to be written
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline void WRITE_BF_16BIT_REG(volatile uint16_t *addr,
				      uint8_t shift,
				      uint16_t mask,
				      uint16_t val
				      )
{
	*addr = (uint16_t)(*addr & (uint16_t)(~mask)) | (uint16_t)((uint16_t)(val << shift) & mask);
}

/**
 *******************************************************************************
 * Function: WRITE_BF_32BIT_REG
 *
 * @brief       Write 32 bit H/W reg
 *
 * @details     This inline function write a bit field to an 32 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  shift - Number of bits to shift result down
 * @param [in]  mask - Mask for final result
 * @param [in]  val - Data to be written
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline void WRITE_BF_32BIT_REG(volatile uint32_t *addr,
				      uint8_t shift,
				      uint32_t mask,
				      uint32_t val
				      )
{
	*addr = (*addr & (~mask)) | ((val << shift) & mask);
}

/**
 *******************************************************************************
 * Function: WRITE_8BIT_REG
 *
 * @brief       Write 8 bit H/W reg as with a whole register value
 *
 * @details     This inline function writes a  whole register value to an 8 bit
 *              reg.
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to write.
 * @param [in]  val - Data to be written to the register
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline void WRITE_8BIT_REG(volatile uint8_t *addr, uint8_t val)
{
	*addr = val;
	return;
}
/**
 *******************************************************************************
 * Function: WRITE_16BIT_REG
 *
 * @brief       Write 16 bit H/W reg
 *
 * @details     This inline function write a whole register value to an 16 bit
 *              reg.
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  val - Data to be written
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline void WRITE_16BIT_REG(volatile uint16_t *addr, uint16_t val)
{
	*addr = val;
}

/**
 *******************************************************************************
 * Function: WRITE_32BIT_REG
 *
 * @brief       Write 32 bit H/W reg
 *
 * @details     This inline function write a whole register value to an 32 bit
 *              reg.
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 * @param [in]  val - Data to be written
 *
 * @return      None
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline void WRITE_32BIT_REG(volatile uint32_t *addr, uint32_t val)
{
	*addr = val;
}

/**
 *******************************************************************************
 * Function: READ_8BIT_REG
 *
 * @brief       Read 8 bit H/W reg
 *
 * @details     This inline function reads the register value from an 8 bit reg
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 *
 * @return      whole register value read back from H/W
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline uint8_t READ_8BIT_REG(volatile const uint8_t *addr)
{
	return *addr;
}
/**
 *******************************************************************************
 * Function: READ_16BIT_REG
 *
 * @brief       Read 16 bit H/W reg
 *
 * @details     This inline function reads the whole register value from an 16
 *              bit reg.
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 *
 * @return      Data read back from the 16-bit regsiter.
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline uint16_t READ_16BIT_REG(volatile const uint16_t *addr)
{
	return *addr;
}


/**
 *******************************************************************************
 * Function: READ_32BIT_REG
 *
 * @brief       Read 32 bit H/W reg
 *
 * @details     This inline function reads the whole register value from an 32
 *              bit reg.
 *
 * Parameters:
 * @param [in]  addr - Pointer to address to be read
 *
 * @return      Data read back from the 32-bit regsiter.
 *
 * Reference to other related functions
 * @sa
 *
 *
 *******************************************************************************
 */
static inline uint32_t READ_32BIT_REG(volatile const uint32_t *addr)
{
	return *addr;
}


#endif /* HARDWARE_IF_H */
