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

 @file    task_classb.c
 @brief   Implementation of the CLASSB task
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#define TASK_CLASSB_PROTECTED

#include "main.h"
#include "task_classb.h"
#include "task_daq.h"
#include "classb.h"
#include "classb_runtime.h"
#include "classb_vars.h"
#include "watchdog.h"
#include "timer.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

//! Command handler
typedef struct
{
  fnClassBHandler_Runtime* Handler;  ///< Command handler
  uint32_t MaxErrors;                ///< Max errors before test is marked failed
  uint32_t ErrorReset_Sec;           ///< Time in seconds to reset error count after a failure
  char* Name;                        ///< Name of the command
} tConfig;

typedef struct
{
  bool Initialized;                             ///< True when initialized
  uint32_t Errors[eClassBRunItem_NUM];          ///< Current error count for each test type
  uint32_t ErrorsTotal[eClassBRunItem_NUM];     ///< Total errors since boot (not reset after timeout)
  uint32_t ErrorTimestamp[eClassBRunItem_NUM];  ///< Timestamp of last error for each test type (used for timeout)
  tClassBRunItem Item;                          ///< Test state
} tRuntime;

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_CLSB";  //!< Module Name for debug logging
static tRuntime _Runtime;               //!< Runtime data

/* Private function prototypes -----------------------------------------------*/

static const fnClassBHandler_Runtime _Handler_CPU;
static const fnClassBHandler_Runtime _Handler_RAM;
static const fnClassBHandler_Runtime _Handler_CRC;
static const fnClassBHandler_Runtime _Handler_ADC;
static const fnClassBHandler_Runtime _Handler_CLK;
static const fnClassBHandler_Runtime _Handler_WDG;
static const fnClassBHandler_Runtime _Handler_Stack;
static const fnClassBHandler_Runtime _Handler_App;
static const fnClassBHandler_Runtime _Handler_Flow;

static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item);

// clang-format off
//! Tech commands and associated handlers (must be same order as tClassBRunItem enum)
static const tConfig _Config[] = {
  { .Handler = _Handler_CPU  , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "cpu",   },
  { .Handler = _Handler_RAM  , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "ram",   },
  { .Handler = _Handler_CRC  , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "crc",   },
  { .Handler = _Handler_ADC  , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "adc",   },
  { .Handler = _Handler_CLK  , .MaxErrors = 3u, .ErrorReset_Sec = 10u, .Name = "clk",   },
  { .Handler = _Handler_WDG  , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "wdg",   },
  { .Handler = _Handler_Stack, .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "stack", },
  { .Handler = _Handler_App  , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "app",  },
  { .Handler = _Handler_Flow , .MaxErrors = 1u, .ErrorReset_Sec = 10u, .Name = "flow",  },
};
// clang-format on

static_assert(sizeof(_Config) / sizeof(tConfig) == eClassBRunItem_NUM, "tConfig size mismatch");

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the task
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool CLASSB_Init(void)
{
  // Let's get started

  if (!_Runtime.Initialized)
  {
    ClassB_VarsInit();
    _Runtime.Initialized = Runtime_Init();

    _Runtime.Item = (tClassBRunItem)0;

    for (int item = 0; item < eClassBRunItem_NUM; item++)
    {
      _Runtime.Errors[item] = 0;
      _Runtime.ErrorsTotal[item] = 0;
      _Runtime.ErrorTimestamp[item] = 0;
    }

    // Stop timer for CLK test now
    HAL_TIM_IC_Stop_IT(&hTIM_CLKTEST, TIM_CLKTEST_CH);

    // Register DAQ items
    tDAQ_Config daq;
    daq.CallbackRead = _DAQReadCallback;
    daq.CallbackWrite = NULLPTR;  // Read-only
    DAQ_RegisterItem(eDAQ_ClassBTest, &daq, false);

    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Not Initialized");
  }

  return _Runtime.Initialized;
}

