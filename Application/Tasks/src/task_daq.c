/*!
*****************************************************************************
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
* @file    task_daq.c
* @brief   Implementation of the DAQ task
* @author  Steve Quinlan
* @date    2026-March
*
* @note    See the document technician.md for a complete description of the DAQ technician interface
*
******************************************************************************/

#include "main.h"
#include "task_daq.h"

#include "classb.h"
#include "log.h"
#include "timer.h"
#include "task_system.h"
#include "util.h"

/* Private typedef -----------------------------------------------------------*/

//! DAQ data value
typedef struct
{
  tDataValue Value[DAQ_MAX_SUBITEMS];
  uint32_t Time;
} tRuntime;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_DAQ";  //!< Module name for debug logging
static const tDAQ_Config _Defaults[] = {
  // eDAQ_NOP
  { .Type = eDataType_U32,
    .Enabled = false,
    .Interval = 5000,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = 1,
    .Units = "",
    .Description = "nop",
    .Name = "nop" },

  // eDAQ_Test
  { .Type = eDataType_Float,
    .Enabled = false,
    .Interval = 5,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = 1,
    .Units = "",
    .Description = "sawtooth test pattern",
    .Name = "test" },

  // eDAQ_TempPCB
  { .Type = eDataType_Float,
    .Enabled = false,
    .Interval = 500,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = 1,
    .Units = "DegC",
    .Description = "pcb temperature",
    .Name = "pcbtemp" },

  // eDAQ_Vref
  { .Type = eDataType_Float,
    .Enabled = false,
    .Interval = 501,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = 1,
    .Units = "V",
    .Description = "vref voltage",
    .Name = "vref" },

  // eDAQ_Uptime
  { .Type = eDataType_U32,
    .Enabled = false,
    .Interval = 25,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = 1,
    .Units = "ms",
    .Description = "system uptime",
    .Name = "uptime" },

  // eDAQ_Sleep
  { .Type = eDataType_U32,
    .Enabled = false,
    .Interval = 25,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = 1,
    .Units = "ms",
    .Description = "power sleep time",
    .Name = "sleep" },

  // eDAQ_ClassBTest
  { .Type = eDataType_U32,
    .Enabled = false,
    .Interval = 253,
    .ReadOnly = true,
    .Modified = false,
    .Scale = 1.0f,
    .Min = 0.0f,
    .Max = 0.0f,
    .CallbackRead = NULLPTR,
    .CallbackWrite = NULLPTR,
    .Items = eClassBRunItem_NUM,
    .Units = "",
    .Description = "test status",
    .Name = "rttest" },
};

static_assert(sizeof(_Defaults) / sizeof(tDAQ_Config) == eDAQ_Entry_NUM, "tDAQ_Config size mismatch");

static bool _Initialized = false;             //!< True when module is initialized
static tDAQ_Config _Config[eDAQ_Entry_NUM];   //!< Item configuration
static tRuntime _Runtime[eDAQ_Entry_NUM];     //!< Item runtime data
static uint8_t _MaxItems = DAQ_MAX_SUBITEMS;  //!< Maximum number of data items

