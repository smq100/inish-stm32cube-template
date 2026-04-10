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

 @file    task_app.c
 @brief   Implementation of the APPLICATION task
 @author  Steve Quinlan
 @date    2026-March
 @see     task.c

 ******************************************************************************/

#define TASK_APP_PROTECTED

#include "task_app.h"
#include "task_buttons.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_APP";  //!< Module name for debug logging
static bool _Initialized = false;      //!< True when module is initialized

/* Private function prototypes -----------------------------------------------*/

static fnButtonEventCB _BtnCallback;

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the task
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool APP_Init(void)
{
  // Perform initialization of the task here. This can include initializing hardware, software, and data structures.
  // If initialization is successful, return true. If initialization fails, return false to disable the
  // task and prevent it from being scheduled.

  // Register button event callback to process buttons
  BUTTON_RegisterCallback(_BtnCallback);

  _Initialized = true;

  if (_Initialized)
  {
    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Not Initialized");
  }

  return _Initialized;
}

/*******************************************************************/
/*!
 @brief     Executive loop of the APPLICATION module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool APP_Exec(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing of the APPLICATION module
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool APP_Shutdown(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a test of the APPLICATION module
 @return    True if successful; no errors detected

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool APP_Test(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a Class B test of the APPLICATION module
 @details   Class B tests are a set of tests defined by the IEC 60730 standard
 @return    True if successful; no errors detected

 *******************************************************************/
bool APP_ClassBTest(void)
{
  return true;
}

/* Private Implementation ----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Callback function for button events
 @param     Button  The ID of the button that triggered the event
 @param     State   The state of the button (pressed or released)
 @return    None

 *******************************************************************/
static void _BtnCallback(tButtonID Button, tButtonState State)
{
  LOG_Write(eLogger_Sys, eLogLevel_Med, _Module, false, "Button %u State %u", Button, State);
}
