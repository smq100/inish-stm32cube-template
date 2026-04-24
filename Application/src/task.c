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

 @file    task.c
 @brief   Implementation of the TASK superloop
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************

 @verbatim

# The Superloop Tasks Architecture

This software does not employ a real-time operating system. It uses a superloop architecture using a modular and
consistent interface along with features that provide precise execution control and tuning metrics.

The basic element of the task architecture is referred to as a *task*. Tasks are similar to threads in an RTOS.
Each task is implemented in its own implementation and header file (.c/h) and, at a minimum,
includes five functions of the following prototypes:

* typedef bool (fnTaskInit)(void)
* typedef bool (fnTaskExec)(void)
* typedef bool (fnTaskShutdown)(void)
* typedef bool (fnTaskTest)(void)

## Initialization Function
The initialization function is called at system startup to initialize the task.

Each task must implement an initialization function, and that function must be included in the tTaskConfig _Config data
structures. The initialization function must return true, otherwise the associated will be disabled. Normally, all tasks
must return true.

## Exec Function
The executive function is the main processing function of the task. It is called repeatedly throughout the application
lifetime.

Each task must implement an exec function, and that function must be included in the tTaskConfig _Config data
structures. The exec function must return true.

## Shutdown Function
The shutdown function is called when the system is powering down and allows for the attempted graceful shutdown of
tasks.

Each task must implement a shutdown function, and that function must be included in the tTaskConfig _Config data
structures. Tasks that need no shutdown processing simply return true. Any shutdown function that returns false will be
called again in TASK_AreAllShutdown() until it returns a true.

## Test Function
The test function is called at system startup to test the task during the power-on self-test (POST).

Each task must implement a test function, and that function must be included in the tTaskConfig _Config data structures.
The test function must return true, otherwise the associated task will be disabled.

## Task Frequency
Not every task needs to be serviced at every run of the superloop. The tTaskConfig structure contains the variable Delay
that may be set to a value greater than zero to lower the frequency in which the executive is called. When Delay = 0,
the exec function of the associated task will be called at every loop. Otherwise the task executive will be called every
‘Delay’ times in the loop.

## Task Metrics
Simple tasks metrics are available to provide data to assist in debugging and tuning task execution. When the module
variable _MetricsEnabled = true, the number of executions (aka context switches in a RTOS environment) as well as the
time spent in each task execution.

## Idle Hook
When enabled via the _IdleHookEnabled const bool flag, the scheduler detects when no task is ready to run in a superloop
pass and enters a low-power sleep state via POWER_Sleep() instead of busy-polling. This reduces power consumption during
periods when tasks are not due for execution.

The idle hook operates by:
1. Tracking whether any task executed during the current superloop pass via the taskRan flag
2. Computing the time until the next task deadline via _GetTimeUntilNextTask_ms()
3. Entering POWER_Sleep() if time is available and the system is not shutting down
4. Accumulating idle time separately from task execution time for metrics purposes

The idle hook can be disabled at runtime by setting _IdleHookEnabled = false without requiring a rebuild.

## Load Measurement API
The TASK_GetLoad() function provides scheduler load metrics as percentages:
* Busy_pct: Percentage of time spent executing tasks
* Idle_pct: Percentage of time in POWER_Sleep() (idle hook only)
* Overhead_pct: Percentage of time in scheduler overhead (loop management)

These percentages sum to 100% when metrics are enabled. Load information is included in the task status summary output
when printed.

 @endverbatim
******************************************************************************/

#define TASK_PROTECTED

#include <assert.h>

#include "main.h"

#include "task.h"

#include "task_system.h"
#include "task_serial.h"
#include "task_daq.h"
#include "task_classb.h"
#include "task_app.h"
#include "task_tech.h"
#include "task_buttons.h"
#include "task_led.h"
#include "classb.h"
#include "classb_vars.h"
#include "timer.h"
#include "power.h"
#include "util.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