/* Private function prototypes -----------------------------------------------*/
static fnDAQCallbackRead _TestPattern;
static fnDAQCallbackRead _NopRead;

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the task
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool DAQ_Init(void)
{
  tDAQ_Config daq;

  // Configure with the defaults and initialize
  for (tDAQ_Entry i = (tDAQ_Entry)0; i < eDAQ_Entry_NUM; i++)
  {
    _Config[i] = _Defaults[i];

    for (int j = 0; j < _MaxItems; j++)
    {
      _Runtime[i].Value[j].U32 = 0;
    }

    _Runtime[i].Time = 0;
  }

  // Register some base entries defined in this module
  daq.CallbackRead = _NopRead;
  daq.CallbackWrite = NULLPTR;
  DAQ_RegisterItem(eDAQ_NOP, &daq, false);

  daq.CallbackRead = _TestPattern;
  DAQ_RegisterItem(eDAQ_Test, &daq, false);

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
 @brief     Executive loop of the DAQ module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool DAQ_Exec(void)
{
  for (tDAQ_Entry i = (tDAQ_Entry)0; i < eDAQ_Entry_NUM; i++)
  {
    if (!_Config[i].Enabled)
    {
      // Disabled
    }
    else if (_Config[i].CallbackRead == NULLPTR)
    {
      // No callback specified
    }
    else if (TIMER_GetElapsed_ms(_Runtime[i].Time) >= _Config[i].Interval)
    {
      // Call the client callback to update the data
      for (int j = 0; j < _Config[i].Items; j++)
      {
        _Config[i].CallbackRead(i, j);
      }

      _Runtime[i].Time = TIMER_GetTick();
    }
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing of the DAQ module
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool DAQ_Shutdown(void)
{
  // No shutdown processing required
  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a test of the DAQ module
 @return    True if successful; no errors detected

 @note      Function takes the form of the TASK prototypes (see task.c/h)

 *******************************************************************/
bool DAQ_Test(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Register a DAQ item to be available
 @param     Entry: The DAQ item to be registered
 @param     Config: The pointer to the configuration struct
 @param     All: Override all metadata when true, otherwise just the callbacks
 @return    True if successful

 *******************************************************************/
bool DAQ_RegisterItem(tDAQ_Entry Entry, tDAQ_Config* Config, bool All)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Config))
  {
    assert_always();
  }
  else if (Entry >= eDAQ_Entry_NUM)
  {
    // Should never be here
    assert_always();
  }
  else if (All)
  {
    _Config[Entry] = *Config;
    _Config[Entry].Enabled = true;
    success = true;
  }
  else
  {
    _Config[Entry].CallbackRead = Config->CallbackRead;
    _Config[Entry].CallbackWrite = Config->CallbackWrite;
    _Config[Entry].Enabled = true;
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Get a DAQ item's metadata
 @param     Entry: The DAQ item
 @param     Config: A pointer to The DAQ item's data
 @return    True if successful

 *******************************************************************/
bool DAQ_GetMetadata(tDAQ_Entry Entry, tDAQ_Config* Config)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Config))
  {
    assert_always();
  }
  else if (Entry < eDAQ_Entry_NUM)
  {
    *Config = _Config[Entry];
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Get a DAQ item's data
 @param     Entry: The DAQ entry
 @param     Item: The DAQ item
 @param     Value: A pointer to the DAQ item's data
 @return    True if successful

 *******************************************************************/
bool DAQ_GetItem(tDAQ_Entry Entry, uint8_t Item, tDataValue* Value)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Value))
  {
    assert_always();
  }
  else if (Entry >= eDAQ_Entry_NUM)
  {
    // Invalid entry
  }
  else if (Item >= _Config[Entry].Items)
  {
    // Invalid item
  }
  else if (_Config[Entry].Enabled)
  {
    *Value = _Runtime[Entry].Value[Item];
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Update a DAQ item's data
 @param     Entry: The DAQ entry
 @param     Item: The DAQ item
 @param     Value: The DAQ item's data
 @return    True if successful

 *******************************************************************/
bool DAQ_UpdateItem(tDAQ_Entry Entry, uint8_t Item, tDataValue Value)
{
  bool success = false;

  if (Entry >= eDAQ_Entry_NUM)
  {
    // Invalid entry
  }
  else if (Item >= _MaxItems)
  {
    // Invalid item
  }
  else if (_Config[Entry].Enabled)
  {
    _Runtime[Entry].Value[Item] = Value;
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Write a DAQ item's data
 @param     Entry: The DAQ entry
 @param     Item: The DAQ item
 @param     Value: The DAQ item's data
 @return    True if successful

 *******************************************************************/
bool DAQ_WriteItem(tDAQ_Entry Entry, uint8_t Item, tDataValue Value)
{
  bool success = false;

  if (Entry >= eDAQ_Entry_NUM)
  {
    // Invalid entry
  }
  else if (Item >= _MaxItems)
  {
    // Invalid item
  }
  else if (!_Config[Entry].Enabled)
  {
    // Disabled
  }
  else if (_Config[Entry].ReadOnly)
  {
    // Read only
  }
  else if (_Config[Entry].CallbackWrite == NULLPTR)
  {
    // No write callback registered
  }
  else
  {
    // Call the write callback to modify the value
    // Value is always passed as a float. The callback should cast as appropriate
    success = _Config[Entry].CallbackWrite(Entry, Item, Value);
    _Config[Entry].Modified = success;

    LOG_Write(eLogger_Sys,
              eLogLevel_Low,
              _Module,
              false,
              "Write '%s' : %s%.2f %s",
              _Config[Entry].Name,
              success ? "" : "fail ",
              Value.Float,
              _Config[Entry].Units);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Provides 1Hz sawtooth DAQ test pattern for testing DAQ clients
 @param     Entry: The DAQ entry
 @param     Item: The DAQ item
 @return    True if successful

 *******************************************************************/
static bool _TestPattern(tDAQ_Entry Entry, uint8_t Item)
{
  tDataValue value;

  if (Entry == eDAQ_NOP)
  {
    value.U32 = 0;

    DAQ_UpdateItem(Entry, Item, value);
  }
  else if (Entry == eDAQ_Test)
  {
    // 1Hz sawtooth value between 0.0 and 1.0
    uint32_t mod = SYSTEM_GetUpTime_MS() % 1000;
    value.Float = (float)mod / 1000;

    DAQ_UpdateItem(Entry, Item, value);
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     DAQ read stub
 @param     Entry: The DAQ entry
 @param     Item: The DAQ item
 @return    Always true

 *******************************************************************/
static bool _NopRead(tDAQ_Entry Entry, uint8_t Item)
{
  return true;
}
