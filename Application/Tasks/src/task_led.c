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
* @file    task_led.c
* @brief   Implementation of the LED processing
* @author  Steve Quinlan
* @date    2026-March
*
******************************************************************************/

#include "main.h"

#include "task_led.h"
#include "timer.h"
#include "log.h"

/* Private define ------------------------------------------------------------*/

#define MAX_KEYLENGTH 10  //!< The max keylength of an LED locking string

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  bool Active;           //!< True when flashing
  bool Lit;              //!< True when LED is lit while flashing
  float Interval_S;      //!< Flash interval in secs
  int32_t Qty;           //!< Number of flashes
  int32_t Pulses;        //!< Flashes per interval
  int32_t PulsesRemain;  //!< Flashes per interval
  uint32_t FlashTime;    //!< Time stamp for flashes
  uint32_t PulseTime;    //!< Time stamp for pulses
} tLEDFlash;

//! Runtime data structure
typedef struct
{
  bool Lit;                 //!< True when LED is lit
  tLEDFlash Flash;          //!< Active parameters
  char Key[MAX_KEYLENGTH];  //!< Allows for and LED to be locked to a module using a string as a key
} tLEDRuntime;

typedef struct
{
  bool OnPolarityH;  //!< Level for LED on
} tLEDConfig;

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_LED";                //!< Module name for debug logging
static const float _TestStep_S = 0.50f;              //!< Time between LED tests in seconds
static const float _PulseDuration_S = 0.2f;          //!< Duration of LED pulse in seconds
static const uint8_t _MaxKeyLength = MAX_KEYLENGTH;  //!< The max keylength of an LED locking string
static const bool _DisableLocking = true;            //!< True when locking LED is disabled

static const tLEDConfig _Config[] = {
  { .OnPolarityH = true },
};

//! LED GPIO configuration
static const tGPIOConfig _GPIO[] = {
  { LED_STATUS_GPIO_Port, LED_STATUS_Pin },
};

static_assert(sizeof(_Config) / sizeof(_Config[0]) == eLED_NUM, "tLEDConfig size mismatch");
static_assert(sizeof(_GPIO) / sizeof(_GPIO[0]) == eLED_NUM, "tGPIOConfig size mismatch");

static bool _Initialized = false;       //!< True when module is initialized
static bool _Testing = false;           //!< True during POST
static tLEDRuntime _Runtime[eLED_NUM];  //!< Runtime data of the LEDs

/* Private function prototypes -----------------------------------------------*/

static void _OnOff(tLED led, bool On);
static void _ProcessFlash(tLED led);
static void _ProcessPulse(tLED led);
static void _ToggleFlash(tLED led);
static void _ProcessTest(void);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the LED interface
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool LED_Init(void)
{
  int led;

  // Initialize GPIO if not already initialized
  if (!_Initialized)
  {
    for (led = 0; led < eLED_NUM; led++)
    {
      _Runtime[led].Lit = false;
      _Runtime[led].Flash.Active = false;
      _Runtime[led].Flash.Lit = false;
      _Runtime[led].Flash.Interval_S = 0.5;
      _Runtime[led].Flash.Qty = 0;
      _Runtime[led].Flash.Pulses = 0;
      _Runtime[led].Flash.PulsesRemain = -1;
      _Runtime[led].Flash.FlashTime = 0;
      _Runtime[led].Flash.PulseTime = 0;
      _Runtime[led].Key[0] = '\0';
    }

    // Initialize GPIO LEDs
    _Initialized = true;

    if (_Initialized)
    {
      LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
    }
    else
    {
      LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Not Initialized");
    }
  }

  return _Initialized;
}

