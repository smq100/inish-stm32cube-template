/*!
******************************************************************************
* @copyright Copyright (c) 2026 Inish Corporation
*
* SPDX-License-Identifier: MIT
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
******************************************************************************

 @file    common.h
 @brief   Common system interface includes
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __COMMON_H
#define __COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "resources.h"
#include "test.h"

/* Exported types ------------------------------------------------------------*/

//! Available tDataValue data types. Used with tDataValue union to allow passing of different data
// types in a generic way
typedef enum DataType
{
  eDataType_Float,
  eDataType_Bool,
  eDataType_U32,
  eDataType_V32,
  eDataType_S32,
  eDataType_U16,
  eDataType_S16,
  eDataType_U8,
  eDataType_S8,
  eDataType_Enum,
} tDataType;

//! Union of data values that are used throughout the application where data of differing types
// may be passed from module to module. Used with tDataType enum to allow passing of different
// data types in a generic way
typedef union DataValue
{
  float Float;            //!< Float value
  bool Bool;              //!< Bool value
  uint32_t U32;           //!< Unsigned int
  volatile uint32_t V32;  //!< Volatile unsigned int
  int32_t S32;            //!< Signed int
  uint16_t U16;           //!< Unsigned int
  int16_t S16;            //!< Signed int
  uint8_t U8;             //!< Unsigned int
  int8_t S8;              //!< Signed int
  uint8_t Enum;           //!< Enum value stored as uint32
} tDataValue;

typedef struct
{
  GPIO_TypeDef* Port;  //!< HAL port
  uint32_t Pin;        //!< Pin designation on Port
} tGPIOConfig;

//! System UARTs
typedef enum
{
  eSerial_Debug,  //!< Debug serial port and UART
  eSerial_Tech,   //!< Tech serial port and UART
  NUM_SERIAL_PORTS
} tSerialPort;

typedef int16_t tHCTN_Analog;  ///< abstract our type used for ANALOG readings

typedef int32_t tHCTN_Status;  ///< abstract our type used for STATUS/ERROR returns

typedef enum
{
  HCTN_ERR_TIMEOUT = -4,  ///< action timed out with no result
  HCTN_ERR_PARAM,         ///< function input parameter was bad
  HCTN_ERR_HANDLE,        ///< handle or object index was bad
  HCTN_ERR_GENERAL,       ///< generic uncategorized result
  HCTN_SUCCESS = 0,       ///< result was good or as expected
  HCTN_CHANGED            ///< Class-B memory SET was a CHANGE in previous value
} HCTN_Status_Code_t;

/* Exported constants --------------------------------------------------------*/

#define CRITICAL_SECTION_START() __disable_irq()
#define CRITICAL_SECTION_END() __enable_irq()

#define NULLPTR ((void*)0)

#define HW_IS_NUCLEO  ///< define if code compiled for white NUCLEO eval board

#define HCTN_STARTUP_STEP_DELAY 250  ///< the msec delay between START-UP tests

/* for structures with a safety 'magic' values */
#define HCTN_MAGIC_MASK 0xFFFF  ///< define number of bits in a MAGIC value

/* Analogs are 2-bit 0-4096 only */
#define HCTN_NAN -29999  ///< define a fake not-a-number used with signed-short (16-bit) value

/* Exported macros -----------------------------------------------------------*/

#define assert_always() assert_failed((uint8_t*)__FILE__, __LINE__);

/* Exported vars -------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

#endif /* __COMMON_H */