//! Task configuration data structure
typedef struct
{
  bool Enabled;              //!< Task enabled when true
  fnTaskInit* Init;          //!< Task initialization method
  fnTaskExec* Exec;          //!< Task executive method
  fnTaskShutdown* Shutdown;  //!< Task shutdown method
  fnTaskTest* Test;          //!< Task POST test method
  uint32_t Delay_ms;         //!< Tick priority
  uint32_t TestDelay;        //!< Testing initialization delay milliseconds
  const char* Name;          //!< Module name used for logging
} tTaskConfig;

//! Task runtime data structure
typedef struct
{
  bool Initialized;         //!< True when process is initialized
  uint32_t Timestamp;       //!< The active tick value
  tTaskMetrics Metrics;     //!< Task metrics
  bool Shutdown;            //!< True when the process has been properly shutdown
  uint32_t DebugPeriod_ms;  //!< Period to callback debug function (milliseconds)
} tTaskRuntime;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "TASK";         //!< Module name used for debug logging
static const uint8_t _NumTasks = eTask_NUM;  //!< Number of tasks
static const bool _MetricsEnabled = true;    //!< True to collect task metrics
static const bool _IdleHookEnabled = true;   //!< True to enable scheduler idle hook sleep path

// Delays were chosen, if possible, to distribute servicing of tasks so that tasks are
// not all serviced on the same loop iteration by using prime numbers. Some Primes:
// 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97
// 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199
// 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331
//! Task configuration defaults (order must match tTaskConfig enum)
static const tTaskConfig _Defaults[] = {
  { true, SYSTEM_Init, SYSTEM_Exec, SYSTEM_Shutdown, SYSTEM_Test, 23, 0, "SYS" },   // eTask_System
  { true, SERIAL_Init, SERIAL_Exec, SERIAL_Shutdown, SERIAL_Test, 7, 0, "SER" },    // eTask_Serial
  { true, DAQ_Init, DAQ_Exec, DAQ_Shutdown, DAQ_Test, 9, 0, "DAQ" },                // eTask_DAQ
  { true, TECH_Init, TECH_Exec, TECH_Shutdown, TECH_Test, 13, 0, "TECH" },          // eTask_Tech
  { true, BUTTON_Init, BUTTON_Exec, BUTTON_Shutdown, BUTTON_Test, 11, 0, "BTN" },   // eTask_Buttons
  { true, LED_Init, LED_Exec, LED_Shutdown, LED_Test, 31, 0, "LED" },               // eTask_LED
  { true, CLASSB_Init, CLASSB_Exec, CLASSB_Shutdown, CLASSB_Test, 37, 0, "CLSB" },  // eTask_ClassB
  { true, APP_Init, APP_Exec, APP_Shutdown, APP_Test, 5, 0, "APP" },                // eTask_App
};

static_assert(sizeof(_Defaults) / sizeof(tTaskConfig) == eTask_NUM, "tTaskConfig size mismatch");

static tTaskConfig _Config[eTask_NUM];    //!< Task configuration data. Task configuration initially based on _Defaults
static tTaskRuntime _Runtime[eTask_NUM];  //!< Task runtime data
static tLoopMetrics _LoopMetrics;         //!< Loop metrics data
static tTask _ActiveTask;                 //!< The active task in the executive loop
static uint32_t _ErrorField;              //!< Task error field
static uint16_t _ShutdownTimeout_ms = 0;  //!< Shutdown timer in ms. 0 means shutdown not initiated
static uint32_t _ShutdownTimestamp = 0;   //!< Shutdown timestamp used to track shutdown timeout
static bool _ShutdownInProcess = false;   //!< True when shutdown has been initiated
static float _IdleTimeTotal_s = 0.0f;     //!< Total time spent in scheduler idle hook

static const uint32_t _TaskBreadcrumbMagic = 0x5441534Bu;  // 'TASK'
static uint32_t _LastTaskBreadcrumbMagic __attribute__((section(".noinit")));
static uint32_t _LastTaskBreadcrumbValue __attribute__((section(".noinit")));

