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

 @file    timer.c
 @brief   Implementation for supporting application timers
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#include "main.h"
#include "timer.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define MAX_TIMERS 3

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "TIMER";            //!< Module name for debug logging
static const uint8_t _MaxCBTimers = MAX_TIMERS;  //!< Maximum number of timer callback functions

static pfcnTimerCallback pfcnTimerCallbackArray[MAX_TIMERS];  //!< Timer callback functions
static pfcnTimerGetTick _GetTick = HAL_GetTick;  //!< Timer get-tick function. If NULLPTR, defaults to TIMER_GetTick()

/* Private function prototypes -----------------------------------------------*/
/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initialize timer
 @return    Timer period in ms

 Assumes System Timer has been configured to 1ms in call to HAL_INIT()
 *******************************************************************/
uint16_t TIMER_Init(void)
{
  static bool init = false;  // timer initialized if true

  // initialize timer if not already initialized
  if (!init)
  {
    // initialize callback array
    for (int i = 0; i < _MaxCBTimers; i++)
    {
      pfcnTimerCallbackArray[i] = NULLPTR;
    }

    init = true;
  }

  if (init)
  {
    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Not Initialized");
  }

  return TIMER_PERIOD_MS;
}

/*******************************************************************/
/*!
 @brief     Get the timer period in ms
 @return    Timer period in ms
 *******************************************************************/
uint16_t TIMER_GetPeriod(void)
{
  return TIMER_PERIOD_MS;
}

/*******************************************************************/
/*!
 @brief     Returns after delaying the specified number of milliseconds
 @param     Delay_ms: milliseconds to delay
 *******************************************************************/
void TIMER_DelayMs(uint32_t Delay_ms)
{
  HAL_Delay(Delay_ms);
}

/*******************************************************************/
/*!
 @brief     Returns the system time (SystemTicks count)
 @return    Current SystemTicks time
 *******************************************************************/
uint32_t TIMER_GetTick(void)
{
  if (_GetTick != NULLPTR)
  {
    return _GetTick();
  }
  else
  {
    assert_param(false);
    return HAL_GetTick();
  }
}

/*******************************************************************/
/*!
 @brief     Returns the elapsed time
 @param     RefTime_ms: The starting time reference in ms ticks
 @return    Elapsed time between the starting and current time in ms
 *******************************************************************/
uint32_t TIMER_GetElapsed_ms(uint32_t RefTime_ms)
{
  return (TIMER_GetTick() - RefTime_ms);
}

/*******************************************************************/
/*!
 @brief     Returns the elapsed time
 @param     RefTime_ms: The starting time reference in ms ticks
 @return    Elapsed time between the starting and current time in secs
 *******************************************************************/
float TIMER_GetElapsed_s(uint32_t RefTime_ms)
{
  return (float)(TIMER_GetTick() - RefTime_ms) / 1000.0f;
}

/*******************************************************************/
/*!
 @brief     This is an interrupt service routine executed to keep track of the system timers
*******************************************************************/
void TIMER_ProcessCallbacks(void)
{
  for (int i = 0; i < _MaxCBTimers; i++)
  {
    if (pfcnTimerCallbackArray[i] != NULLPTR)
    {
      // Timer callbacks
      (*pfcnTimerCallbackArray[i])();
    }
  }
}

/*******************************************************************/
/*!
 @brief     Registers a callback function to retrieve the current tick count.
            If not registered, defaults to HAL_GetTick().

 @param     PfcnFunction: Address of function
 @return    True if successful

 @note      Register an alternate get-tick function for use by timer module
            and other modules that need to know the system tick time.
            This allows the tick time to include sleep time,
            which is not included in the default HAL_GetTick() value
            because the HAL tick is suspended during sleep.
 *******************************************************************/
bool TIMER_RegisterTickGetter(pfcnTimerGetTick PfcnFunction)
{
  if (PfcnFunction != NULLPTR)
  {
    _GetTick = PfcnFunction;
  }
  else
  {
    assert_param(false);
    _GetTick = HAL_GetTick;
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Register a timer callback function
 @param     PfcnFunction: Address of function to register
 @return    True if successful

 Initializes the timer if it isn't. Should not be called from an interrupt
 *******************************************************************/
bool TIMER_RegisterCallback(pfcnTimerCallback PfcnFunction)
{
  bool success = false;

  // TIMER_Init();

  for (int i = 0; i < _MaxCBTimers; i++)
  {
    if (pfcnTimerCallbackArray[i] == NULLPTR)
    {
      pfcnTimerCallbackArray[i] = PfcnFunction;
      success = true;
      break;
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Un-registers a timer callback function
 @param     PfcnFunction: Address of function to register

 Should not be called from an interrupt
 *******************************************************************/
void TIMER_UnregisterCallback(pfcnTimerCallback PfcnFunction)
{
  for (int i = 0; i < _MaxCBTimers; i++)
  {
    if (pfcnTimerCallbackArray[i] == PfcnFunction)
    {
      pfcnTimerCallbackArray[i] = NULLPTR;
      break;
    }
  }
}
