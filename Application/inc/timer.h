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

 @file    timer.h
 @brief   Interface for supporting application timers
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#ifndef __TIMER_H
#define __TIMER_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

typedef void (*pfcnTimerCallback)(void);     //!< Timer callback prototype
typedef uint32_t (*pfcnTimerGetTick)(void);  //!< Timer get-tick prototype

/* Exported constants --------------------------------------------------------*/

#define TIMER_PERIOD_MS 1  //!< timer period in ms

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

uint16_t TIMER_Init(void);
uint16_t TIMER_GetPeriod(void);
void TIMER_DelayMs(uint32_t Delay_ms);
uint32_t TIMER_GetTick(void);
uint32_t TIMER_GetElapsed_ms(uint32_t RefTime_ms);
float TIMER_GetElapsed_s(uint32_t RefTime_ms);
void TIMER_ProcessCallbacks(void);
bool TIMER_RegisterTickGetter(pfcnTimerGetTick PfcnFunction);
bool TIMER_RegisterCallback(pfcnTimerCallback PfcnFunction);
void TIMER_UnregisterCallback(pfcnTimerCallback PfcnFunction);
void TIMER_SysTick_Handler(void);

#ifdef TIMER_PROTECTED

#endif /* TIMER_PROTECTED */

#endif /* __TIMER_H */
