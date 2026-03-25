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

 @file    task_system.h
 @brief   Interface for overall system functions
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __TASK_SYSTEM_H
#define __TASK_SYSTEM_H

#include "common.h"
#include "task.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

//! Task function prototypes (See task.c/h)
fnTaskInit SYSTEM_Init;
fnTaskExec SYSTEM_Exec;
fnTaskShutdown SYSTEM_Shutdown;
fnTaskTest SYSTEM_Test;

void SYSTEM_FactoryReset(void);
uint32_t SYSTEM_GetBootCount(void);
uint32_t SYSTEM_GetUpTime_MS(void);
uint32_t SYSTEM_GetUpTime_S(void);
uint32_t SYSTEM_GetTick(void);
bool SYSTEM_IsRunning(void);
uint32_t SYSTEM_GetHWVersion(void);
float SYSTEM_GetSysClockFrequency(void);

#ifdef SYSTEM_PROTECTED

void SYSTEM__BeginShutdown(uint16_t Time_ms);

#endif /* SYSTEM_PROTECTED */

#endif /* __TASK_SYSTEM_H */
