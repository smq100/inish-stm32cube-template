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

 @file    task.h
 @brief   Interface of the TASK superloop
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#ifndef __TASK_H
#define __TASK_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

// System tasks (_Defaults[] order must match)
// Order of tasks defines intialization and execution order
// Tasks dependent on other tasks must be placed after those tasks
typedef enum
{
  eTask_System,   ///< System
  eTask_Serial,   ///< Serial
  eTask_DAQ,      ///< DAQ
  eTask_Tech,     ///< Technician
  eTask_Buttons,  ///< Buttons
  eTask_LED,      ///< LED
  eTask_ClassB,   ///< ClassB
  eTask_App,      ///< Main Application
  eTask_NUM       ///< Number of tasks. Must be last item
} tTask;

// Metrics for overall loop
// Note: It's possible for the uint32_t values to overflow if the system runs for a very long time
//       2^32ms ~= ~49.7 days, and task metrics are probably disabled in production build so this is
//       acceptable vs the numerous FLOPs for non-FPU systems
typedef struct
{
  uint64_t Loops;       ///< Quantity of times the loop has completed
  float TimeTotal_s;    ///< Loop execution time total in seconds
  uint32_t TimeMin_ms;  ///< Loop execution time min in ms
  uint32_t TimeMax_ms;  ///< Loop execution time max in ms
  uint32_t TimeAvg_ms;  ///< Loop execution time avg in ms
} tLoopMetrics;

//! Metrics for tasks
// Note: It's possible for the uint32_t values to overflow if the system runs for a very long time
//       2^32ms ~= ~49.7 days, and task metrics are probably disabled in production build so this is
//       acceptable vs the numerous FLOPs for non-FPU systems
typedef struct
{
  // Metrics for each task
  const char* Name;     ///< Name of task
  uint32_t Delay_ms;    ///< Delay of task
  uint64_t Switches;    ///< Quantity of times the process exec function has been called
  float TimeTotal_s;    ///< Task execution time total in seconds
  uint32_t TimeMin_ms;  ///< Loop execution time min in ms
  uint32_t TimeMax_ms;  ///< Loop execution time max in ms
  uint32_t TimeAvg_ms;  ///< Loop execution time avg in ms
} tTaskMetrics;

//! Overall scheduler load percentages based on loop runtime
typedef struct
{
  float Busy_pct;      ///< Time spent inside task Exec methods
  float Idle_pct;      ///< Time spent in scheduler idle hook
  float Overhead_pct;  ///< Time spent in scheduler overhead outside tasks and idle
} tTaskLoad;

//! Task prototypes
typedef bool(fnTaskInit)(void);
typedef bool(fnTaskExec)(void);
typedef bool(fnTaskShutdown)(void);
typedef bool(fnTaskTest)(void);

/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

fnTaskInit TASK_Init;
fnTaskExec TASK_Exec;

bool TASK_IsInitialized(tTask Task);
bool TASK_AreAllInitialized(void);
bool TASK_AreAllShutdown(void);
void TASK_SetDelay(tTask Task, uint32_t Delay);
bool TASK_GetStatus(tTask Task, tLoopMetrics* Loop, tTaskMetrics* Metrics);
bool TASK_GetLoad(tTaskLoad* Load);
const char* TASK_GetName(tTask Task);
bool TASK_GetLastRunningBeforeReset(tTask* Task, const char** Name);
void TASK_ClearLastRunningBeforeReset(void);
uint32_t TASK_PrintStatus(tTask Task, bool Summary, char* Buf, uint32_t BufSize);
uint32_t TASK_GetErrorField(void);
void TASK_ResetDefaults(void);

#ifdef TASK_PROTECTED
void TASK__BeginShutdown(uint16_t Time_ms);

#endif /* TASK_PROTECTED */

#endif /* __TASK_H */
