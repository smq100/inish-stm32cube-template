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
 @file    task_buttons.c
 @brief   Implementation for supporting hardware buttons
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#include "main.h"
#include "task_buttons.h"
#include "task_system.h"
#include "queue.h"
#include "timer.h"
#include "util.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

//! Button data structure
typedef struct
{
  bool IsPressed;          //!< Current state of button GPIO state
  bool IsPressed_Prev;     //!< Previous state of button GPIO state
  bool ChangeState;        //!< Change of state detected
  bool DebounceStarted;    //!< Debouncing process started
  uint16_t DebounceCount;  //!< Loop count of debounce state
  bool DebounceComplete;   //!< Debouce state completed
  bool HoldStarted;        //!< Button hold state started
  uint16_t HoldCount;      //!< Loop count of button hold state
  bool HoldComplete;       //!< Hold state completed
  bool RepeatStarted;      //!< Button repeat state started
  uint16_t RepeatCount;    //!< Loop count of button repeat state
  bool RepeatComplete;     //!< Repeat state completed
  bool Stuck;              //!< Button is detected as stuck
} tButtonStatus;

typedef struct
{
  bool ActiveHigh;   //!< Pressed is logic H at the GPIO when true
  tGPIOConfig GPIO;  //!< GPIO configuration
} tButtonConfig;

typedef struct
{
  bool Enabled;        //!< Enabled when true
  bool Locked;         //!< Locked when true
  bool StateChange;    //!< State has changed when true
  tButtonState State;  //!< Button state
} tButtonRuntime;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_BTN";          //!< Module name used for debug logging
static const uint16_t _DebounceTime_ms = 40u;  //!< Debounce time in ms
static const uint16_t _HoldTime_ms = 2000u;    //!< Button hold time in ms
static const uint16_t _RepeatTime_ms = 3000u;  //!< Starts after hold time has timed out
static const uint16_t _StuckTime_ms = 3000u;   //!< Milliseconds after boot that we check for stuck button

// clang-format off
//! Button GPIO configuration
static const tButtonConfig _Config[eButtonID_NUM] =
{
  { .ActiveHigh = false, .GPIO = { BTN_NUCLEO_GPIO_Port, BTN_NUCLEO_Pin } }
};
static_assert(sizeof(_Config) / sizeof(tButtonConfig) == eButtonID_NUM, "tButtonConfig size mismatch");

//! Button names for debug output
static const char* _Name[eButtonID_NUM] =
{
  "Nucleo",
};

//! Button states for debug output
static const char* _State[eButtonState_NUM] =
{
  "Pressed",
  "Released",
  "Hold",
  "Repeat",
  "Click",
};
// clang-format on

static_assert(sizeof(_Name) / sizeof(char*) == eButtonID_NUM, "tButtonName size mismatch");
static_assert(sizeof(_State) / sizeof(char*) == eButtonState_NUM, "tButtonState size mismatch");

static bool _Initialized = false;               //!< True when module is initialized
static tButtonRuntime _Runtime[eButtonID_NUM];  //!< Button runtime data
static tButtonStatus _Status[eButtonID_NUM];    //!< Button status
static uint16_t _DebounceTimeout;               //!< Debounce count
static uint16_t _HoldTimeout;                   //!< Hold count
static uint16_t _RepeatTimeout;                 //!< Repeat count
static tQueue _Queue;                           //!< Queue of button states
static uint8_t _BtnBuffer[BUTTON_BUFFER_SIZE];  //!< Buffer used in the queue
static fnButtonEventCB* _Callback = NULLPTR;    //!< Button event callback

/* Private function prototypes -----------------------------------------------*/

