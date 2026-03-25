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

 @file    task_led.h
 @brief   Interface of the LED processing
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __TASK_LED_H
#define __TASK_LED_H

#include "common.h"
#include "task.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  eLED_Status,  ///< Status LED
  eLED_NUM      // Must be last
} tLED;

/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

//! Task function prototypes (See task.c/h)
fnTaskInit LED_Init;
fnTaskExec LED_Exec;
fnTaskShutdown LED_Shutdown;
fnTaskTest LED_Test;

bool LED_OnOff(tLED led, bool On, const char* Key);
bool LED_Toggle(tLED led, const char* Key);
bool LED_Flash(tLED led, float Secs, int32_t Qty, const char* Key);
bool LED_Pulse(tLED led, float Secs, int32_t Qty, int32_t Pulses, const char* Key);
bool LED_IsOn(tLED led);
void LED_Lock(tLED led, bool Lock, const char* Key);
bool LED_IsLocked(tLED led);
bool LED_IsTesting(void);

#ifdef LED_PROTECTED

#endif /* LED_PROTECTED */

#endif /* __TASK_LED_H */
