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

 @file    log.c
 @brief   Implementation for supporting log messages
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#include <stdarg.h>

#include "main.h"
#include "log.h"
#include "task_serial.h"
#include "task_system.h"
#include "eeprom_mcu.h"
#include "eeprom_ext.h"
#include "timer.h"
#include "uart.h"
#include "util.h"

#ifdef TEST__ENABLE_LOGGING

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define MAX_CACHE_LOGS 16u    //!< The max cache entries. Use minimum needed. Chews up a lot of RAM for each entry.
#define MAX_EEPROM_LOGS 128u  //!< The max entries to store in external EEPROM memory. 0=disabled

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "LOG";                      //!< Name of the module. Used for debug logging
static const float _BlackoutS[eLogger_NUM] = { -1.0f };  //!< Blackout period in seconds (< 0.0f = blackout disabled)
static const uint32_t _LenMsg = MAX_DEBUGLOG_CHARS_MSG - 1;   // The max allowed chars of payload msg
static const uint32_t _LenTot = MAX_DEBUGLOG_CHARS_TOT - 1;   // The max allowed chars in a debug log line
static const uint32_t _CacheMax = MAX_CACHE_LOGS;             // The number of cache entries
static const uint32_t _ExternalMax = MAX_EEPROM_LOGS;         // The number of EEPROM log entries
static const bool _ExtEEPROMenabled = (MAX_EEPROM_LOGS > 0);  //!< logs stored in non-volatile memory when desired

// clang-format off
//! Names of the log levels
static const char _LevelName[eLogLevel_NUM][10] =
{
  "",
  "*   Error",
  "! Warning",
  "-   Debug",
  "  SystemH",
  "  SystemM",
  "  SystemL",
  "   Always"
};
// clang-format on

static bool _Enabled[eLogger_NUM] = { false };              ///< True when logging enabled
static bool _Initialized[eLogger_NUM] = { false };          ///< True when initialized
static tLogLevel _Level[eLogger_NUM] = { eLogLevel_None };  ///< Disabled by default
static tSerialPort _Port[eLogger_NUM];                      ///< The desired serial port to use
static uint32_t _Timestamp[eLogger_NUM];                    ///< Timestamp. Milli-secs since powered on
static uint32_t _Blackout[eLogger_NUM];                     ///< Blackout incoming logs for this ID
static uint32_t _Progress[eLogger_NUM];                     ///< Progress when printing progress chars
static uint32_t _ExternalLogIndex = 0;                      ///< Index for next external log entry EEPROM

static char _Cache[eLogger_NUM][MAX_CACHE_LOGS][MAX_DEBUGLOG_CHARS_TOT] = { { { '\0' } } };
static uint32_t _CacheIndex[eLogger_NUM] = { 0 };
static uint32_t _CacheCount[eLogger_NUM] = { 0 };

/* Private function prototypes -----------------------------------------------*/

static bool _WillPrint(tLogger Logger, tLogLevel Level, const char* Module, const char* Output);
static bool _Output(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, bool Direct, const char* Output);
static bool _Update(tLogger Logger, char* Output);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the serial debug output log module
 @param     Logger: The desired logger
 @param     Port: The desired serial port
 @param     Level: The desired output log level
 @param     Enabled: Enable the log when true
 @return    True if successful

 *******************************************************************/
bool LOG_Init(tLogger Logger, tSerialPort Port, tLogLevel Level, bool Enabled)
{
  _Port[Logger] = Port;
  _Enabled[Logger] = Enabled;
  _Initialized[Logger] = false;

  // Clear cache
  _CacheIndex[Logger] = 0;
  _CacheCount[Logger] = 0;
  for (uint32_t i = 0; i < MAX_CACHE_LOGS; i++)
  {
    _Cache[Logger][i][0] = '\0';
  }

  // Get the next log index from EEPROM
  EEPROM_MCU_ReadReg(eEEPROM_Reg_LogIndex, &_ExternalLogIndex);

  if (SERIAL_IsEnabled(Port))
  {
    LOG_Reset(Logger, false);

    if (Level < eLogLevel_NUM)
    {
      _Level[Logger] = Level;
    }

    _Initialized[Logger] = true;

    LOG_Write(Logger, eLogLevel_High, _Module, false, "Initialized (%u), EXT logs: %u", Logger, _ExternalLogIndex);
  }

  return _Initialized[Logger];
}

