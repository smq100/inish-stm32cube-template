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

 @file    task_system.c
 @brief   Implementation and data for overall system functions
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#define SYSTEM_PROTECTED
#define TASK_PROTECTED

#include "main.h"
#include "task_system.h"
#include "task_classb.h"
#include "task_led.h"
#include "task_daq.h"
#include "classb.h"
#include "power.h"
#include "log.h"
#include "timer.h"
#include "version.h"
#include "rtc.h"
#include "eeprom_mcu.h"
#include "eeprom_ext.h"
#include "util.h"

/* Private typedef -----------------------------------------------------------*/

//! System states
typedef enum
{
  eSystemState_Init1,      // Initial basic initialization
  eSystemState_Init2,      // After all tasks initialized
  eSystemState_Init3,      // Final initialization
  eSystemState_Completed,  // Initialization complete
  eSystemState_Running,    // Normal running state
  eSystemState_Error,      // Error state
} tSystemState;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/

extern uint32_t _ram_start;  ///< Start of RAM. Defined in the linker script
extern uint32_t _ram_end;    ///< One-past-end of RAM. Defined in the linker script
extern uint32_t _rom_start;  ///< Start of ROM. Defined in the linker script
extern uint32_t _rom_end;    ///< One-past-end of ROM. Defined in the linker script

/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_SYS";                                  ///< Module name to be used for debug logging
static const uint32_t _ProcessingPeriod_ms[3] = { 100, 1000, 10000 };  ///< System processing periods in milliseconds
static const char* _StatusLEDStateNames[eStatusLED_NUM] = { "Init", "Normal", "Error" };

static uint32_t _BootCycles = 0;                           ///< Number of boot cycles since EEPROM was erased
static tStatusLEDState _StatusLEDState = eStatusLED_Init;  ///< Current status LED state
static uint8_t _LEDErrorPulseCount = 0;                    // Number of pulses to indicate error code
static uint32_t _RAM_kb;
static uint32_t _ROM_kb;

static tSystemState _State = eSystemState_Init1;  //!< System state
static float _CoreClkFreq = 0.0;                  //!< Core clock freq in Hz
static uint32_t _HardwareRev = 0;                 //!< Detected HW revision

static_assert(sizeof(_StatusLEDStateNames) / sizeof(_StatusLEDStateNames[0]) == eStatusLED_NUM, "LED state names");

/* Private function prototypes -----------------------------------------------*/

static bool _Init1(void);
static bool _Init2(void);
static bool _Init3(void);
static bool _ProcessStatusLED(void);
static void _ResetRTC(void);
static uint32_t _GetHardwareRev(void);
static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes all system data and peripherals
 @return    True if successful, otherwise false

 @note      Function takes the form of the TASK prototypes (see task.c/h)
*******************************************************************/
bool SYSTEM_Init(void)
{
  UNUSED(_Module);

  _State = _Init1() ? eSystemState_Init2 : eSystemState_Error;

  return _State == eSystemState_Init2;
}

/*******************************************************************/
/*!
 @brief     Initializes all system data and peripherals
 @return    True if successful, otherwise false

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SYSTEM_Exec(void)
{
  static uint32_t time[3] = { 0 };
  bool success = true;

  if (_State == eSystemState_Init1)
  {
    _State = eSystemState_Init2;
  }
  else if (_State == eSystemState_Init2)
  {
    if (TASK_AreAllInitialized())
    {
      // Perform secondary initialization after all tasks are initialized
      _State = _Init2() ? eSystemState_Init3 : eSystemState_Error;
    }

    if (_State == eSystemState_Error)
    {
      printf("Initialization failed during Init2\r\n");
    }
  }
  else if (_State == eSystemState_Init3)
  {
    _State = _Init3() ? eSystemState_Completed : eSystemState_Error;

    if (_State == eSystemState_Error)
    {
      printf("Initialization failed during Init3\r\n");
    }
  }
  else if (_State == eSystemState_Completed)
  {
    _State = eSystemState_Running;
  }
  else if (_State == eSystemState_Running)
  {
    // High frequency processing
    if (TIMER_GetElapsed_ms(time[0]) > _ProcessingPeriod_ms[0])
    {
      if (!CLASSB_DidAllRuntimePass())
      {
        // TODO Runtime ClassB test(s) failed
      }

      // Process the status LED for any changes based on system state or faults
      _ProcessStatusLED();

      time[0] = TIMER_GetTick();
    }

    // Med frequency processing
    if (TIMER_GetElapsed_ms(time[1]) > _ProcessingPeriod_ms[1])
    {
      time[1] = TIMER_GetTick();
    }

    // Low frequency processing
    if (TIMER_GetElapsed_ms(time[2]) > _ProcessingPeriod_ms[2])
    {
      time[2] = TIMER_GetTick();
    }
  }
  else if (_State == eSystemState_Error)
  {
    // Should never get here since the system should be rebooted on any error,
    // but if we do, stay here and flash the LEDs to indicate an error state
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);
    success = false;
  }
  else
  {
    success = false;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SYSTEM_Shutdown(void)
{
  // No shutdown processing required
  return true;
}

/*******************************************************************/
/*!
 @brief     POST routine for SYSTEM module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SYSTEM_Test(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Set the status LED state
 @param     State   Desired status LED state
 @param     ErrorCode  Error code to indicate if State is eStatusLED_Error
 @return    None
 *******************************************************************/