static void _ProcessState(void);
static void _ProcessButton(void);
static void _ProcessCommon(tButtonID Button);
static bool _GetStateChange(void);
static bool _IsStuck(void);
static void _TimerCallback(void);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the BUTTON interface
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool BUTTON_Init(void)
{
  if (!_Initialized)
  {
    // Initialize button inputs
    _DebounceTimeout = (uint16_t)(_DebounceTime_ms / TIMER_GetPeriod());
    _HoldTimeout = (uint16_t)(_HoldTime_ms / TIMER_GetPeriod());
    _RepeatTimeout = (uint16_t)(_RepeatTime_ms / TIMER_GetPeriod());

    // Create a queue to hold all button events
    bool init = QUEUE_Init(&_Queue, _BtnBuffer, sizeof(uint8_t), BUTTON_BUFFER_SIZE, "Button");
    assert(init);

    for (int i = 0; i < eButtonID_NUM; i++)
    {
      _Status[i].IsPressed = false;
      _Status[i].IsPressed_Prev = false;
      _Status[i].ChangeState = false;
      _Status[i].DebounceStarted = false;
      _Status[i].DebounceCount = 0;
      _Status[i].DebounceComplete = false;
      _Status[i].HoldStarted = false;
      _Status[i].HoldCount = 0;
      _Status[i].HoldComplete = false;
      _Status[i].RepeatStarted = false;
      _Status[i].RepeatCount = 0;
      _Status[i].RepeatComplete = false;
      _Status[i].Stuck = false;
    }

    // Register callbacks with timer
    if (TIMER_RegisterCallback(&_TimerCallback))
    {
      _Initialized = true;
    }
    else
    {
      LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Timer callback registration failed");
    }
  }

  if (_Initialized)
  {
    for (int i = 0; i < eButtonID_NUM; i++)
    {
      _Runtime[i].Enabled = true;
      _Runtime[i].Locked = false;
      _Runtime[i].StateChange = false;
      _Runtime[i].State = eButtonState_Released;
    }

    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Not Initialized");
  }

  return _Initialized;
}

/*******************************************************************/
/*!
 @brief     Executive loop of the BUTTON module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool BUTTON_Exec(void)
{
  _ProcessState();
  _ProcessButton();

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool BUTTON_Shutdown(void)
{
  // No shutdown processing required
  return true;
}

/*******************************************************************/
/*!
 @brief     POST routine for BUTTON module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool BUTTON_Test(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Checks and returns a button event if available
 @param     Button: button ID
 @param     State: State of the button
 @return	true if button/state available in buffer, otherwise false

 *******************************************************************/
bool BUTTON_GetButtonEvent(tButtonID* const Button, tButtonState* const State)
{
  bool new_event = false;
  uint8_t button;

  if (!_Initialized)
  {
  }
  else if (!IsRAM((uintptr_t)Button))
  {
    assert_always();
  }
  else if (!IsRAM((uintptr_t)State))
  {
    assert_always();
  }
  else if (QUEUE_Deque(&_Queue, &button))
  {
    *Button = (tButtonID)(button >> BUTTON_STATE_BITS);
    *State = (tButtonState)(button & BUTTON_STATE_MASK);
    new_event = true;
  }

  return new_event;
}

/*******************************************************************/
/*!
 @brief     Get the current state of a button
 @param     Button: The desired button
 @return    The button state

 *******************************************************************/
tButtonState BUTTON_GetState(tButtonID Button)
{
  tButtonState state;

  if (Button < eButtonID_NUM)
  {
    state = _Runtime[Button].State;
  }
  else
  {
    state = eButtonState_Released;
  }

  return state;
}

/*******************************************************************/
/*!
 @brief     Returns the chord state of button (a chord is 2 or more buttons in a simultaneous held state)
 @return    The button chord as a bit mask of buttons

 *******************************************************************/
uint32_t BUTTON_GetChord(void)
{
  uint32_t button;
  uint32_t found;
  uint32_t chord;

  for (button = 0, found = 0, chord = 0; button < eButtonID_NUM; button++)
  {
    if (_Runtime[button].State == eButtonState_Hold)
    {
      chord |= (1 << button);
      found++;
    }
  }

  return (found > 1) ? chord : 0;
}

/*******************************************************************/
/*!
 @brief     Return the raw state of the button GPIO state (no de-bouncing)
 @return    True if pressed

 *******************************************************************/
bool BUTTON_IsPressedRaw(tButtonID Button)
{
  bool pressed = false;

  if (Button < eButtonID_NUM)
  {
    GPIO_PinState polarity = _Config[Button].ActiveHigh ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState state = HAL_GPIO_ReadPin(_Config[Button].GPIO.Port, _Config[Button].GPIO.Pin);
    pressed = (state == polarity);
  }

  return pressed;
}

/*******************************************************************/
/*!
 @brief     Returns the stuck status of the buttons
 @return    True if any button is stuck

 *******************************************************************/
bool BUTTON_IsStuck(void)
{
  bool stuck = false;

  for (int i = 0; i < eButtonID_NUM; i++)
  {
    if (_Status[i].Stuck)
    {
      stuck = true;
      break;
    }
  }

  return stuck;
}

