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

 @file    map.h
 @brief   Interface header file for abstracting HAL resource mapping
 @author  Steve Quinlan (Design Solutions, Inc.)
 @date    2026-Apr

 ******************************************************************************/

#ifndef __MAP_H
#define __MAP_H

#include "tim.h"
#include "usart.h"
#include "adc.h"
#include "crc.h"
#include "rtc.h"

#include "main.h"
#include "test.h"

#ifndef TEST__DISABLE_IWDG
#include "iwdg.h"
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

// Timer: Timer responsible for PWM generation (used for simulating Zero-Cross on Nucleo)
#define hTIM_SIMPWM htim2
#define TIM_SIMPWM_CH TIM_CHANNEL_1

// Timer: Timer responsible for Zero-Cross detection
#define hTIM_ZC htim3
#define TIM_ZC_FALLEDGE_CH TIM_CHANNEL_1
#define TIM_ZC_RISEEDGE_CH TIM_CHANNEL_2

// Timer: Timer responsible for sleep counting and IWDG refresh
#define hTIM_SLEEP_IWDG_CNT htim4
#define TIM_SLEEP_IWDG_CNT_CH TIM_CHANNEL_1
#define TIM_SLEEP_IWDG_COUNT_FREQ 4000000u  ///< 4.0 MHz

// Timer: Timer responsible for clock testing
#define hTIM_CLKTEST htim10
#define TIM_CLKTEST_CH TIM_CHANNEL_1
#define TIM_CLKTEST_FREQ 4000000u  ///< 4.0 MHz

// Timer: Timer responsible for sleep delay
#define hTIM_SLEEP htim11
#define TIM_SLEEP_CH TIM_CHANNEL_1

// Independent watchdog
#ifndef TEST__DISABLE_IWDG
#define hIWDG_APP hiwdg
#endif

// USARTs
#define hUART_TECH huart2           // UART used for technical messages
#define hUART_DBG huart1            // UART used for debug messages
#define BOOT_UART_HANDLE hUART_DBG  // UART used for ClassB boot messages for startup tests

// I2C
#define hI2C_EXTEEPROM hi2c1

// ADC
#define hADC_APP hadc
#define ADC_INST ADC1

// CRC
#define hCRC_APP hcrc

// RTC
#define hRTC_APP hrtc

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

#ifdef MAP_PROTECTED

#endif /* MAP_PROTECTED */

#endif