void SYSTEM_SetStatusLED(tStatusLEDState State, uint8_t ErrorCode)
{
  _StatusLEDState = State;

  if (State == eStatusLED_Error)
  {
    _LEDErrorPulseCount = ErrorCode;
  }
}

/*******************************************************************/
/*!
 @brief     Factory reset processing. Resets all persistent settings to factory defaults.
 @return    None
 *******************************************************************/
void SYSTEM_FactoryReset(void)
{
  LOG_WriteDirect(eLogger_Sys, eLogLevel_Warning, _Module, true, "Factory reset initiated");

  // Reset the system logs (which also resets the log index in EXT EEPROM)
  LOG_Reset(eLogger_Sys, true);
  LOG_Reset(eLogger_Tech, false);

  // Reset the regs only (don't reset the entire EEPROM)
  EEPROM_MCU_Erase(true);

  // Reset the external EEPROM (where logs are stored) to all 0xFF to indicate empty
  EEPROM_EXT_Erase();

  SYSTEM__BeginShutdown(1500);
}

/*******************************************************************/
/*!
 @brief     Begins the shutdown process for the system.
 @param     Time_ms  Time in milliseconds before shutdown
 @return    None
 *******************************************************************/
void SYSTEM__BeginShutdown(uint16_t Time_ms)
{
  // Shutdown all running tasks and then reboot the system (protected function)
  TASK__BeginShutdown(Time_ms);
}

/*******************************************************************/
/*!
 @brief     Returns the number of boot cycles since EEPROM was erased (factory reset)
 @return    Number of boot cycles
 *******************************************************************/
uint32_t SYSTEM_GetBootCount(void)
{
  return _BootCycles;
}

/*******************************************************************/
/*!
 @brief     Returns system up-time
 @return    System up-time in milliseconds
 *******************************************************************/
uint32_t SYSTEM_GetUpTime_MS(void)
{
  return TIMER_GetTick();
}

/*******************************************************************/
/*!
 @brief     Returns system up-time
 @return    System up-time in seconds
 *******************************************************************/
uint32_t SYSTEM_GetUpTime_S(void)
{
  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;

  // Date must be read after time to provide accurate values (according to HAL docs)
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

  uint32_t up_time_s = (uint32_t)time.Seconds + (uint32_t)time.Minutes * 60u + (uint32_t)time.Hours * 3600u;

  return up_time_s;
}

/*******************************************************************/
/*!
 @brief     Returns system tick in milliseconds.
            Note that this is not necessarily the HAL tick time since boot because it is not updated during sleep
 @return    System tick in milliseconds
 *******************************************************************/
uint32_t SYSTEM_GetTick(void)
{
  return HAL_GetTick() + POWER_GetSleepTime_ms();
}

/*******************************************************************/
/*!
 @brief     Set a POST error
 *******************************************************************/
bool SYSTEM_IsRunning(void)
{
  return _State == eSystemState_Running;
}

/*******************************************************************/
/*!
 @brief     Returns the HW board version ID
 @return    HW version ID

 *******************************************************************/
uint32_t SYSTEM_GetHWVersion(void)
{
  return _HardwareRev;
}

/*******************************************************************/
/*!
 @brief     Returns base processor frequency
 @return    Frequency in Hz
 *******************************************************************/
float SYSTEM_GetSysClockFrequency(void)
{
  return _CoreClkFreq;
}

/* Private Implementation --------------------------------------------------- */

/*******************************************************************/
/*!
 @brief     Initializes system module
 @return    True if successful
 *******************************************************************/