/*******************************************************************/
/*!
 @brief     Registers a callback for button events
 @param     Callback: The desired callback

 *******************************************************************/
void BUTTON_RegisterCallback(fnButtonEventCB* Callback)
{
  _Callback = Callback;
}

/******************************************************************************
 * Implementation (private)
 ******************************************************************************/

/*******************************************************************/
/*!
 @brief     Process button states

 *******************************************************************/
static void _ProcessState(void)
{
  static tButtonID button = (tButtonID)0;
  uint8_t newState;
  bool queued;

  if (_Initialized)
  {
    _Status[button].IsPressed = BUTTON_IsPressedRaw(button);

    if (_Status[button].IsPressed == _Status[button].IsPressed_Prev)
    {
      // No change, but check debounce status
      if (_Status[button].DebounceStarted)
      {
        if (_Status[button].DebounceComplete)
        {
          // Debounce complete
          _Status[button].DebounceStarted = false;
          _Status[button].ChangeState = true;
        }
      }
    }
    else if (!_Status[button].DebounceStarted)
    {
      // New change-of-state; Start debounce timer
      _Status[button].IsPressed_Prev = _Status[button].IsPressed;
      _Status[button].ChangeState = true;
      _Status[button].DebounceCount = _DebounceTimeout;
      _Status[button].DebounceComplete = false;
      _Status[button].DebounceStarted = true;
    }
    else
    {
      // Debounce failed (input reversed during debounce)
      _Status[button].DebounceStarted = false;
      _Status[button].ChangeState = false;
    }

    // Check for a (debounced) button state change
    if (_Status[button].ChangeState && _Status[button].DebounceComplete)
    {
      // Change-of-state (debounced)
      _Status[button].IsPressed_Prev = _Status[button].IsPressed;
      _Status[button].DebounceStarted = false;
      _Status[button].ChangeState = false;

      // Record state change (push to buffer)
      if (_Status[button].IsPressed)
      {
        // Button pressed
        newState = (uint8_t)(button << BUTTON_STATE_BITS | (eButtonState_Pressed & BUTTON_STATE_MASK));
        queued = QUEUE_Enque(&_Queue, &newState);
        if (!queued)
        {
          LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Button queue full");
        }

        // Start hold timing
        _Status[button].HoldCount = _HoldTimeout;
        _Status[button].HoldComplete = false;
        _Status[button].HoldStarted = true;
        _Status[button].RepeatStarted = false;
      }
      else
      {
        // Button released
        newState = (uint8_t)(button << BUTTON_STATE_BITS | (eButtonState_Released & BUTTON_STATE_MASK));
        queued = QUEUE_Enque(&_Queue, &newState);
        if (!queued)
        {
          LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Button queue full");
        }

        // Check if button released before hold timeout (i.e. button click)
        if (_Status[button].HoldStarted)
        {
          // Button clicked
          newState = (uint8_t)(button << BUTTON_STATE_BITS | (eButtonState_Click & BUTTON_STATE_MASK));
          queued = QUEUE_Enque(&_Queue, &newState);
          if (!queued)
          {
            LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Button queue full");
          }
        }

        // Stop hold and repeat timing
        _Status[button].HoldStarted = false;
        _Status[button].RepeatStarted = false;
      }
    }

    // Check if button is being held (pressed)
    if (_Status[button].IsPressed_Prev && _Status[button].HoldStarted && !_Status[button].RepeatStarted)
    {
      if (_Status[button].HoldComplete)
      {
        // Button hold
        newState = (uint8_t)(button << BUTTON_STATE_BITS | (eButtonState_Hold & BUTTON_STATE_MASK));
        queued = QUEUE_Enque(&_Queue, &newState);
        if (!queued)
        {
          LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Button queue full");
        }

        // Stop hold timing
        _Status[button].HoldStarted = false;

        // Start repeat timing
        _Status[button].RepeatCount = _RepeatTimeout;
        _Status[button].RepeatComplete = false;
        _Status[button].RepeatStarted = true;
      }
    }

    // check if button is still being held (pressed)
    if ((_Status[button].IsPressed_Prev) && !_Status[button].HoldStarted && _Status[button].RepeatStarted)
    {
      // check for repeat timeout
      if (_Status[button].RepeatComplete)
      {
        // repeat timer has timed out, log hold state to circular buffer
        newState = (uint8_t)(button << BUTTON_STATE_BITS | (eButtonState_Repeat & BUTTON_STATE_MASK));
        if (!QUEUE_Enque(&_Queue, &newState))
        {
          LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Button queue full");
        }

        // restart Repeat timer
        _Status[button].RepeatCount = _RepeatTimeout;
        _Status[button].RepeatComplete = false;
      }
    }

    // Increment for next time
    if (++button >= eButtonID_NUM)
    {
      button = (tButtonID)0;
    }

    // Check for stuck status when first starting
    if (SYSTEM_GetUpTime_MS() <= (_StuckTime_ms + 1000u))
    {
      _IsStuck();
    }
  }
}

