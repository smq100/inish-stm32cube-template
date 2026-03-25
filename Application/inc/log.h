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

 @file    log.h
 @brief   Interface for supporting log messages
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __LOG_H
#define __LOG_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  eLogger_Sys,
  eLogger_Tech,
  eLogger_NUM
} tLogger;

//! Allowable levels of the debug output log
typedef enum
{
  eLogLevel_None,     //!< Never print output
  eLogLevel_Error,    //!< For errors and unusual conditions. Highest
  eLogLevel_Warning,  //!< For warnings
  eLogLevel_Debug,    //!< For debugging
  eLogLevel_High,     //!< Important system messages
  eLogLevel_Med,      //!< Med system messages
  eLogLevel_Low,      //!< Lowest level system messages
  eLogLevel_Always,   //!< Always print output. Must be last level
  eLogLevel_NUM
} tLogLevel;

/* Exported constants --------------------------------------------------------*/

#define LOGGING_DEFAULT_LEVEL eLogLevel_Med  //! Default level of output log
#define MAX_DEBUGLOG_CHARS_MSG 64u   //!< The max allowed chars in a debug message (excludes module name, level, etc.)
#define MAX_DEBUGLOG_CHARS_TOT 128u  //!< The total max allowed chars (includes module name, level, etc.)
#define CACHE_ENTRIES_MAX 16u     //!< The max cache entries. Use minimum needed. Chews up a lot of RAM for each entry.
#define PERSIST_ENTRIES_MAX 128u  //!< The max entries to store in non-volatile memory. 0=disabled

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

bool LOG_Init(tLogger Logger, tSerialPort Port, tLogLevel Level, bool Enabled);
bool LOG_IsInit(tLogger Logger);
bool LOG_Write(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, const char* Fmt, ...);
uint32_t LOG_GetCachedEntry(tLogger Logger, uint32_t Index, char* Buffer);
bool LOG_Print(tLogger Logger, const char* Output, bool Linefeed);
void LOG_Reset(tLogger Logger);
void LOG_EnableOutput(tLogger Logger, bool Enable);
bool LOG_IsOutputEnabled(tLogger Logger);
void LOG_SetLevel(tLogger Logger, tLogLevel Level);
bool LOG_Progress(tLogger Logger, tLogLevel Level, const char* Module, char Dot);

#endif /* __LOG_H */