static bool _Init1(void)
{
  bool success = false;

  _CoreClkFreq = (float)HAL_RCC_GetSysClockFreq();
  _HardwareRev = _GetHardwareRev();
  _StatusLEDState = eStatusLED_Init;
  _RAM_kb = (uint32_t)(&_ram_end - &_ram_start) * 4u / 1024u;
  _ROM_kb = (uint32_t)(&_rom_end - &_rom_start) * 4u / 1024u;

  if (TIMER_Init() > 0)
  {
    _ResetRTC();

    // Register alternate get-tick function for use by timer module
    // and other modules that need to know the system tick time.
    // This allows the tick time to include sleep time,
    // which is not included in the default HAL_GetTick() value
    // because the HAL tick is suspended during sleep.
    TIMER_RegisterTickGetter(SYSTEM_GetTick);

    // Persistent boot-cycle counter stored in data EEPROM reg0
    if (EEPROM_MCU_IncrementReg(eEEPROM_Reg_BootCount))
    {
      success = EEPROM_MCU_ReadReg(eEEPROM_Reg_BootCount, &_BootCycles);
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Initializes system module after all tasks are initialized
 @return    True if successful
 *******************************************************************/
static bool _Init2(void)
{
  bool ret = false;

  if (POWER_Init())
  {
    // Register DAQ items
    tDAQ_Config daq;
    daq.CallbackRead = _DAQReadCallback;
    daq.CallbackWrite = NULLPTR;  // Read-only
    DAQ_RegisterItem(eDAQ_Uptime, &daq, false);

    ret = true;
  }

  return ret;
}

/*******************************************************************/
/*!
 @brief     Initializes system module after all tests are completed
 @return    True if successful
 *******************************************************************/
static bool _Init3(void)
{
  // Start the normal heartbeat LED, aka the Happy Light :-)
  SYSTEM_SetStatusLED(eStatusLED_Normal, 0);

  LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "CPU ID: %X, Clock: %.2f MHz", SCB->CPUID, _CoreClkFreq / 1e6);
  LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "CPU RAM: %uk, Flash: %uk", _RAM_kb, _ROM_kb);
  LOG_Write(eLogger_Sys, eLogLevel_High, _Module, true, "HW: %lu, FW: %s (%s)", _HardwareRev, FW_REV_STR, FW_DATE_STR);

  ClassB_PrintErrorCounts();

  LOG_Write(eLogger_Sys,
            eLogLevel_High,
            _Module,
            true,
            "System boot and initialization complete. Boot cycle: %lu",
            _BootCycles);

  return true;
}

/*******************************************************************/
/*!
 @brief     Processes the status LED
 @return    True if the status LED state changed
 *******************************************************************/
static bool _ProcessStatusLED(void)
{
  bool changed = false;
  static tStatusLEDState state = eStatusLED_Init;

  if (state == _StatusLEDState)
  {
    // Nothing new to change
  }
  else if (_StatusLEDState == eStatusLED_Normal)
  {
    LED_Flash(eLED_Status, 0.50f, -1, "");
  }
  else if (_StatusLEDState == eStatusLED_Error)
  {
    LED_Pulse(eLED_Status, 1.0f, -1, (int32_t)(_LEDErrorPulseCount), "");
  }

  if (state != _StatusLEDState)
  {
    state = _StatusLEDState;
    changed = true;

    LOG_Write(eLogger_Sys, eLogLevel_Med, _Module, false, "Status LED changed to '%s'", _StatusLEDStateNames[state]);
  }

  return changed;
}

/*******************************************************************/
/*!
 @brief     Resets the RTC time and date to 0 (1/1/2000 00:00:00)
 @return    None
 *******************************************************************/
static void _ResetRTC(void)
{
  RTC_TimeTypeDef t = { 0 };
  RTC_DateTypeDef d = { 0 };

  t.Hours = 0;
  t.Minutes = 0;
  t.Seconds = 0;
  d.WeekDay = RTC_WEEKDAY_MONDAY;
  d.Month = RTC_MONTH_JANUARY;
  d.Date = 1;
  d.Year = 0;

  HAL_RTC_SetTime(&hrtc, &t, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BIN);
}

/*******************************************************************/
/*!
 @brief     Retrieves the HW revision value
 @return    Numeric HW version
 *******************************************************************/
static uint32_t _GetHardwareRev(void)
{
  // TODO
  uint32_t rev = 0;

  return rev;
}

/*******************************************************************/
/*!
 @brief     Callback for DAQ reads of system variables
 @return    True if successful
 *******************************************************************/
static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item)
{
  tDataValue value;
  bool success = true;

  if (Entry == eDAQ_Uptime)
  {
    value.U32 = SYSTEM_GetTick();
    DAQ_UpdateItem(Entry, Item, value);
  }

  return success;
}
