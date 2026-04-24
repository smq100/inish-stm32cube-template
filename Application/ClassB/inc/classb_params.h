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

 @file    classb_params.h
 @brief   Interface header file for Class B parameters
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

  ******************************************************************************/

#ifndef __CLASSB_PARAMS_H
#define __CLASSB_PARAMS_H

#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

// Non volatile backup register to store RAM test result across resets
#define RTC_RAMCHECK_STATUS_REG RTC_BKP_DR0
#define RTC_WDGEXPECT_MARKER_REG RTC_BKP_DR1
#define RTC_WDGEXPECT_MARKERINV_REG RTC_BKP_DR2

#define MAX_FLASH_LATENCY 1

#define CPUTEST_SUCCESS ((uint32_t)0x00000001uL)

// system clock when HSI is applied as PLL source (HSE is not used)
#define SYSCLK_AT_STARTUP (uint32_t)(16000000uL)
#define SYSCLK_AT_RUN (uint32_t)(16000000uL)
#define LSI_MEASURE_PRESCSALER 8uL

// Reserved area for RAM buffer, incl overlap for test purposes
// Don't change this parameter as it is related to physical technology used!
#define RT_RAM_BLOCKSIZE ((uint32_t)6u)

// Min overlap to cover coupling fault from one tested row to the other
#define RT_RAM_BLOCK_OVERLAP ((uint32_t)1u)

// These are the direct and inverted data (pattern) used during the RAM
// test, performed using March C- Algorithm
#define BCKGRND ((uint32_t)0x00000000uL)
#define INV_BCKGRND ((uint32_t)0xFFFFFFFFuL)

// Constants necessary for Flash CRC calculation (ROM_SIZE in byte)
// byte-aligned addresses
#define ROM_START ((uint32_t*)0x08000000uL)
#define ROM_END ((uint32_t*)&_Check_Sum)
#define ROM_SIZE ((uint32_t)ROM_END - (uint32_t)ROM_START)

#define ROM_SIZE_WORDS (uint32_t)(ROM_SIZE / 4u)
#define STEPS_NUMBER ((uint32_t)ROM_SIZE / 0x1000u)
#define FLASH_BLOCK_WORDS (uint32_t)((ROM_SIZE_WORDS) / STEPS_NUMBER)

// Constants necessary for execution initial March test
#define RAM_START ((uint32_t*)0x20000000uL)
#define RAM_END ((uint32_t*)0x20007FFFuL)

// Checksum linker script var value placed at the end of the code section.
// and used for Flash CRC calculation. Value is computed by the linker batch script
// and placed in the .Check_Sum section of the hex file
extern const uint32_t _Check_Sum;
#define REF_CRC32 _Check_Sum

// Stack overflow detection patterns
#define STACK_OVERFLOW_PATTERN_1 0xEEEEEE0EuL
#define STACK_OVERFLOW_PATTERN_2 0xCCCCCC0CuL
#define STACK_OVERFLOW_PATTERN_3 0xBBBBBB0BuL
#define STACK_OVERFLOW_PATTERN_4 0xDDDDDD0DuL

// Constants necessary for execution of transparent run time March tests
#define CLASS_B_START ((uint32_t*)&__class_b_ram_start__)
#define CLASS_B_END ((uint32_t*)&__class_b_ram_rev_end__)

// ADC test limits
#define CLASSB_ADC_VREF_LOW_MV ((int16_t)2700)      // Minimum VDDA in mV
#define CLASSB_ADC_VREF_HIGH_MV ((int16_t)3600)     // Maximum VDDA in mV
#define CLASSB_ADC_TEMP_LOW_DECIC ((int16_t)-400)   // Minimum MCU die temp in deci-°C (-40.0 °C)
#define CLASSB_ADC_TEMP_HIGH_DECIC ((int16_t)1050)  // Maximum MCU die temp in deci-°C (+105.0 °C)

/* Exported macros ------------------------------------------------------------*/

// Clock testing
#define SYS_LIMIT_HIGH(f) ((uint32_t)(((f) / LSI_VALUE) * 1.20f) * LSI_MEASURE_PRESCSALER) /* (LSI_VALUE + %) */
#define SYS_LIMIT_LOW(f) ((uint32_t)(((f) / LSI_VALUE) / 1.20f) * LSI_MEASURE_PRESCSALER)  /* (LSI_VALUE - %) */

/* Exported vars ------------------------------------------------------------ */

// Linker-defined symbols for Class B RAM boundaries
extern const uint32_t __class_b_ram_start__;
extern const uint32_t __class_b_ram_rev_end__;

/* Exported functions ------------------------------------------------------- */

#endif /* __CLASSB_PARAMS_H */
