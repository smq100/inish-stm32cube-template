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

 @file    power.c
 @brief   Implementation for supporting power management and sleep
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#include "main.h"
#include "power.h"
#include "timer.h"
#include "log.h"
#include "task_daq.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

volatile bool tim_sleep;          ///< flag set/cleared in timer11 IRQ while sleeping
uint16_t volatile phase_irq = 0;  ///< BITS set by interrupt-handlers, recording which IRQ caused sleep to wake

/* Private variables ---------------------------------------------------------*/

static uint32_t _SleepTime_ms = 0;    // Ignore rollover at ~49 days of sleeptime
static uint32_t _SleepFracTicks = 0;  // LPTIM ticks below 1ms carried between sleeps
static const char* _Module = "POWER";

/* Private function prototypes -----------------------------------------------*/

static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief  Initialize the POWER module
 @param None
 @retval True if successful
*******************************************************************/
bool POWER_Init(void)
{
  // Register DAQ items
  tDAQ_Config daq;
  daq.CallbackRead = _DAQReadCallback;
  daq.CallbackWrite = NULLPTR;  // Read-only
  DAQ_RegisterItem(eDAQ_Sleep, &daq, false);

  __HAL_RCC_PWR_CLK_ENABLE();

  LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");

  return true;
}

/*******************************************************************/
/*!
 @brief  Put the system into sleep mode, until a wake-up event occurs
 @param None
 @retval True if successful
*******************************************************************/
void POWER_Sleep(void)
{
  // Use TIM10 counter which keeps running during sleep

  // Get timestamp before sleep. Have to use hTIM_SLEEP_IWDG_CNT because systicks are suspended
  uint32_t sleep_tic = __HAL_TIM_GET_COUNTER(&hTIM_SLEEP_IWDG_CNT);

  // Go to sleep; wait for a cross-over (other?) wake-up
  // Suspend Tick increment to prevent wakeup by Systick interrupt. Otherwise
  // the Systick interrupt will wake up the device within 1ms (HAL time base)

  HAL_SuspendTick();
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();

  // Convert sleep to milliseconds (hTIM_SLEEP_IWDG_CNT freq is 4.0 MHz)
  uint32_t sleep_toc = __HAL_TIM_GET_COUNTER(&hTIM_SLEEP_IWDG_CNT);
  if (sleep_toc < sleep_tic)
  {
    // Handle rollover of 16-bit counter (rollover every 16.384ms at 4.0 MHz)
    sleep_toc += 0x10000;
  }
  uint32_t sleep = sleep_toc - sleep_tic;

  // Convert to milliseconds while preserving sub-ms contributions across calls.
  // This is important for POWER_Delay_us(), where each sleep is often <1ms.
  const uint32_t ticks_per_ms = (TIM_SLEEP_IWDG_COUNT_FREQ / 1000u);
  uint32_t total_ticks = sleep + _SleepFracTicks;

  // Integer ms portion of sleep is added to total sleep time
  _SleepTime_ms += (total_ticks / ticks_per_ms);

  // The fractional ticks will be added to the next sleep duration to preserve accuracy over time.
  _SleepFracTicks = (total_ticks % ticks_per_ms);
}

/*******************************************************************/
/*!
 @brief  Cause a quick, self-limited sleep using internal timer (TIM10)
 @param  Usec   micro-seconds to sleep for (capped to ~32.7ms to stay below 16-bit timer rollover)
 @param  Sleep  true to actually sleep, false to just wait
 @retval None
*******************************************************************/
void POWER_Delay_us(uint16_t Usec, bool Sleep)
{
  if (Usec == 0u)
  {
    Usec = 1u;
  }
  else if (Usec > POWER_SLEEP_US_MAX)
  {
    Usec = POWER_SLEEP_US_MAX;
    LOG_Write(
      eLogger_Sys, eLogLevel_Warning, _Module, true, "Requested sleep time too long; capping to max of ~32.7 ms");
  }

  tim_sleep = true;

  // Configure TIM11 as a one-shot wake timer for this call.
  // Clear stale status first to avoid an immediate wake on pending update flags.
  HAL_TIM_Base_Stop_IT(&hTIM_SLEEP);
  __HAL_TIM_SET_COUNTER(&hTIM_SLEEP, 0u);
  __HAL_TIM_SET_AUTORELOAD(&hTIM_SLEEP, (uint32_t)Usec - 1u);
  __HAL_TIM_CLEAR_IT(&hTIM_SLEEP, TIM_IT_UPDATE);
  __HAL_TIM_CLEAR_FLAG(&hTIM_SLEEP, TIM_FLAG_UPDATE);

  // Start timer and update interrupt. IRQ clears tim_sleep when delay expires.
  if (HAL_TIM_Base_Start_IT(&hTIM_SLEEP) != HAL_OK)
  {
    tim_sleep = false;
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Failed to start short sleep timer");
    return;
  }

  if (Sleep)
  {
    // Sweet dreams...
    POWER_Sleep();
  }

  // Note: Sleep stops for ANY interrupt, so wait for the timer IRQ
  // with a timeout to prevent lockup if something goes wrong with the timer or its interrupt.
  // The timer IRQ handler will clear the tim_sleep flag when it fires
  uint32_t timeout = TIMER_GetTick();
  while (tim_sleep)
  {
    if ((TIMER_GetElapsed_ms(timeout)) > 50u)  // Larger than ~32.7ms max timer delay
    {
      tim_sleep = false;

      LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Timeout waiting for short sleep timer");
    }
  }

  // Ensure timer is fully stopped and IRQ source cleared after each short sleep,
  // so normal POWER_Sleep() calls are not disturbed by periodic TIM11 wakeups.
  HAL_TIM_Base_Stop_IT(&hTIM_SLEEP);
  __HAL_TIM_CLEAR_IT(&hTIM_SLEEP, TIM_IT_UPDATE);
  __HAL_TIM_CLEAR_FLAG(&hTIM_SLEEP, TIM_FLAG_UPDATE);
}

/*******************************************************************/
/*!
 @brief  Delay for a specified number of 100us units, using repeated short sleeps to allow wake-up on other events
 @param  Value  Number of 100us units to delay
 @param  Sleep  true to actually sleep, false to just wait
 @retval None
*******************************************************************/
void POWER_Delay_100us(uint16_t Value, bool Sleep)
{
  uint32_t remaining_us = ((uint32_t)Value * 100u);

  while (remaining_us > 0u)
  {
    uint16_t chunk_us = (remaining_us > POWER_SLEEP_US_MAX) ? POWER_SLEEP_US_MAX : (uint16_t)remaining_us;
    POWER_Delay_us(chunk_us, Sleep);
    remaining_us -= chunk_us;
  }
}

/*******************************************************************/
/*!
 @brief  Get the time spent sleeping since boot, in milliseconds
 @param  None
 @retval Time spent sleeping in milliseconds
*******************************************************************/
uint32_t POWER_GetSleepTime_ms(void)
{
  return _SleepTime_ms;
}

/*******************************************************************/
/*!
 @brief  DAQ read callback for POWER module
 @param  Entry   DAQ entry being read
 @param  Item    DAQ item being read (not used in this module)
 @retval True if successful
*******************************************************************/
static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item)
{
  tDataValue value;
  bool success = true;

  if (Entry == eDAQ_Sleep)
  {
    value.U32 = _SleepTime_ms;
    DAQ_UpdateItem(Entry, Item, value);
  }

  return success;
}