/*******************************************************************/
/*!
 @brief     Process any button state changes

 *******************************************************************/
static void _ProcessButton(void)
{
  tButtonID button;

  if (_GetStateChange())
  {
    for (button = (tButtonID)0; button < eButtonID_NUM; button++)
    {
      if (!_Runtime[button].Enabled)
      {
      }
      else if (_Runtime[button].Locked)
      {
      }
      else if (_Runtime[button].StateChange)
      {
        _Runtime[button].StateChange = false;

        _ProcessCommon(button);

        // Process callback if registered
        if (_Callback != NULLPTR)
        {
          _Callback(button, _Runtime[button].State);
        }
      }
    }
  }
}

/*******************************************************************/
/*!
 @brief     Log event to debug port
 @param     Button: The desired button

 *******************************************************************/
static void _ProcessCommon(tButtonID Button)
{
  uint32_t chord;

  LOG_Write(
    eLogger_Sys, eLogLevel_Low, _Module, false, "Button: %s, %s", _Name[Button], _State[_Runtime[Button].State]);

  chord = BUTTON_GetChord();
  if (chord > 0)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "Chord: 0x%08lX", chord);
  }
}

/*******************************************************************/
/*!
 @brief     Process button state changes
 @return    True if successful

 *******************************************************************/
static bool _GetStateChange(void)
{
  bool ret = false;
  tButtonID button;
  tButtonState state;

  if (BUTTON_GetButtonEvent(&button, &state))
  {
    if (button >= eButtonID_NUM)
    {
      // Invalid button
    }
    else if (state >= eButtonState_NUM)
    {
      // Invalid state
    }
    else
    {
      // Button event
      _Runtime[button].State = state;
      _Runtime[button].StateChange = true;

      ret = true;
    }
  }

  return ret;
}

/*******************************************************************/
/*!
 @brief     Checks for stuck button conditions
 @return    True id any button is stuck

 *******************************************************************/
static bool _IsStuck(void)
{
  static uint32_t time = 0;
  static bool prevState[eButtonID_NUM] = { false };
  static uint32_t count[eButtonID_NUM] = { 0 };
  bool ret = false;
  bool pressed;

  if (TIMER_GetElapsed_ms(time) > 1000)
  {
    for (tButtonID i = (tButtonID)0; i < eButtonID_NUM; i++)
    {
      // Count how many loops the button has been detected as pressed
      pressed = BUTTON_IsPressedRaw(i);
      if (pressed)
      {
        if (prevState[i])
        {
          count[i]++;
        }
        else
        {
          count[i] = 1;
        }
      }
      else
      {
        count[i] = 0;
        _Status[i].Stuck = false;
      }

      // Three strikes and you're out
      if (count[i] >= 3)
      {
        _Status[i].Stuck = ret = true;
      }

      prevState[i] = pressed;
    }

    time = TIMER_GetTick();
  }

  return ret;
}

/*******************************************************************/
/*!
 @brief     Called by timer to update the button state timers
 *******************************************************************/
static void _TimerCallback(void)
{
  for (int i = 0; i < eButtonID_NUM; i++)
  {
    if (_Status[i].DebounceCount > 0)
    {
      _Status[i].DebounceCount--;
    }
    else
    {
      _Status[i].DebounceComplete = true;
    }

    if (_Status[i].HoldCount > 0)
    {
      _Status[i].HoldCount--;
    }
    else
    {
      _Status[i].HoldComplete = true;
    }

    if (_Status[i].RepeatCount > 0)
    {
      _Status[i].RepeatCount--;
    }
    else
    {
      _Status[i].RepeatComplete = true;
    }
  }
}
