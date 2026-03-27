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

 @file    task_daq.h
 @brief   Interface for the DAQ task
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __TASK_DAQ_H
#define __TASK_DAQ_H

#include "common.h"
#include "task.h"

/* Exported types ------------------------------------------------------------*/

//! The available DAQ items. When adding items update tech.c::_Handler_Get() for proper output formatting
typedef enum
{
  eDAQ_NOP,      ///< No operation
  eDAQ_Test,     ///< Test read
  eDAQ_TempPCB,  ///< PCB Temp
  eDAQ_Vref,     ///< ADC Vref
  eDAQ_Uptime,   ///< Uptime. Note: This is not necessarily the time since boot because it is not updated during sleep
  eDAQ_Sleep,    ///< Sleep time
  eDAQ_ClassBTest,  ///< ClassB runtime test status
  eDAQ_Entry_NUM
} tDAQ_Entry;

//! Prototype for callback
typedef bool(fnDAQCallbackRead)(tDAQ_Entry Entry, uint8_t Item);
typedef bool(fnDAQCallbackWrite)(tDAQ_Entry Entry, uint8_t Item, tDataValue Value);

//! DAQ item configuration
typedef struct
{
  tDataType Type;                     ///< Data type
  bool Enabled;                       ///< Enabled if true
  bool ReadOnly;                      ///< Readonly of true
  uint32_t Interval;                  ///< Update interval in ms
  bool Modified;                      ///< Modified of true
  float Scale;                        ///< Optional scaling of data
  float Min;                          ///< Min allowed value (future)
  float Max;                          ///< Max allowed value (future)
  fnDAQCallbackRead* CallbackRead;    ///< Callback function to retrieve data
  fnDAQCallbackWrite* CallbackWrite;  ///< Callback function to modify data
  uint8_t Items;                      ///< Number of sub items
  const char* Units;                  ///< Const pointer to string of units description
  const char* Description;            ///< Const pointer to string of description
  const char* Name;                   ///< Const pointer to name abbreviation. Must be unique to all items
} tDAQ_Config;

/* Exported constants --------------------------------------------------------*/

//! Maximum number of data values. Set to the number of tasks because that is where multiple data value are used
#define DAQ_MAX_SUBITEMS 25     ///< Max number of sub items per DAQ entry.
#define DAQ_MAX_UPDATE_FREQ 20  ///< Maximum update frequency in Hz

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

//! Task function prototypes (See task.c/h)
fnTaskInit DAQ_Init;
fnTaskExec DAQ_Exec;
fnTaskShutdown DAQ_Shutdown;
fnTaskTest DAQ_Test;

bool DAQ_RegisterItem(tDAQ_Entry Entry, tDAQ_Config* Config, bool All);
bool DAQ_GetMetadata(tDAQ_Entry Entry, tDAQ_Config* Config);
bool DAQ_GetItem(tDAQ_Entry Entry, uint8_t Item, tDataValue* Value);
bool DAQ_UpdateItem(tDAQ_Entry Entry, uint8_t Item, tDataValue Value);
bool DAQ_WriteItem(tDAQ_Entry Entry, uint8_t Item, tDataValue Value);

#ifdef DAQ_PROTECTED

#endif /* DAQ_PROTECTED */

#endif /* __TASK_DAQ_H */