/* Private function prototypes -----------------------------------------------*/
static void _ProcessLoopMetrics(void);
static void _ProcessTaskMetrics(tTask Task, bool Begin);
static void _ProcessIdleMetrics(bool Begin);
static uint32_t _GetTimeUntilNextTask_ms(void);
static void _IdleHook(void);
static void _ProcessShutdown(void);
static bool _Test(void);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes all task in the _Task array
 @return    True if all tasks initialized properly

 This functions takes the form of the TASK init prototype, fnTaskExec (see task.c/h)

 *******************************************************************/
bool TASK_Init(void)
{
  tTask task;
  bool success = true;

  // System task must always be enabled
  assert(_Defaults[eTask_System].Enabled);

  _ActiveTask = (tTask)0;
  ClassB_SetVar(eClassBVar_TASK_ACTIVETASK_ENUM, (tDataValue){ .Enum = (uint8_t)_ActiveTask });

  _ErrorField = 0;
  _ShutdownTimeout_ms = 0;
  _ShutdownInProcess = false;
  _IdleTimeTotal_s = 0.0f;

  // Loop metrics
  _LoopMetrics.Loops = 0;
  _LoopMetrics.TimeTotal_s = 0.0f;
  _LoopMetrics.TimeMin_ms = 100000;  // Set to a large value so first measurement is lower
  _LoopMetrics.TimeMax_ms = 0;
  _LoopMetrics.TimeAvg_ms = 0;

  // Set defaults and initialize other runtime data and metrics
  for (task = (tTask)0; task < _NumTasks; task++)
  {
    // Set task configurations to the defaults
    _Config[task] = _Defaults[task];

    _Runtime[task].Initialized = false;
    _Runtime[task].Timestamp = 0;
    _Runtime[task].Shutdown = false;
    _Runtime[task].DebugPeriod_ms = 0;

    _Runtime[task].Metrics.Name = _Config[task].Name;
    _Runtime[task].Metrics.Delay_ms = _Config[task].Delay_ms;
    _Runtime[task].Metrics.Switches = 0;
    _Runtime[task].Metrics.TimeTotal_s = 0.0f;
    _Runtime[task].Metrics.TimeMin_ms = 100000;  // Set to a large value so first measurement is lower
    _Runtime[task].Metrics.TimeMax_ms = 0;
    _Runtime[task].Metrics.TimeAvg_ms = 0;
  }

  // Initialize all enabled tasks
  for (task = (tTask)0; task < _NumTasks; task++)
  {
    if (!_Config[task].Enabled)
    {
      // Don't initialize
    }
    else if (!_Config[task].Init())
    {
      // Initialization failed. Disable the task
      _Config[task].Enabled = false;

      // Set the error bit
      _ErrorField |= (1 << task);

      printf("*** Error: Failed to initialize task '%s'\r\n", _Config[task].Name);  // Print msg to boot uart
    }
  }

  // Tasks initialized. Now test
  _Test();

  success = (_ErrorField == 0);

  if (success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Not Initialized");
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Executes the executive function of all tasks in the _Task array in an infinite loop
 @return    This function should never return

 This functions takes the form of the TASK executive prototype, fnTaskExec (see task.c/h)

*******************************************************************/
bool TASK_Exec(void)
{
  static bool taskRan = false;

  while (true)
  {
    _ActiveTask = (tTask)ClassB_GetVar(eClassBVar_TASK_ACTIVETASK_ENUM).Enum;

    if (_ActiveTask >= _NumTasks)
    {
      // Reset to first task if out of bounds
      _ActiveTask = (tTask)0;
      ClassB_SetVar(eClassBVar_TASK_ACTIVETASK_ENUM, (tDataValue){ .Enum = (uint8_t)_ActiveTask });

      // Active task should never be out of bounds, so assert if it is. Reset to first task to recover.
      LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Active task out of bounds. Resetting to first task.");
      assert_always();
    }

    if (!_Config[_ActiveTask].Enabled)
    {
      // Skip disabled tasks
    }
    else if (_ShutdownInProcess)
    {
      // Shutdown has been initiated, so call shutdown functions of tasks
      for (tTask task = (tTask)0; task < _NumTasks; task++)
      {
        if (_Runtime[task].Shutdown)
        {
          // Already shutdown
        }
        else if (!_Config[task].Enabled)
        {
          _Runtime[task].Shutdown = true;
        }
        else
        {
          // Call each task's Shutdown function
          _Runtime[task].Shutdown = _Config[task].Shutdown();
        }
      }

      // Check if all tasks are shutdown
      if (TASK_AreAllShutdown())
      {
        // Time to meet your maker. Bye bye.
        HAL_NVIC_SystemReset();
      }
      else if (TIMER_GetElapsed_ms(_ShutdownTimestamp) >= (_ShutdownTimeout_ms * 3))
      {
        // Timeout and reset if a task shutdown is taking too long
        HAL_NVIC_SystemReset();
      }
    }
    else if (TIMER_GetElapsed_ms(_Runtime[_ActiveTask].Timestamp) >= _Config[_ActiveTask].Delay_ms)
    {
      // Execute the task executive if the delay tick value is greater than desired
      {
        // Persist the currently running task so reset diagnostics can report where execution was.
        _LastTaskBreadcrumbValue = (uint32_t)_ActiveTask;
        _LastTaskBreadcrumbMagic = _TaskBreadcrumbMagic;

        _Runtime[_ActiveTask].Timestamp = TIMER_GetTick();

        if (_MetricsEnabled)
        {
          _ProcessTaskMetrics(_ActiveTask, true);
        }

        // Execute the task's main exec function
        if (_Config[_ActiveTask].Exec())
        {
          taskRan = true;
        }
        else
        {
          // Exec function should never return false. If it does, log an error and continue.
          LOG_Write(eLogger_Sys,
                    eLogLevel_Error,
                    _Module,
                    true,
                    "Task '%s' exec function returned false.",
                    _Config[_ActiveTask].Name);
        }

        if (_MetricsEnabled)
        {
          _ProcessTaskMetrics(_ActiveTask, false);
        }
      }
    }

    // Prepare for next task
    if (_ActiveTask + 1 < _NumTasks)
    {
      _ActiveTask++;
      ClassB_SetVar(eClassBVar_TASK_ACTIVETASK_ENUM, (tDataValue){ .Enum = (uint8_t)_ActiveTask });
    }
    else
    {
      _ActiveTask = (tTask)0;
      ClassB_SetVar(eClassBVar_TASK_ACTIVETASK_ENUM, (tDataValue){ .Enum = (uint8_t)_ActiveTask });

      // Scheduler heartbeat proves the cooperative loop is still making forward progress
      ClassB_WdgHeartbeat(CLASSB_WDG_HB_TASK_LOOP);

      if (_IdleHookEnabled && !taskRan && !_ShutdownInProcess)
      {
        _IdleHook();
      }

      // Process loop metrics if enabled
      if (_MetricsEnabled)
      {
        _ProcessLoopMetrics();
      }

      taskRan = false;
    }

    _ProcessShutdown();
  }

  // Should never return
  return false;
}

/*******************************************************************/
/*!
 @brief     Main TASK POST testing routine
 @return    True if successful, otherwise false

 This functions takes the form of the TASK test prototype, fnTaskExec (see task.c/h)

 *******************************************************************/
bool TASK_Test(void)
{
  static tTask task = (tTask)0;

  // Perform POST for all enabled tasks
  for (task = (tTask)0; task < _NumTasks; task++)
  {
    if (_Config[task].Enabled)
    {
      // Some devices need some time to stabilize before we should test them
      while (SYSTEM_GetUpTime_MS() < _Config[task].TestDelay)
      {
      }

      if (_Config[task].Test())
      {
        // Passed testing
        _Runtime[task].Initialized = true;
      }
      else
      {
        // Disable the task
        _Config[task].Enabled = false;

        // Set the error bit
        _ErrorField |= (1 << task);
      }
    }
  }

  return (_ErrorField == 0);
}

/*******************************************************************/
/*!
 @brief     Returns the initialization status
 @param     Task: The task
 @return    True if initialized

 *******************************************************************/
bool TASK_IsInitialized(tTask Task)
{
  return (Task < _NumTasks) ? _Runtime[Task].Initialized : false;
}

/*******************************************************************/
/*!
 @brief     Returns the initialization status of all tasks
 @return    True if initialized

 *******************************************************************/
bool TASK_AreAllInitialized(void)
{
  bool init = true;

  for (int task = (tTask)0; task < _NumTasks; task++)
  {
    if (_Config[task].Enabled)
    {
      if (!_Runtime[task].Initialized)
      {
        init = false;
        break;
      }
    }
  }

  return init;
}

/*******************************************************************/
/*!
 @brief     Returns the shutdown status when power down
 @return    True if all task are shutdown

 *******************************************************************/
bool TASK_AreAllShutdown(void)
{
  bool shutdown = true;

  for (int task = (tTask)0; task < _NumTasks; task++)
  {
    if (!_Config[task].Enabled)
    {
      // Skip disabled tasks
    }
    else if (_Runtime[task].Shutdown)
    {
      // Yes, is shutdown
    }
    else
    {
      shutdown = false;
      break;
    }
  }

  return shutdown;
}

/*******************************************************************/
/*!
 @brief     Sets the callback function that will be called at the debug period
 @param     Task: The task
 @param     Delay: Delay in milliseconds

 *******************************************************************/
void TASK_SetDelay(tTask Task, uint32_t Delay)
{
  if (Task < _NumTasks)
  {
    _Config[Task].Delay_ms = Delay;
  }
}

/*******************************************************************/
/*!
 @brief     Sets the callback function that will be called at the debug period


 *******************************************************************/
uint32_t TASK_GetErrorField(void)
{
  return _ErrorField;
}

/*******************************************************************/
/*!
 @brief     Gets the status of a tasks
 @param     Task: The desired task
 @param     Loop: Pointer for overall loop metrics
 @param     Metrics: Pointer for task metrics
 @return    True if successful

 *******************************************************************/
bool TASK_GetStatus(tTask Task, tLoopMetrics* Loop, tTaskMetrics* Metrics)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Loop))
  {
    assert_always();
  }
  else if (!IsRAM((uintptr_t)Metrics))
  {
    assert_always();
  }
  else if (Task < _NumTasks)
  {
    Loop->Loops = _LoopMetrics.Loops;
    Loop->TimeTotal_s = _LoopMetrics.TimeTotal_s;
    Loop->TimeMin_ms = _LoopMetrics.TimeMin_ms;
    Loop->TimeMax_ms = _LoopMetrics.TimeMax_ms;
    Loop->TimeAvg_ms = _LoopMetrics.TimeAvg_ms;

    Metrics->Name = _Config[Task].Name;
    Metrics->Delay_ms = _Config[Task].Delay_ms;
    Metrics->Switches = _Runtime[Task].Metrics.Switches;
    Metrics->TimeTotal_s = _Runtime[Task].Metrics.TimeTotal_s;
    Metrics->TimeMin_ms = _Runtime[Task].Metrics.TimeMin_ms;
    Metrics->TimeMax_ms = _Runtime[Task].Metrics.TimeMax_ms;
    Metrics->TimeAvg_ms = _Runtime[Task].Metrics.TimeAvg_ms;

    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Gets overall scheduler load percentages
 @param     Load: Pointer for scheduler load data
 @return    True if successful

 *******************************************************************/
bool TASK_GetLoad(tTaskLoad* Load)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Load))
  {
    assert_always();
  }
  else if (_LoopMetrics.TimeTotal_s > 0.0f)
  {
    float busy = 0.0f;
    float overhead = 0.0f;

    for (tTask task = (tTask)0; task < _NumTasks; task++)
    {
      busy += _Runtime[task].Metrics.TimeTotal_s;
    }

    overhead = _LoopMetrics.TimeTotal_s - busy - _IdleTimeTotal_s;
    if (overhead < 0.0f)
    {
      overhead = 0.0f;
    }

    Load->Busy_pct = busy / _LoopMetrics.TimeTotal_s * 100.0f;
    Load->Idle_pct = _IdleTimeTotal_s / _LoopMetrics.TimeTotal_s * 100.0f;
    Load->Overhead_pct = overhead / _LoopMetrics.TimeTotal_s * 100.0f;
    success = true;
  }
  else
  {
    Load->Busy_pct = 0.0f;
    Load->Idle_pct = 0.0f;
    Load->Overhead_pct = 0.0f;
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Gets the name of a task
 @param     Task: The desired task
 @return    Pointer to name

 *******************************************************************/
const char* TASK_GetName(tTask Task)
{
  return (Task < _NumTasks) ? _Config[Task].Name : "error";
}

/*******************************************************************/
/*!
 @brief     Gets reset-persistent breadcrumb for last task that entered Exec
 @param     Task: Output pointer for task enum
 @param     Name: Output pointer for task name
 @return    True if breadcrumb is valid, false otherwise

 *******************************************************************/
bool TASK_GetLastRunningBeforeReset(tTask* Task, const char** Name)
{
  bool success = false;

  if (Task == NULLPTR || Name == NULLPTR)
  {
    // Invalid pointers
  }
  else if (_LastTaskBreadcrumbMagic != _TaskBreadcrumbMagic)
  {
    // No valid breadcrumb data
  }
  else if (_LastTaskBreadcrumbValue >= (uint32_t)_NumTasks)
  {
    // Corrupt breadcrumb data
  }
  else
  {
    *Task = (tTask)_LastTaskBreadcrumbValue;
    *Name = _Defaults[*Task].Name;
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Clears reset-persistent breadcrumb for last running task

 *******************************************************************/
void TASK_ClearLastRunningBeforeReset(void)
{
  _LastTaskBreadcrumbMagic = 0u;
  _LastTaskBreadcrumbValue = 0u;
}

/*******************************************************************/
/*!
 @brief      Prints the status of a tasks in the form of a JSON string
 @param      Task: The desired task, or -1 for overall loop and scheduler status
 @param      Summary: True to print summary of overall loop and scheduler status, false to print individual task status
 @param      Buf: Buffer to store the status string
 @param      BufSize: Size of the buffer
 *******************************************************************/
uint32_t TASK_PrintStatus(tTask Task, bool Summary, char* Buf, uint32_t BufSize)
{
  // If a caller is not the tech task, buffer size may need to be adjusted
  char output[TECH_RESPONSE_MAXSIZE];
  uint32_t size = 0;

  if (BufSize > TECH_RESPONSE_MAXSIZE)
  {
    assert_always();
  }
  else if (!IsRAM(((uintptr_t)Buf) + BufSize - 1))
  {
    assert_always();
  }
  else if (Summary)
  {
    tTaskLoad load = { 0 };
    TASK_GetLoad(&load);

    size = snprintf(output,
                    BufSize,
                    "summary:{\"loops\":%lu%lu, \"tot\":%.01fs, \"min\":%lu, \"max\":%lu,"
                    "\"avg\":%lu},\"load\":{\"busy\":%.1f, \"idle\":%.1f, \"oh\":%.1f}",
                    (uint32_t)(_LoopMetrics.Loops >> 32),
                    (uint32_t)(_LoopMetrics.Loops & 0xFFFFFFFF),
                    _LoopMetrics.TimeTotal_s,
                    _LoopMetrics.TimeMin_ms,
                    _LoopMetrics.TimeMax_ms,
                    _LoopMetrics.TimeAvg_ms,
                    load.Busy_pct,
                    load.Idle_pct,
                    load.Overhead_pct);
  }
  else if (Task < _NumTasks)
  {
    size = snprintf(
      output,
      BufSize,
      "task:{\"name\":\"%s\", \"delay\":%lu, \"sw\":%lu%lu, \"tot\":%.01fs, \"min\":%lu, \"max\":%lu,\"avg\":%lu}",
      _Runtime[Task].Metrics.Name,
      _Runtime[Task].Metrics.Delay_ms,
      (uint32_t)(_Runtime[Task].Metrics.Switches >> 32),
      (uint32_t)(_Runtime[Task].Metrics.Switches & 0xFFFFFFFF),
      _Runtime[Task].Metrics.TimeTotal_s,
      _Runtime[Task].Metrics.TimeMin_ms,
      _Runtime[Task].Metrics.TimeMax_ms,
      _Runtime[Task].Metrics.TimeAvg_ms);
  }

  if (size > 0)
  {
    strncpy(Buf, output, BufSize - 1);
  }

  return size;
}

/*******************************************************************/
/*!
 @brief     Resets all tasks to their default configurations

 *******************************************************************/
void TASK_ResetDefaults(void)
{
  for (int task = (tTask)0; task < _NumTasks; task++)
  {
    _Config[task] = _Defaults[task];
  }

  LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Resetting to defaults");
}

/*******************************************************************/
/*!
 @brief     Sets the shutdown timer
 @param     Time_ms  Time in milliseconds. 0 means shutdown immediately
 *******************************************************************/
void TASK__BeginShutdown(uint16_t Time_ms)
{
  // Set to 1ms if 0 to ensure shutdown processing occurs in the exec loop
  _ShutdownTimeout_ms = Time_ms > 0 ? Time_ms : 1ul;
  _ShutdownTimestamp = TIMER_GetTick();

  LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Shutdown initiated. Time to shutdown: %d ms", Time_ms);
}

/* Private Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Process metrics for the system

 *******************************************************************/
static void _ProcessLoopMetrics(void)
{
  static uint32_t timestamp = 0;
  static bool initialized = false;
  uint32_t time;

  // Prime the baseline on first call so startup time does not skew loop metrics.
  if (!initialized)
  {
    timestamp = TIMER_GetTick();
    initialized = true;
  }
  else
  {
    time = TIMER_GetElapsed_ms(timestamp);

    _LoopMetrics.Loops++;
    _LoopMetrics.TimeTotal_s += (time / 1000.0f);

    if (time < _LoopMetrics.TimeMin_ms)
    {
      _LoopMetrics.TimeMin_ms = time;
    }
    else if (time > _LoopMetrics.TimeMax_ms)
    {
      _LoopMetrics.TimeMax_ms = time;
    }

    _LoopMetrics.TimeAvg_ms = _LoopMetrics.TimeTotal_s / _LoopMetrics.Loops * 1000.0f;

    timestamp = TIMER_GetTick();
  }
}

/*******************************************************************/
/*!
 @brief     Process metrics for a active task
 @param     Task: The relevant task
 @param     Begin: True if beginning a task metric

 *******************************************************************/
static void _ProcessTaskMetrics(tTask Task, bool Begin)
{
  static uint32_t timestamp[eTask_NUM] = { 0 };
  uint32_t time;

  if (Begin)
  {
    // Track number of "context switches"
    _Runtime[Task].Metrics.Switches++;

    // Get ticks to compute exec time
    timestamp[Task] = TIMER_GetTick();
  }
  else
  {
    time = TIMER_GetElapsed_ms(timestamp[Task]);

    // Update task metrics
    _Runtime[Task].Metrics.TimeTotal_s += (time / 1000.0f);

    if (time < _Runtime[Task].Metrics.TimeMin_ms)
    {
      _Runtime[Task].Metrics.TimeMin_ms = time;
    }
    else if (time > _Runtime[Task].Metrics.TimeMax_ms)
    {
      _Runtime[Task].Metrics.TimeMax_ms = time;
    }

    _Runtime[Task].Metrics.TimeAvg_ms = _Runtime[Task].Metrics.TimeTotal_s / _Runtime[Task].Metrics.Switches * 1000.0f;
  }
}

/*******************************************************************/
/*!
 @brief     Process metrics for the scheduler idle hook
 @param     Begin: True if beginning idle metric

 *******************************************************************/
static void _ProcessIdleMetrics(bool Begin)
{
  static uint32_t timestamp = 0;
  uint32_t time;

  if (Begin)
  {
    timestamp = TIMER_GetTick();
  }
  else
  {
    time = TIMER_GetElapsed_ms(timestamp);
    _IdleTimeTotal_s += (time / 1000.0f);
  }
}

/*******************************************************************/
/*!
 @brief     Returns the time until the next enabled task is due to run
 @return    Remaining time in ms. 0 indicates a task is due now

 *******************************************************************/
static uint32_t _GetTimeUntilNextTask_ms(void)
{
  uint32_t now = TIMER_GetTick();
  uint32_t nextDue_ms = UINT32_MAX;

  for (tTask task = (tTask)0; task < _NumTasks; task++)
  {
    uint32_t delay = _Config[task].Delay_ms;
    uint32_t elapsed = now - _Runtime[task].Timestamp;

    if (!_Config[task].Enabled)
    {
    }
    else if ((delay == 0u) || (elapsed >= delay))
    {
      nextDue_ms = 0u;
      break;
    }
    else if ((delay - elapsed) < nextDue_ms)
    {
      nextDue_ms = (delay - elapsed);
    }
  }

  return (nextDue_ms == UINT32_MAX) ? 0u : nextDue_ms;
}

/*******************************************************************/
/*!
 @brief     Enter sleep while no task is ready to run

 Sleep handling is centralized in POWER_Sleep() that manages watchdog
 refresh and HAL tick suspend/resume around the WFI entry
 *******************************************************************/
static void _IdleHook(void)
{
  if (_GetTimeUntilNextTask_ms() > 0u)
  {
    _ProcessIdleMetrics(true);
    POWER_Sleep();
    _ProcessIdleMetrics(false);
  }
}

/*******************************************************************/
/*!
 @brief     Process shutdown by calling shutdown functions
 *******************************************************************/
static void _ProcessShutdown(void)
{
  if (_ShutdownTimeout_ms > 0 && !_ShutdownInProcess)
  {
    if (TIMER_GetElapsed_ms(_ShutdownTimestamp) >= _ShutdownTimeout_ms)
    {
      _ShutdownInProcess = true;
    }
  }
}

/*******************************************************************/
/*!
 @brief     Main TASK POST testing routine
 @return    True if successful, otherwise false

 This functions takes the form of the TASK test prototype, fnTaskExec (see task.c/h)

 *******************************************************************/
static bool _Test(void)
{
  static tTask task = (tTask)0;

  // Perform POST for all enabled tasks
  for (task = (tTask)0; task < _NumTasks; task++)
  {
    if (_Config[task].Enabled)
    {
      // Some devices need some time to stabilize before we should test them
      while (SYSTEM_GetUpTime_MS() < _Config[task].TestDelay)
      {
      }

      if (_Config[task].Test())
      {
        // Passed testing
        _Runtime[task].Initialized = true;
      }
      else
      {
        // Disable the task
        _Config[task].Enabled = false;

        // Set the error bit
        _ErrorField |= (1 << task);
      }
    }
  }

  return (_ErrorField == 0);
}
