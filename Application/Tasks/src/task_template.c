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

 @file    task_template.c
 @brief   Implementation of the TEMPLATE task
 @author  Steve Quinlan
 @date    2026-March
 @see     task.c

 ******************************************************************************/

#define TASK_TEMPLATE_PROTECTED

#include "task_template.h"  // TODO rename
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "TEMPLATE";  //!< Module name for debug logging (TODO rename)
static bool _Initialized = false;         //!< True when module is initialized

/* Private function prototypes -----------------------------------------------*/
/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the task
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TEMPLATE_Init(void)
{
  // Perform initialization of the task here. This can include initializing hardware, software, and data structures.
  // If initialization is successful, return true. If initialization fails, return false to disable the
  // task and prevent it from being scheduled.

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
 @brief     Executive loop of the TEMPLATE module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TEMPLATE_Exec(void)
{
  // Exec should always return true to indicate the task is still healthy and should continue to be scheduled.
  // If false is returned, the task will be removed from the scheduler and not executed again.

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing of the TEMPLATE module
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TEMPLATE_Shutdown(void)
{
  // No shutdown processing required if true, othereise return false to prevent
  // task powerdown until shutdown processing is complete.
  // This allows the task to perform any necessary cleanup before the system powers down.
  // If false is returned, the task will continue to be scheduled until it returns true.

  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a test of the TEMPLATE module
 @return    True if successful; no errors detected

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool TEMPLATE_Test(void)
{
  // Perform testing of the task here. This can be called as part of the system POST (Power On Self Test)
  // or can be called at any time to perform a health check of the task.
  // If false is returned, the task will be removed from the scheduler and not executed again

  return true;
}