/*******************************************************************/
/*!
 @brief     Executive loop of the LED module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool LED_Exec(void)
{
  tLED led;

  // Process any LEDs testing
  if (!_Initialized)
  {
  }
  else if (_Testing)
  {
    _ProcessTest();
  }
  else
  {
    // Process any flashing LEDs
    for (led = (tLED)0; led < eLED_NUM; led++)
    {
      _ProcessFlash(led);
    }
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool LED_Shutdown(void)
{
  // No shutdown processing required
  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a test of the LED module
 @return    True if successful; no errors detected

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool LED_Test(void)
{
  // _Testing = true;

  return true;
}

/*******************************************************************/
/*!
 @brief     Turns on or off the desired LED
 @param     led: The selected LED
 @param     On: Turns on the selected LED if true, otherwise off
 @param     Key: Key used access LED. Key needs to match in order to turn on or off, if LED is locked
 @return    True if successful
 *******************************************************************/
bool LED_OnOff(tLED led, bool On, const char* Key)
{
  bool success = true;

  if (!_Initialized)
  {
    success = false;
  }
  else if (led >= eLED_NUM)
  {
    // Invalid LED
    success = false;
  }
  else if (_Runtime[led].Lit == On)
  {
    // No change
    success = false;
  }
  else if (_Runtime[led].Key[0] == '\0')
  {
    // LED is unlocked
    _OnOff(led, On);
  }
  else if (strncmp(_Runtime[led].Key, Key, _MaxKeyLength) == 0)
  {
    // Keys match
    _OnOff(led, On);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Toggles the desired LED
 @param     led: The selected LED
 @param     Key: Key used access LED. Key needs to match in order to turn on or off, if LED is locked
 @return    True if successful
 *******************************************************************/
bool LED_Toggle(tLED led, const char* Key)
{
  bool success = true;

  if (!_Initialized)
  {
    success = false;
  }
  else if (led >= eLED_NUM)
  {
    success = false;
  }
  else if (_Runtime[led].Key[0] == '\0')
  {
    // LED is unlocked
    _OnOff(led, !_Runtime[led].Lit);
  }
  else if (strncmp(_Runtime[led].Key, Key, _MaxKeyLength) == 0)
  {
    // Keys match
    _OnOff(led, !_Runtime[led].Lit);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Flashes the desired LED for the desired number of seconds
 @param     led: The selected LED
 @param     Secs: The desired period of the flashing
 @param     Qty: The number of times to flash. -1 = infinite
 @param     Key: Key used access LED. Key needs to match in order to turn on or off, if LED is locked
 @return    True if successful
 *******************************************************************/
bool LED_Flash(tLED led, float Secs, int32_t Qty, const char* Key)
{
  bool success = false;

  if (!_Initialized)
  {
  }
  else if (led >= eLED_NUM)
  {
  }
  else if (Secs < 0.10f)
  {
    // Too fast
  }
  else if (_Runtime[led].Key[0] == '\0')
  {
    // LED is unlocked
    _Runtime[led].Flash.Active = true;
    _Runtime[led].Flash.Interval_S = Secs;
    _Runtime[led].Flash.Qty = Qty - 1;
    _Runtime[led].Flash.Pulses = 0;
    _Runtime[led].Flash.PulsesRemain = -1;
    _Runtime[led].Flash.FlashTime = TIMER_GetTick();

    _OnOff(led, true);

    success = true;
  }
  else if (strncmp(_Runtime[led].Key, Key, _MaxKeyLength) == 0)
  {
    // Keys match
    _Runtime[led].Flash.Active = true;
    _Runtime[led].Flash.Interval_S = Secs;
    _Runtime[led].Flash.Qty = Qty - 1;
    _Runtime[led].Flash.Pulses = 0;
    _Runtime[led].Flash.PulsesRemain = -1;

    _Runtime[led].Flash.FlashTime = TIMER_GetTick();

    _OnOff(led, true);

    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Pulses the desired LED for the desired number of seconds
 @param     led: The selected LED
 @param     Secs: The desired period of the flashing
 @param     Qty: The number of times to flash. -1 = infinite
 @param     Pulses: The number of pulses per period
 @param     Key: Key used access LED. Key needs to match in order to turn on or off, if LED is locked
 @return    True if successful
 *******************************************************************/
bool LED_Pulse(tLED led, float Secs, int32_t Qty, int32_t Pulses, const char* Key)
{
  bool success = false;

  if (!_Initialized)
  {
  }
  else if (led < eLED_NUM)
  {
    if ((Pulses * _PulseDuration_S * 2) > Secs)
    {
      // Adjust Secs to fit in the requested pulses
      Secs = Pulses * _PulseDuration_S * 2;
    }

    LED_OnOff(led, false, Key);
    if (LED_Flash(led, 2 * Secs, Qty, Key))
    {
      _Runtime[led].Flash.Pulses = Pulses;
      _Runtime[led].Flash.PulsesRemain = Pulses;
      _Runtime[led].Flash.PulseTime = TIMER_GetTick();

      success = true;
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Returns the status of the LED
 @param     led: The selected LED
 @return    True if LED is on
 *******************************************************************/
bool LED_IsOn(tLED led)
{
  return _Initialized ? _Runtime[led].Lit : false;
}

/*******************************************************************/
/*!
 @brief     Locks the LED from changes. Useful if one module wants ownership of the LED
 @param     led: The selected LED
 @param     Key: Key used access LED. Key needs to match in order to turn on or off, if LED is locked
 @param     Lock: Locked if true
 *******************************************************************/
void LED_Lock(tLED led, bool Lock, const char* Key)
{
  if (led >= eLED_NUM)
  {
    // Say what?
  }
  else if (_DisableLocking)
  {
    // LED locking disabled
    _Runtime[led].Key[0] = '\0';
  }
  else if (_Runtime[led].Key[0] == '\0')
  {
    // LED is unlocked
    if (Lock)
    {
      // Module would like to lock the LED for it's exclusive use
      strncpy(_Runtime[led].Key, Key, _MaxKeyLength);

      // Just in case, truncate
      _Runtime[led].Key[_MaxKeyLength - 1] = '\0';
    }
    else
    {
      // Module is releasing the lock
      _Runtime[led].Key[0] = '\0';
    }
  }
  else if (strncmp(_Runtime[led].Key, Key, _MaxKeyLength) != 0)
  {
    // Not the owner
  }
  else if (Lock)
  {
    // Module would like to lock the LED for it's exclusive use
    strncpy(_Runtime[led].Key, Key, _MaxKeyLength);

    // Just in case, truncate
    _Runtime[led].Key[_MaxKeyLength - 1] = '\0';
  }
  else
  {
    // Module is releasing the lock
    _Runtime[led].Key[0] = '\0';
  }
}

/*******************************************************************/
/*!
 @brief		Returns the lock status of the LED
 @param     led: The selected LED
 @return	True if LED is locked
 *******************************************************************/
bool LED_IsLocked(tLED led)
{
  bool locked = false;

  if (led < eLED_NUM)
  {
    locked = _Runtime[led].Key[0] != '\0';
  }

  return locked;
}

/*******************************************************************/
/*!
 @brief		Returns the testing status
 @return	True if module is being tested
 *******************************************************************/
bool LED_IsTesting(void)
{
  return _Testing;
}

/* Private Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Turns on or off the desired LED
 @param     led: The selected LED
 @param     On: Turns on the selected LED if true, otherwise off
 *******************************************************************/
static void _OnOff(tLED led, bool On)
{
  bool level;

  if (led < eLED_NUM)
  {
    if (On)
    {
      level = _Config[led].OnPolarityH;
    }
    else
    {
      level = !_Config[led].OnPolarityH;

      // Cancel all flashing if turning off
      _Runtime[led].Flash.Lit = false;
      _Runtime[led].Flash.Active = false;
      _Runtime[led].Flash.Qty = 0;
      _Runtime[led].Flash.Pulses = 0;
      _Runtime[led].Flash.PulsesRemain = -1;
    }

    HAL_GPIO_WritePin(_GPIO[led].Port, _GPIO[led].Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    _Runtime[led].Lit = On;

    if (_Runtime[led].Flash.Active)
    {
      _Runtime[led].Flash.Lit = On;
    }
  }
}

/*******************************************************************/
/*!
 @brief     Process flashing
 @param     led: The selected LED
 *******************************************************************/
static void _ProcessFlash(tLED led)
{
  if (!_Runtime[led].Flash.Active)
  {
  }
  else if (!_Runtime[led].Lit)
  {
  }
  else if (_Runtime[led].Flash.PulsesRemain > 0)
  {
    // Pulse mode
    _ProcessPulse(led);
  }
  else if (TIMER_GetElapsed_s(_Runtime[led].Flash.FlashTime) > _Runtime[led].Flash.Interval_S)
  {
    if (_Runtime[led].Flash.Qty < 0)
    {
      // Flash/pulse forever
      if (_Runtime[led].Flash.Pulses > 0)
      {
        // Refresh pulse data
        _Runtime[led].Flash.PulsesRemain = _Runtime[led].Flash.Pulses;
        _Runtime[led].Flash.PulseTime = TIMER_GetTick();

        _ToggleFlash(led);
      }
      else
      {
        _ToggleFlash(led);
      }
    }
    else if (_Runtime[led].Flash.Qty > 0)
    {
      // Keep flashing/pulsing
      if (_Runtime[led].Flash.Pulses > 0)
      {
        // Refresh pulse data
        _Runtime[led].Flash.PulsesRemain = _Runtime[led].Flash.Pulses;
        _Runtime[led].Flash.PulseTime = TIMER_GetTick();

        _ToggleFlash(led);

        _Runtime[led].Flash.Qty--;
      }
      else
      {
        _ToggleFlash(led);

        // Update remaining
        if (_Runtime[led].Flash.Lit)
        {
          _Runtime[led].Flash.Qty--;
        }
      }
    }
    else
    {
      // Done flashing
      _OnOff(led, false);
    }

    _Runtime[led].Flash.FlashTime = TIMER_GetTick();
  }
}

/*******************************************************************/
/*!
 @brief     Process pulsing
 @param     led: The selected LED
 *******************************************************************/
static void _ProcessPulse(tLED led)
{
  if (!_Runtime[led].Flash.Active)
  {
  }
  else if (TIMER_GetElapsed_s(_Runtime[led].Flash.PulseTime) > (_PulseDuration_S / 2.0f))
  {
    _ToggleFlash(led);

    if (!_Runtime[led].Flash.Lit)
    {
      // Update remaining when starting a new pulse
      _Runtime[led].Flash.PulsesRemain--;
    }

    _Runtime[led].Flash.PulseTime = TIMER_GetTick();
  }
}

/*******************************************************************/
/*!
 @brief     Toggles the desired LED
 @param     led: The selected LED

 *******************************************************************/
static void _ToggleFlash(tLED led)
{
  bool on, level;

  if (!_Initialized)
  {
  }
  else if (led < eLED_NUM)
  {
    on = !_Runtime[led].Flash.Lit;
    level = on ? _Config[led].OnPolarityH : !_Config[led].OnPolarityH;

    HAL_GPIO_WritePin(_GPIO[led].Port, _GPIO[led].Pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    _Runtime[led].Flash.Lit = on;
  }
}

/*******************************************************************/
/*!
 @brief     Process POST testing
 *******************************************************************/
static void _ProcessTest(void)
{
  static bool testInProgress = false;
  static uint32_t time_test = 0;
  tLED led;

  if (testInProgress)
  {
    if (TIMER_GetElapsed_s(time_test) > _TestStep_S)
    {
      // Turn off LEDs
      for (led = (tLED)0; led < eLED_NUM; led++)
      {
        LED_OnOff(led, false, "");
      }

      // All done testing
      _Testing = testInProgress = false;
    }
  }
  else
  {
    testInProgress = true;

    // Turn on board LEDs
    for (led = (tLED)0; led < eLED_NUM; led++)
    {
      LED_OnOff(led, true, "");
    }

    time_test = TIMER_GetTick();
  }
}