/*******************************************************************/
/*!
 @brief     Checks if a log will be printed based on the logger's level and the log's level
 @param     Logger: The desired logger

 *******************************************************************/
bool LOG_IsInit(tLogger Logger)
{
  return _Initialized[Logger];
}

/*******************************************************************/
/*!
 @brief     Writes a log to the debug serial output
 @param     Logger: The desired logger
 @param     Level: The desired level
 @param     Persist: When true, log is stored in non-volatile memory
 @param     Module: The module name issuing the log
 @param     Fmt: The log string
 @param     ...: Additional arguments for formatting the log string
 @return    True if successful

 *******************************************************************/
bool LOG_Write(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, const char* Fmt, ...)
{
  char output[MAX_DEBUGLOG_CHARS_TOT];
  bool success = false;

  if (!IsPointerValid((uintptr_t)Module))
  {
    assert_always();
  }
  else if (!IsPointerValid((uintptr_t)Fmt))
  {
    assert_always();
  }
  else
  {
    if (Level == eLogLevel_Error || Level == eLogLevel_Warning)
    {
      Persist = true;  // Always persist errors and warnings
    }

    va_list args;
    va_start(args, Fmt);
    vsnprintf(output, _LenMsg, Fmt, args);
    va_end(args);

    success = _Output(Logger, Level, Module, Persist, false, output);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Writes a log directly to the UART bypassing any task_serial queuing
 @param     Logger: The desired logger
 @param     Level: The desired level
 @param     Persist: When true, log is stored in non-volatile memory
 @param     Module: The module name issuing the log
 @param     Fmt: The log string
 @param     ...: Additional arguments for formatting the log string
 @return    True if successful

*******************************************************************/
bool LOG_WriteDirect(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, const char* Fmt, ...)
{
  char output[MAX_DEBUGLOG_CHARS_TOT];
  bool success = false;

  if (!IsPointerValid((uintptr_t)Module))
  {
    assert_always();
  }
  else if (!IsPointerValid((uintptr_t)Fmt))
  {
    assert_always();
  }
  else
  {
    va_list args;
    va_start(args, Fmt);
    vsnprintf(output, _LenMsg, Fmt, args);
    va_end(args);

    success = _Output(Logger, Level, Module, Persist, true, output);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Retrieves a log entry from the cache
 @param     Logger: The desired logger
 @param     Index: The index of the cached entry. 0 = newest entry, 1 = 2nd newest, etc.
 @param     Buffer: The buffer to store the retrieved entry
 @return    The total number of cached entries for the logger

 *******************************************************************/
uint32_t LOG_GetCachedEntry(tLogger Logger, uint32_t Index, char* Buffer)
{
  uint32_t len = _CacheCount[Logger];

  if (!IsRAM((uintptr_t)Buffer))
  {
    assert_always();
    len = 0;
  }
  else if (Index >= _CacheCount[Logger])
  {
    Buffer[0] = '\0';
    len = 0;
  }
  else
  {
    uint32_t latest = (_CacheIndex[Logger] + _CacheMax - 1u) % _CacheMax;
    uint32_t cache_idx = (latest + _CacheMax - Index) % _CacheMax;
    char* entry = _Cache[Logger][cache_idx];
    strncpy(Buffer, entry, _LenTot);
    Buffer[strlen(entry)] = '\0';
  }

  return len;
}

/*******************************************************************/
/*!
 @brief     Writes a serial string to the debug port without formatting
 @param     Logger: The desired logger
 @param     Output: The raw string
 @param     Linefeed: Append CR/LF when true
 @return    True if successful

 *******************************************************************/
bool LOG_Print(tLogger Logger, const char* Output, bool Linefeed)
{
  char output[MAX_DEBUGLOG_CHARS_TOT];
  bool success = false;

  if (Output != NULLPTR)
  {
    strcpy(output, Output);
    success = SERIAL_SendString(_Port[Logger], output);

    if (Linefeed && success)
    {
      success = SERIAL_SendString(_Port[Logger], "\r\n");
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief      Resets the log when true
 @param      Logger: The desired logger
 @return     None

 *******************************************************************/
void LOG_Reset(tLogger Logger, bool ClearEEPROM)
{
  _Level[Logger] = LOGGING_DEFAULT_LEVEL;
  _Timestamp[Logger] = 0;
  _Blackout[Logger] = 0;
  _Progress[Logger] = 0;

  if (Logger == eLogger_Sys)
  {
    if (ClearEEPROM)
    {
      _ExternalLogIndex = 0;
      EEPROM_MCU_WriteReg(eEEPROM_Reg_LogIndex, _ExternalLogIndex);
    }
  }
}

/*******************************************************************/
/*!
 @brief      Enables or disables debug logging
 @param      Logger: The desired logger
 @param      Enable: Enables the log when true
 @return     None

 *******************************************************************/
void LOG_EnableOutput(tLogger Logger, bool Enable)
{
  _Enabled[Logger] = Enable;
}

/*******************************************************************/
/*!
 @brief      Returns enabled status
 @param      Logger: The desired logger
 @return     True when the log is enabled
 *******************************************************************/
bool LOG_IsOutputEnabled(tLogger Logger)
{
  return _Enabled[Logger];
}

/*******************************************************************/
/*!
 @brief     Sets the logging level
 @param     Logger: The desired logger
 @param     Level: The desired level


 *******************************************************************/
void LOG_SetLevel(tLogger Logger, tLogLevel Level)
{
  if (Level < eLogLevel_NUM)
  {
    _Level[Logger] = Level;
  }
}

/*******************************************************************/
/*!
 @brief     Prints a character to the log (without CR/LF)
 @param     Logger: The desired logger
 @param     Level: The desired log level
 @param     Module: The module name
 @param     Dot: The desired character
 @return    True if successful

 *******************************************************************/
bool LOG_Progress(tLogger Logger, tLogLevel Level, const char* Module, char Dot)
{
  bool success = false;

  if (!_WillPrint(Logger, Level, Module, ""))
  {
    // Nothing will print
  }
  else if (_Progress[Logger] > 0)
  {
    // Append progress char
    success = SERIAL_SendByte(_Port[Logger], Dot) > 0;
    _Progress[Logger]++;
  }
  else
  {
    // Start a new line
    success = _Output(Logger, Level, Module, false, false, "Starting progress bar: ");
    _Progress[Logger]++;
  }

  return success;
}

/* Private Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Writes a log to the debug serial output
 @param     Logger: The desired logger
 @param     Level: The desired level
 @param     Module: The module name issuing the log
 @param     Persist: When true, log is stored in non-volatile memory
 @param     Direct: When true, the Output is sent directly to the UART without queuing
 @param     Output: The log string
 @param     Analysis: Bare output when true
 @return    True if output is sent

 *******************************************************************/
static bool _Output(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, bool Direct, const char* Output)
{
  char output[MAX_DEBUGLOG_CHARS_TOT];
  float uptime_s;
  bool proceed = false;

  if (_WillPrint(Logger, Level, Module, Output))
  {
    // Determine if we want to print the output based on id and timeout
    if (TIMER_GetElapsed_s(_Timestamp[Logger]) > _BlackoutS[Logger])
    {
      // Blackout expired. Print and reset timer
      strcat(output, " ***");
      _Timestamp[Logger] = TIMER_GetTick();
      proceed = true;
    }
    else
    {
      // Blackout still in effect
    }

    if (proceed)
    {
      // Prepend some useful stuff like time and tag
      uptime_s = SYSTEM_GetUpTime_MS() / 1000.0f;
      char p = Persist ? '#' : ' ';
      snprintf(output, _LenTot, "[%10.3f] %c%-4s : %6s : %s", uptime_s, p, _LevelName[Level], Module, Output);

      if (_Progress[Logger] > 0)
      {
        // Send a CR if we were previously sending progress dots
        SERIAL_SendByte(_Port[Logger], '\r');

        // Reset progress count so that any progress logging starts a new line
        _Progress[Logger] = 0;
      }

      if (Persist)
      {
        _Update(Logger, output);
      }

      strcat(output, "\r\n");
      if (!Direct)
      {
        // Send output to debug serial port
        if (!SERIAL_SendString(_Port[Logger], output))
        {
          // Some chars were not sent. Overflow?
          assert_always();
        }
      }
      else
      {
        // Send directly to the UART without queuing in the serial task
        if (!UART_SendString(_Port[Logger], output, true))
        {
          // Some chars were not sent. Overflow?
          assert_always();
        }
      }
    }
  }

  return proceed;
}

/*******************************************************************/
/*!
 @brief     Analysis whether output will be output given level and state
 @param     Logger: The desired logger
 @param     Level: The desired output
 @param     Module: The module name
 @param     Output: The output text
 @return    True when output allowed

 *******************************************************************/
static bool _WillPrint(tLogger Logger, tLogLevel Level, const char* Module, const char* Output)
{
  bool print = false;

  if (!IsPointerValid((uintptr_t)Module))
  {
    assert_always();
  }
  else if (!IsPointerValid((uintptr_t)Output))
  {
    assert_always();
  }
  else if (!SERIAL_IsEnabled(_Port[Logger]))
  {
  }
  else if (!_Enabled[Logger])
  {
  }
  else if (!_Initialized[Logger])
  {
  }
  else if (Level == eLogLevel_None)
  {
  }
  else if (Level == eLogLevel_Always)
  {
    // Always print at this level
    print = true;
  }
  else if (_Level[Logger] == eLogLevel_None)
  {
    // No output, ever
  }
  else if (Level > _Level[Logger])
  {
    // Level not high enough
  }
  else
  {
    // Everything looks good!
    print = true;
  }

  return print;
}

/*******************************************************************/
/*!
 @brief     Updates the log cache and optionally stores the log in non-volatile memory
 @param     Logger: The desired logger
 @param     Output: The output text
 @return    True if eeprom (not just the cache) was updated

 *******************************************************************/
static bool _Update(tLogger Logger, char* Output)
{
  bool updated = false;

  // Store the log in the circular cache, overwriting oldest entry if full
  strncpy(_Cache[Logger][_CacheIndex[Logger]], Output, _LenTot);
  _Cache[Logger][_CacheIndex[Logger]][strlen(Output)] = '\0';
  _CacheIndex[Logger] = (_CacheIndex[Logger] + 1) % _CacheMax;
  if (_CacheCount[Logger] < _CacheMax)
  {
    _CacheCount[Logger]++;
  }

  if (_ExtEEPROMenabled)
  {
    // Store the log in EXT EEPROM memory
    EEPROM_EXT_WriteLog(_ExternalLogIndex, (uint8_t*)Output);

    // Increment EXT EEPROM log index in MCU EEPROM
    _ExternalLogIndex = (_ExternalLogIndex + 1) % _ExternalMax;
    updated = EEPROM_MCU_WriteReg(eEEPROM_Reg_LogIndex, _ExternalLogIndex);
  }

  return updated;
}

/*******************************************************************/
// Stubs for when debug output is not enabled
/*******************************************************************/

#else

bool LOG_Init(tLogger Logger, tSerialPort Port, tLogLevel Level, bool Enabled)
{
  return true;
}

bool LOG_IsInit(tLogger Logger)
{
  return true;
}

void LOG_Reset(tLogger Logger, bool ClearEEPROM)
{
}

void LOG_EnableOutput(tLogger Logger, bool Enable)
{
}

bool LOG_IsOutputEnabled(tLogger Logger)
{
  return false;
}

void LOG_SetLevel(tLogger Logger, tLogLevel Level)
{
}

bool LOG_Write(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, const char* Fmt, ...)
{
  return true;
}

bool LOG_WriteDirect(tLogger Logger, tLogLevel Level, const char* Module, bool Persist, const char* Fmt, ...)
{
  return true;
}

uint32_t LOG_GetCachedEntry(tLogger Logger, uint32_t Index, char* Buffer)
{
  return 0;
}

bool LOG_Print(tLogger Logger, const char* Output, bool Linefeed)
{
  return false;
}

void LOG_UseLineNumbers(tLogger Logger, bool Use)
{
}

bool LOG_Progress(tLogger Logger, tLogLevel Level, const char* Module, char Dot)
{
  return true;
}

#endif /* TEST__ENABLE_LOGGING */
