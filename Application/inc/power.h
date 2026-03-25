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

 @file    power.h
 @brief   Header for supporting power management and sleep
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __POWER_H
#define __POWER_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

#define APP_IRQ_SHORT_TIM 0x0010u  ///< bit flag for within TIM22 short-sleep timer interrupt handler
#define POWER_SLEEP_US_MAX 32767u  ///< max short sleep (us) to stay below one 16-bit LPTIM rollover (~32.8ms @ 2MHz)

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */

extern volatile uint16_t phase_irq;

/* Exported functions ------------------------------------------------------- */

bool POWER_Init(void);
void POWER_Sleep(void);
void POWER_Delay_us(uint16_t Usec, bool Sleep);
void POWER_Delay_100us(uint16_t Value, bool Sleep);
uint32_t POWER_GetSleepTime_ms(void);

#endif /* __POWER_H */