/*******************************************************************/
/*!
 @brief     Executive loop of the CLASSB module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool CLASSB_Exec(void)
{
  char name_buf[10];
  tClassBRunStatus status;
  tClassBRunItem current = _Runtime.Item;

  bool enabled = ClassB_IsRuntimeTestEnabled(current, name_buf);

  if (_Runtime.Errors[current] < _Config[current].MaxErrors)
  {
    if (_Runtime.Errors[current] == 0)
    {
    }
    else if (TIMER_GetElapsed_ms(_Runtime.ErrorTimestamp[current]) >= (_Config[current].ErrorReset_Sec * 1000u))
    {
      // Reset error count after timeout
      _Runtime.Errors[current] = 0;
      _Runtime.ErrorTimestamp[current] = 0;

      LOG_Write(eLogger_Sys, eLogLevel_Med, _Module, false, "Resetting error count for '%s'", name_buf);
    }

    // Run handler normally if no previous error
    status = _Config[current].Handler(enabled);
  }
  else
  {
    // If previous error, run handler with enabled=false to skip test but
    // still run any necessary code (e.g. control flow)
    status = _Config[current].Handler(false);

    WDG_SetHeartbeat(CLASSB_WDG_HB_CLASSB);
  }

  if (status == eClassBRunStatus_NOT_STARTED)
  {
  }
  else if (_Runtime.Errors[current] >= _Config[current].MaxErrors)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "ClassB '%s' test skipping (previously failed)", name_buf);
  }
  else if (status == eClassBRunStatus_PASS)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "ClassB '%s' test passed", name_buf);
  }
  else if (status == eClassBRunStatus_IN_PROGRESS)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "ClassB '%s' test in progress...", name_buf);
  }
  else if (status == eClassBRunStatus_FAIL)
  {
    _Runtime.Errors[current]++;
    _Runtime.ErrorsTotal[current]++;
    _Runtime.ErrorTimestamp[current] = TIMER_GetTick();

    if (_Runtime.Errors[current] >= _Config[current].MaxErrors)
    {
      ClassB_Fail_Runtime(current);

      LOG_Write(eLogger_Sys,
                eLogLevel_Error,
                _Module,
                true,
                "ClassB '%s' test failed (%lu)",
                name_buf,
                _Runtime.Errors[current]);
    }
    else
    {
      LOG_Write(eLogger_Sys,
                eLogLevel_Warning,
                _Module,
                true,
                "ClassB '%s' test warning (%lu of max %lu)",
                name_buf,
                _Runtime.Errors[current],
                _Config[current].MaxErrors);
    }
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "ClassB '%s' test status unknown", name_buf);
    assert_always();
  }

  _Runtime.Item++;
  if (_Runtime.Item >= eClassBRunItem_NUM)
  {
    _Runtime.Item = (tClassBRunItem)0;
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing of the CLASSB module
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool CLASSB_Shutdown(void)
{
  // Force re-initialization of ClassB variables to reset any persistent variables
  ClassB_InitVars(true);

  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a test of the CLASSB module
 @return    True if successful; no errors detected

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool CLASSB_Test(void)
{
  return true;
}

/* Protected Implementation --------------------------------------------------*/

/*******************************************************************/
/*!
 @brief   Returns whether all runtime tests passed
 @param   None
 @return  true if all tests passed, false otherwise
*******************************************************************/
bool CLASSB_DidAllRuntimePass(void)
{
  bool success = true;
  for (tClassBRunItem i = 0; i < eClassBRunItem_NUM; i++)
  {
    if (!_Runtime.Errors[i])
    {
      success = false;
      break;
    }
  }

  return success;
}

/* Private Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Handles the CPU test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_CPU(bool enabled)
{
  return Runtime_CPUTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the RAM test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_RAM(bool enabled)
{
  return Runtime_RAMTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the CLK test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_CLK(bool enabled)
{
  return Runtime_CLKTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the ADC test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_ADC(bool enabled)
{
  return Runtime_ADCTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the CRC test
 @param     None
 @return    Status of the operation

 *******************************************************************/
static tClassBRunStatus _Handler_CRC(bool enabled)
{
  return Runtime_CRCTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the WDG test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_WDG(bool enabled)
{
  return Runtime_WDGTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the Stack test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_Stack(bool enabled)
{
  return Runtime_STKTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the Application test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_App(bool enabled)
{
  return Runtime_APPTest(enabled);
}

/*******************************************************************/
/*!
 @brief     Handles the Flow test
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)

 *******************************************************************/
static tClassBRunStatus _Handler_Flow(bool enabled)
{
  return Runtime_FLOWTest(enabled);
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

  if (Entry == eDAQ_ClassBTest)
  {
    value.U32 = _Runtime.ErrorsTotal[Item];
    DAQ_UpdateItem(Entry, Item, value);
  }
  else
  {
    success = false;
    assert_param(false);
  }

  return success;
}
