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
*****************************************************************************

 @file    task_tech.c
 @brief   Implementation for the serial Tech Port
 @author  Steve Quinlan
 @date    2026-March

 @note    See TECHMODE.md for a complete description of the technician interface

******************************************************************************/
#define TASK_TECH_PROTECTED
#define SYSTEM_PROTECTED
#define CLASSB_PROTECTED

#include <ctype.h>

#include "main.h"
#include "classb.h"
#include "version.h"
#include "task.h"
#include "task_tech.h"
#include "task_daq.h"
#include "task_system.h"
#include "classb_runtime.h"
#include "eeprom_mcu.h"
#include "queue.h"
#include "timer.h"
#include "log.h"
#include "util.h"

/* Private define ------------------------------------------------------------*/

#define COMMAND_BUFFER_SIZE_RX 256
#define CRC_BUFFER_SIZE_RX 5  ///< 4 chars for CRC plus the CRC delimiter
#define MAX_STREAM_ITEMS 5    ///< Max number of streaming data values
#define MAX_TOKENS eDAQ_Entry_NUM
#define MAX_CMD_SIZE 8
#define MAX_NOTE_SIZE 10

/* Private typedef -----------------------------------------------------------*/

//! Tech commands
typedef enum
{
  eTechCommand_Help,          ///< Print help screen
  eTechCommand_Auth,          ///< Authenticate user
  eTechCommand_System,        ///< Provide system data
  eTechCommand_Reboot,        ///< Reboot the system
  eTechCommand_FactoryReset,  ///< Reset all settings to factory defaults
  eTechCommand_Meta,          ///< Provide DAQ metadata
  eTechCommand_Tasks,         ///< Provide tasks data
  eTechCommand_Get,           ///< Provide DAQ data
  eTechCommand_Set,           ///< Modify DAQ data
  eTechCommand_ReadMemory,    ///< Read memory data
  eTechCommand_GetLog,        ///< Retrieve log data
  eTechCommand_StreamItems,   ///< Specifiy items needed for the stream
  eTechCommand_StreamStart,   ///< Set up streaming output
  eTechCommand_StreamStop,    ///< Stop a running stream
  eTechCommand_Fault,         ///< Inject fault
  NUM_TECH_COMMANDS
} tTechCommands;

//! Tech output state machine
typedef enum
{
  eTechState_Init,       //!< Init state
  eTechState_Idle,       //!< Wait state
  eTechState_Fetch,      //!< Fetch state
  eTechState_CRC,        //!< CRC state
  eTechState_Parse,      //!< Parse state
  eTechState_Run,        //!< Run state
  eTechState_Completed,  //!< Completed state
  eTechState_Wait,       //!< Wait state
  eTechState_Reset,      //!< Reset state
  eTechState_Error,      //!< Error state
  NUM_TECH_STATES
} tTechState;

//! Tech error codes
typedef enum
{
  eTechError_None,
  eTechError_BufferOverflow,
  eTechError_Timeout,
  eTechError_Unknown,
  eTechError_Disabled,
  eTechError_Processing,
  eTechError_Streaming,
  eTechError_Auth,
  eTechError_CRC,
  NUM_TECH_ERRORS
} tTechError;

//! Command handler function prototype
typedef bool(fnHandler)(char*);

//! Command handler
typedef struct
{
  bool Enabled;        //!< Enabled when true
  bool Streamable;     //!< True if streamable
  bool AuthReq;        //!< True if command requires authentication (and auth is enabled)
  char* Name;          //!< Name of the command
  fnHandler* Handler;  //!< Command handler
} tConfig;

typedef struct
{
  bool Active;                          //!< True when streaming is active
  char Command[MAX_CMD_SIZE];           //!< The command (must be a valid command in _Config[].Name)
  int32_t Items[MAX_STREAM_ITEMS];      //!< The DAQ items to be streamed
  int32_t SubItems[MAX_STREAM_ITEMS];   //!< The DAQ subitems to be streamed
  char Header[COMMAND_BUFFER_SIZE_RX];  //!< A string that may be used as a header for the data
  fnHandler* Handler;                   //!< Command handler
  uint32_t Timestamp;                   //!< Timestamp in milliseconds
  uint32_t Interval;                    //!< Interval in milliseconds
  uint32_t Time;                        //!< System timestamp for timing interval
} tRuntimeStream;

typedef struct
{
  bool Initialized;             //!< True when initialized
  bool Authenticated;           //!< True when user has authenticated (if login is required)
  bool Boot;                    //!< True when booting
  tTechState State;             //!< Command state
  tTechCommands ActiveCommand;  //!< The active tech command
  tTechError ErrorCode;         //!< Error code
  tRuntimeStream Stream;        //!< True when streaming is active
} tRuntime;

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

// State machine helpers
static bool _Process(void);
static void _Booting(void);
static bool _Parse(void);
static void _Tokenize(void);
static void _Reset(void);

// Stream processing
static bool _ProcessStream(void);
static void _ResetStream(bool Hard);

// Command handlers
static const fnHandler _Handler_Help;
static const fnHandler _Handler_Auth;
static const fnHandler _Handler_System;
static const fnHandler _Handler_Reboot;
static const fnHandler _Handler_Reset;
static const fnHandler _Handler_Meta;
static const fnHandler _Handler_Tasks;
static const fnHandler _Handler_Get;
static const fnHandler _Handler_Set;
static const fnHandler _Handler_ReadMemory;
static const fnHandler _Handler_GetLog;
static const fnHandler _Handler_StreamStart;
static const fnHandler _Handler_StreamItems;
static const fnHandler _Handler_StreamStop;
static const fnHandler _Handler_Fault;

// Misc
static fnHandler* _GetHandlerFromCommand(const char* Cmd);
static void _WriteMessage(const char* Message, bool InsertCR);

/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_TECH";                       ///< Module name for debug logging
static const uint8_t _Version = 1;                           ///< Tech protocol version number
static const tSerialPort _Port = eSerial_Tech;               ///< The serial port to use
static const uint8_t _StartChar = '*';                       ///< Command start delimiter
static const uint8_t _EndChar = ';';                         ///< Command end delimiter
static const uint8_t _CRCChar = '#';                         ///< Command CRC delimiter
static const uint32_t _Timeout_ms = 500;                     ///< Command processing timeout in milliseconds
static const uint32_t _NumDAQEntries = eDAQ_Entry_NUM;       ///< Number of available DAQ items
static const uint32_t _MaxStreamItems = MAX_STREAM_ITEMS;    ///< Max number of streamable parameters (DAQ items)
static const int32_t _StreamMinFreq = 1;                     ///< Min stream data delivery frequency
static const int32_t _StreamMaxFreq = DAQ_MAX_UPDATE_FREQ;   ///< Max stream data delivery frequency
static const char* _StreamCmdToken = "stream";               ///< Token used in streaming JSON output
static const uint32_t _MaxTokens = MAX_TOKENS;               ///< Max number of parsed tokens
static const char* _Meta = "\"meta\"";                       ///< 'meta' keyword for JSON output
static const bool _DisableSysLogging = true;                 ///< Useful if logging and tech share the same serial port)
static const bool _UseCRC = true;                            ///< Enable CRC error checking on the input and output
static const bool _AuthRequired = TECH_AUTH_REQUIRED;        ///< Require login command before processing other commands
static const char* _AuthUser = TECH_AUTH_USER;               ///< The username required for login (if login is enabled)
static const char* _AuthPassword = TECH_AUTH_PASSWORD;       ///< The password required for login (if login is enabled)
static const uint8_t _AuthUserPWSize = TECH_AUTH_USERPW_SZ;  ///< Max size of the user and pwd
static const uint32_t _MaxResponseSize = TECH_RESPONSE_MAXSIZE - 1;  ///< Size of the response

// clang-format off
//! Tech commands and associated handlers
static const tConfig _Config[] = {
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "help", .Handler = _Handler_Help },
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "auth", .Handler = _Handler_Auth },
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "sys", .Handler = _Handler_System },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "reboot", .Handler = _Handler_Reboot },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "reset", .Handler = _Handler_Reset },
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "meta", .Handler = _Handler_Meta },
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "tasks", .Handler = _Handler_Tasks },
  { .Enabled = true, .AuthReq = false, .Streamable = true, .Name = "get", .Handler = _Handler_Get },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "set", .Handler = _Handler_Set },
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "readmem", .Handler = _Handler_ReadMemory },
  { .Enabled = true, .AuthReq = false, .Streamable = false, .Name = "log", .Handler = _Handler_GetLog },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "sitems", .Handler = _Handler_StreamItems },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "sstart", .Handler = _Handler_StreamStart },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "sstop", .Handler = _Handler_StreamStop },
  { .Enabled = true, .AuthReq = true, .Streamable = false, .Name = "fault", .Handler = _Handler_Fault },
};

static_assert(NUM_TECH_COMMANDS == (sizeof(_Config) / sizeof(tConfig)), "TECH command config size mismatch");

static const char* _ErrorStr[] = {
  "success",
  "overflow",
  "timeout",
  "unknown",
  "disabled",
  "handler",
  "stream",
  "auth",
  "crc"
};

static_assert(NUM_TECH_ERRORS == (sizeof(_ErrorStr) / sizeof(_ErrorStr[0])), "TECH error string size mismatch");
// clang-format on

static tRuntime _Runtime;  //!< Runtime data

static uint8_t _ParseBuffer[COMMAND_BUFFER_SIZE_RX];  //!< The Rx buffer for parsing
static char _ResponseBuffer[TECH_RESPONSE_MAXSIZE];   //!< The message buffer

static uint8_t _QueueBufferRx[COMMAND_BUFFER_SIZE_RX];  //!< The Rx queue buffer
static tQueue _QueueRx;                                 //!< Queue of Rx buffer

static uint8_t _QueueBufferCRC[CRC_BUFFER_SIZE_RX];  //!< The CRC queue buffer
static tQueue _QueueCRC;                             //!< Queue of CRC buffer

static char* _Tokens[MAX_TOKENS];  //!< Parsed command tokens

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the TECH interface
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TECH_Init(void)
{
  _Runtime.Boot = true;
  _Runtime.State = eTechState_Init;
  _Runtime.ActiveCommand = eTechCommand_Help;
  _Runtime.ErrorCode = eTechError_None;
  _Runtime.Authenticated = !_AuthRequired;  // If login is not required, consider authenticated from the start

  _ResetStream(true);

  for (uint32_t i = 0; i < _MaxStreamItems; i++)
  {
    _Runtime.Stream.Items[i] = -1;
    _Runtime.Stream.SubItems[i] = -1;
  }

  // Create queues to hold the Rx and CRC chars
  bool init = QUEUE_Init(&_QueueRx, _QueueBufferRx, sizeof(uint8_t), COMMAND_BUFFER_SIZE_RX, "TechRx");
  assert(init);
  init = QUEUE_Init(&_QueueCRC, _QueueBufferCRC, sizeof(uint8_t), CRC_BUFFER_SIZE_RX + 1, "TechCRC");
  assert(init);

  _Runtime.Initialized = true;

  if (_Runtime.Initialized)
  {
    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized");
    LOG_Write(eLogger_Tech,
              eLogLevel_High,
              _Module,
              false,
              "Initialized. Auth %s",
              _Runtime.Authenticated ? "not required" : "required");
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Not Initialized");
  }

  return _Runtime.Initialized;
}

/*******************************************************************/
/*!
 @brief     Executive loop of the TECH module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TECH_Exec(void)
{
  if (!_Runtime.Boot)
  {
    _Process();
  }
  else
  {
    _Booting();
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing for the TECH module
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TECH_Shutdown(void)
{
  // No shutdown processing required
  return true;
}

/*******************************************************************/
/*!
 @brief     POST routine for TECH module
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool TECH_Test(void)
{
  return true;
}

/* Private Implementation ----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Primary state machine
 @return    True if successful
 *******************************************************************/
static bool _Process(void)
{
  static uint32_t time = 0;
  uint8_t c;
  char response[TECH_RESPONSE_MAXSIZE];

  if (_Runtime.State == eTechState_Init)
  {
    _Reset();

    _Runtime.State = eTechState_Idle;
    _Runtime.ErrorCode = eTechError_None;
    time = TIMER_GetTick();
  }
  else if (_Runtime.State == eTechState_Idle)
  {
    if (SERIAL_ReceiveByte(_Port, &c) > 0)
    {
      if (c == _StartChar)
      {
        // Valid start char; add it to the queue for processing and move to fetch state
        QUEUE_Enque(&_QueueRx, &c);

        _Runtime.State = eTechState_Fetch;
        time = TIMER_GetTick();
      }
    }
  }
  else if (_Runtime.State == eTechState_Fetch)
  {
    if (SERIAL_ReceiveByte(_Port, &c) > 0)
    {
      if (QUEUE_GetCount(&_QueueRx) < COMMAND_BUFFER_SIZE_RX)
      {
        QUEUE_Enque(&_QueueRx, &c);

        if (c == _EndChar)
        {
          // Valid end char; move to CRC state if enabled, otherwise move to parse state
          if (_UseCRC)
          {
            QUEUE_Flush(&_QueueCRC);
            _Runtime.State = eTechState_CRC;
          }
          else
          {
            _Runtime.State = eTechState_Parse;
          }
          time = TIMER_GetTick();
        }
        else if (c == _StartChar)
        {
          // Start char received again before end char; reset command
          _Runtime.State = eTechState_Wait;
          time = TIMER_GetTick();
        }
        else if (!isprint(c))
        {
          // Bogus char; remove it
          QUEUE_Deque(&_QueueRx, &c);
        }
      }
      else
      {
        // Buffer overflow; reset and report error
        _Runtime.State = eTechState_Error;
        _Runtime.ErrorCode = eTechError_BufferOverflow;
        time = TIMER_GetTick();
      }
    }
    else if (TIMER_GetElapsed_ms(time) > _Timeout_ms)
    {
      // Timeout while waiting for command; reset and report error
      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Timeout;
      time = TIMER_GetTick();
    }
  }
  else if (_Runtime.State == eTechState_CRC)
  {
    if (SERIAL_ReceiveByte(_Port, &c) > 0)
    {
      if (QUEUE_GetCount(&_QueueCRC) == 0)
      {
        // First CRC char should be the CRC delimiter
        if (c == _CRCChar)
        {
          // Valid CRC delimiter; move to next state to gather CRC chars
          QUEUE_Enque(&_QueueCRC, &c);
        }
        else
        {
          // Invalid CRC start char; reset and report error
          _Runtime.State = eTechState_Error;
          _Runtime.ErrorCode = eTechError_CRC;
          time = TIMER_GetTick();
        }
      }
      else if (QUEUE_GetCount(&_QueueCRC) < CRC_BUFFER_SIZE_RX)
      {
        // Gather CRC chars until we have the full CRC string (4 bytes + the CRC delimiter)
        if (isxdigit(c))
        {
          QUEUE_Enque(&_QueueCRC, &c);
        }
        else
        {
          // Invalid CRC char; reset and report error
          _Runtime.State = eTechState_Error;
          _Runtime.ErrorCode = eTechError_CRC;
          time = TIMER_GetTick();
        }
      }
    }
    else if (QUEUE_GetCount(&_QueueCRC) == CRC_BUFFER_SIZE_RX)
    {
      // We have the full CRC string; calculate CRC and compare to received CRC
      char crcstr[CRC_BUFFER_SIZE_RX + 1] = { 0 };

      QUEUE_Deque(&_QueueCRC, &c);  // Remove the CRC delimiter before processing the CRC value
      for (int i = 0; i < CRC_BUFFER_SIZE_RX - 1; i++)
      {
        QUEUE_Deque(&_QueueCRC, &c);
        crcstr[i] = c;
      }

      uint8_t* qptr = (uint8_t*)(_QueueRx.QueuePtr);
      uint16_t rx_crc = (uint16_t)strtoul(crcstr, NULLPTR, 16);
      uint16_t calc_crc = Calculate_CRC16(qptr, CRC_BUFFER_SIZE_RX - 1);

      LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "Rx CRC: 0x%04X, Calc CRC: 0x%04X", rx_crc, calc_crc);

      if (rx_crc == calc_crc)
      {
        // CRC valid; move to parse state
        QUEUE_Flush(&_QueueCRC);
        _Runtime.State = eTechState_Parse;
        time = TIMER_GetTick();
      }
      else if (rx_crc == 0)
      {
        // Backdoor bypass (CRC=0x0000) for CRC check to allow use without valid CRC; move to parse state
        QUEUE_Flush(&_QueueCRC);
        _Runtime.State = eTechState_Parse;
        time = TIMER_GetTick();
      }
      else
      {
        // CRC mismatch; reset and report error
        QUEUE_Flush(&_QueueCRC);
        _Runtime.State = eTechState_Error;
        _Runtime.ErrorCode = eTechError_CRC;
        time = TIMER_GetTick();

        LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, true, "CRC error: rx=0x%04X, calc=0x%04X", rx_crc, calc_crc);
      }
    }
    else if (TIMER_GetElapsed_ms(time) > _Timeout_ms)
    {
      // Timeout while waiting for CRC chars; reset and report error
      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Timeout;
      time = TIMER_GetTick();
    }
  }
  else if (_Runtime.State == eTechState_Parse)
  {
    if (_Parse())
    {
      // Parsing successful; move to run state
      _Runtime.State = eTechState_Run;
    }
    else
    {
      // Parsing error; reset and report error
      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Unknown;
    }

    time = TIMER_GetTick();
  }
  else if (_Runtime.State == eTechState_Run)
  {
    if (!_Config[_Runtime.ActiveCommand].Enabled)
    {
      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Disabled;
    }
    else if (_Config[_Runtime.ActiveCommand].AuthReq && !_Runtime.Authenticated)
    {
      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Auth;
    }
    else if (_Config[_Runtime.ActiveCommand].Handler(NULLPTR))  // Run the handler associated with the command
    {
      // Handler returned success
      _Runtime.State = eTechState_Completed;
    }
    else
    {
      // Handler returned an error
      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Processing;
    }

    time = TIMER_GetTick();
  }
  else if (_Runtime.State == eTechState_Completed)
  {
    if (strlen(_ResponseBuffer) > 0)
    {
      snprintf(response,
               _MaxResponseSize,
               "{\"code\":%d,\"status\":\"%s\",\"cmd\":\"%s\",\"data\":{%s}}",
               _Runtime.ErrorCode,
               _ErrorStr[_Runtime.ErrorCode],
               _Config[_Runtime.ActiveCommand].Name,
               _ResponseBuffer);
      _ResponseBuffer[0] = '\0';
    }
    else
    {
      snprintf(response,
               _MaxResponseSize,
               "{\"code\":%d,\"status\":\"%s\",\"cmd\":\"%s\"}",
               _Runtime.ErrorCode,
               _ErrorStr[_Runtime.ErrorCode],
               _Config[_Runtime.ActiveCommand].Name);
    }

    if (strlen(response) > 0)
    {
      if (_UseCRC)
      {
        // Calculate the CRC and append to the end of the message before sending
        uint16_t crc = Calculate_CRC16(response, strlen(response));
        char crcstr[6];
        snprintf(crcstr, 6, "%c%04X", _CRCChar, crc);
        strcat(response, crcstr);
      }

      _WriteMessage(response, true);
    }

    // Re-enable log output
    LOG_EnableOutput(eLogger_Tech, true);
    if (_DisableSysLogging)
    {
      LOG_EnableOutput(eLogger_Sys, true);
    }

    _Runtime.State = eTechState_Init;
  }
  else if (_Runtime.State == eTechState_Wait)
  {
    // Wait for the streaming data to complete sending before resetting
    if (SERIAL_GetTxCount(_Port) == 0)
    {
      _Runtime.State = eTechState_Reset;
    }
  }
  else if (_Runtime.State == eTechState_Reset)
  {
    _Reset();

    // Re-queue the start delimiter that was read to get to this state
    QUEUE_Enque(&_QueueRx, &_StartChar);

    _Runtime.State = eTechState_Fetch;
    time = TIMER_GetTick();
  }
  else if (_Runtime.State == eTechState_Error)
  {
    snprintf(
      response, _MaxResponseSize, "{\"code\":%d,\"error\":\"%s\"}", _Runtime.ErrorCode, _ErrorStr[_Runtime.ErrorCode]);

    if (_UseCRC)
    {
      // Calculate the CRC and append to the end of the message before sending
      uint16_t crc = Calculate_CRC16(response, strlen(response));
      char crcstr[6];
      snprintf(crcstr, 6, "%c%04X", _CRCChar, crc);
      strcat(response, crcstr);
    }

    _WriteMessage(response, true);

    // Re-enable log output
    LOG_EnableOutput(eLogger_Tech, true);
    if (_DisableSysLogging)
    {
      LOG_EnableOutput(eLogger_Sys, true);
    }

    _Runtime.State = eTechState_Init;
    time = TIMER_GetTick();
  }
  else
  {
    // Should never be here
    assert_always();

    _Runtime.State = eTechState_Init;
  }

  // Process any active stream
  if (_Runtime.Stream.Active)
  {
    if (!_ProcessStream())
    {
      _ResetStream(true);

      _Runtime.State = eTechState_Error;
      _Runtime.ErrorCode = eTechError_Streaming;
    }
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Process while booting
 *******************************************************************/
static void _Booting(void)
{
  // Simple booting process that just waits a short time before normal processing
  if (SYSTEM_GetUpTime_MS() > 1000)
  {
    _Runtime.Boot = false;
  }
}

/*******************************************************************/
/*!
 @brief     Parse command buffer
 @return    True if successful
 *******************************************************************/
static bool _Parse(void)
{
  bool ret = false;
  uint32_t i = 0;
  uint8_t c;

  while (QUEUE_Deque(&_QueueRx, &c))
  {
    _ParseBuffer[i++] = c;
  }

  // Break up command into its parts
  _Tokenize();

  for (int i = 0; i < NUM_TECH_COMMANDS; i++)
  {
    if (strncasecmp(_Tokens[0], _Config[i].Name, 12) == 0)
    {
      _Runtime.ActiveCommand = (tTechCommands)i;
      ret = true;
      break;
    }
  }

  return ret;
}

/*******************************************************************/
/*!
 @brief     Tokenize command buffer
 *******************************************************************/
static void _Tokenize(void)
{
  uint32_t i = 1;  // Skip the '*' char
  uint32_t t = 0;

  while (_ParseBuffer[i])
  {
    // Go to next non-ws char
    while (isspace(_ParseBuffer[i++]))
      ;

    // Mark the location of token
    i--;
    _Tokens[t++] = (char*)&(_ParseBuffer[i++]);

    // Go to end of token
    while (!isspace(_ParseBuffer[i]))
    {
      if (_ParseBuffer[i])
      {
        i++;
      }
      else
      {
        break;
      }
    }

    // Terminate token
    _ParseBuffer[i++] = '\0';
  }

  // Erase endchar
  i -= 2;
  _ParseBuffer[i] = '\0';
}

/*******************************************************************/
/*!
 @brief     Reset the command interface
 *******************************************************************/
static void _Reset(void)
{
  _ResponseBuffer[0] = '\0';

  for (uint32_t i = 0; i < COMMAND_BUFFER_SIZE_RX; i++)
  {
    _ParseBuffer[i] = 0;
  }

  for (uint32_t i = 0; i < _MaxTokens; i++)
  {
    _Tokens[i] = NULLPTR;
  }

  QUEUE_Flush(&_QueueRx);
  QUEUE_Flush(&_QueueCRC);
}

/*******************************************************************/
/*!
 @brief     Process an active stream
 @return    True if no errors
 *******************************************************************/
static bool _ProcessStream(void)
{
  bool success = true;
  uint32_t time;
  char response[TECH_RESPONSE_MAXSIZE];

  time = TIMER_GetElapsed_ms(_Runtime.Stream.Time);
  if (time < _Runtime.Stream.Interval)
  {
    // Not time for next stream update yet
  }
  else
  {
    _Runtime.Stream.Timestamp += time;

    if (_Runtime.Stream.Handler != NULLPTR)
    {
      for (uint32_t i = 0; i < _MaxStreamItems; i++)
      {
        int32_t index = _Runtime.Stream.Items[i];
        if (index >= 0)
        {
          // Run the handler associated with the stream (always 'get' in our case)
          snprintf(response, _MaxResponseSize, "%li,%li", _Runtime.Stream.Items[i], _Runtime.Stream.SubItems[i]);
          _Runtime.Stream.Handler(response);

          if (strlen(_ResponseBuffer) > 0)
          {
            snprintf(response,
                     _MaxResponseSize,
                     "{\"cmd\":\"%s\",\"stype\":\"get\",\"ts\":%lu,\"data\":{%s}}",
                     _StreamCmdToken,
                     _Runtime.Stream.Timestamp,
                     _ResponseBuffer);

            _ResponseBuffer[0] = '\0';
          }
          else
          {
            snprintf(response,
                     _MaxResponseSize,
                     "{\"cmd\":\"%s\",\"stype\":\"get\",\"ts\":%lu}",
                     _StreamCmdToken,
                     _Runtime.Stream.Timestamp);
          }

          if (_UseCRC)
          {
            // Calculate the CRC and append to the end of the message before sending
            uint16_t crc = Calculate_CRC16(response, strlen(response));
            char crcstr[6];
            snprintf(crcstr, 6, "%c%04X", _CRCChar, crc);
            strcat(response, crcstr);
          }

          _WriteMessage(response, true);

          _Runtime.Stream.Time = TIMER_GetTick();
        }
        else
        {
          break;
        }
      }
    }
    else
    {
      // Should never be here
      assert_always();
      success = false;
    }

    if (success)
    {
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Reset an active stream
 @param     Hard: Hard reset when true
 *******************************************************************/
static void _ResetStream(bool Hard)
{
  _Runtime.Stream.Active = false;
  _ResponseBuffer[0] = '\0';

  if (Hard)
  {
    _Runtime.Stream.Command[0] = '\0';
    _Runtime.Stream.Header[0] = '\0';
    _Runtime.Stream.Handler = NULLPTR;
    _Runtime.Stream.Timestamp = 0;
    _Runtime.Stream.Interval = 100;
    _Runtime.Stream.Time = 0;

    for (uint32_t i = 0; i < _MaxStreamItems; i++)
    {
      _Runtime.Stream.Items[i] = -1;
      _Runtime.Stream.SubItems[i] = -1;
    }
  }
}

/******************************************************************************
 * Command handlers
 ******************************************************************************/

/*******************************************************************/
/*!
 @brief     Command handler for 'help' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful

 *******************************************************************/
static bool _Handler_Help(char* StreamTokens)
{
  _WriteMessage("\n\n", false);
  _WriteMessage("Welcome to the command and data acquisition interface\n", true);
  _WriteMessage("The following commands are available:", true);
  _WriteMessage("help          Displays this help message", true);
  _WriteMessage("auth user,pw  Authenticates the user", true);
  _WriteMessage("meta (n)      Returns a DAQ item metadata. n=DAQ index, or interface metadata if none", true);
  _WriteMessage("reboot (n)    Reboots the system. n=optional delay in ms", true);
  _WriteMessage("reset         Resets all settings to factory defaults (system will reboot)", true);
  _WriteMessage("meta (n)      Returns a DAQ item metadata. n=DAQ index, or interface metadata if none", true);
  _WriteMessage("tasks (n)     Returns task information (n=task, otherwise all tasks, -1 for idle metrics)", true);
  _WriteMessage("get n(,m)     Returns a DAQ value. n=item, m=optional subitems (no spaces between items)", true);
  _WriteMessage("set n,v       Set a DAQ value. n=DAQ item, v=numeric value (not all items modifiable)", true);
  _WriteMessage("log n         Returns log information. n=log index", true);
  _WriteMessage("sitems n[m]   Specify items for DAQ streaming", true);
  _WriteMessage("sstart get,f  Start DAQ streaming. 'get'=cmd (only 'get' is supported), f=frequency in Hz", true);
  _WriteMessage("sstop         Stop DAQ streaming", true);
  _WriteMessage("fault n       Simulate a ClassB fault condition. n=fault code", true);

  return true;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'auth' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful

 *******************************************************************/
static bool _Handler_Auth(char* StreamTokens)
{
  bool success = false;
  char user[TECH_AUTH_USERPW_SZ];
  char pw[TECH_AUTH_USERPW_SZ];

  // Expecting "username,pw"
  int items = sscanf(_Tokens[1], "%7[^,],%7s", user, pw);

  if (items != 2)
  {
    // Invalid format
  }
  else if (strncmp(user, _AuthUser, _AuthUserPWSize - 1) != 0)
  {
    // Authentication failed - username
    _Runtime.Authenticated = false;
  }
  else if (strncmp(pw, _AuthPassword, _AuthUserPWSize - 1) != 0)
  {
    // Authentication failed - password
    _Runtime.Authenticated = false;
  }
  else
  {
    // Authentication successful
    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, true, "Technician mode authenticated");
    _Runtime.Authenticated = true;
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'sys' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_System(char* StreamTokens)
{
  bool success = true;
  char output[TECH_RESPONSE_MAXSIZE];

  uint32_t hw = SYSTEM_GetHWVersion();

  snprintf(output, _MaxResponseSize, "\"sw\":\"%s (%s)\",\"hw\":%lu", FW_REV_STR, FW_DATE_STR, hw);

  if (success)
  {
    strncpy(_ResponseBuffer, output, _MaxResponseSize);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'reset' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_Reboot(char* StreamTokens)
{
  int items;
  uint16_t entry;

  items = sscanf(_Tokens[1], "%hu", &entry);

  if (items <= 0)
  {
    // Will not return as system will reboot
    SYSTEM__BeginShutdown(0);
  }
  else if (items == 1)
  {
    // Will not return as system will reboot
    SYSTEM__BeginShutdown(entry);
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'reset' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_Reset(char* StreamTokens)
{
  // Will not return as system will reboot
  SYSTEM_FactoryReset();

  return true;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'daqmeta' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_Meta(char* StreamTokens)
{
  bool success = false;
  char output[TECH_RESPONSE_MAXSIZE];
  int entry, items;

  items = sscanf(_Tokens[1], "%i", &entry);

  if (items <= 0)
  {
    // Meta data of the DAQ interface
    snprintf(output, _MaxResponseSize, "%s:{\"items\":%lu,\"version\":%u}", _Meta, _NumDAQEntries, _Version);

    success = true;
  }
  else if (items != 1)
  {
    // Invalid format
  }
  else if (entry < 0)
  {
    // Invalid
  }
  else if (entry < (signed)_NumDAQEntries)
  {
    tDAQ_Config config;
    DAQ_GetMetadata((tDAQ_Entry)entry, &config);

    snprintf(
      output,
      _MaxResponseSize,
      "%s:{\"item\":%i,\"enabled\":%u,\"ro\":%u,\"mod\":%u,\"int\":%lu,\"name\":\"%s\",\"desc\":\"%s\",\"subitems\":"
      "%u,\"scale\":%.2f,\"min\":%.2f,\"max\":%.2f,\"units\":\"%s\"}",
      _Meta,
      entry,
      config.Enabled,
      config.ReadOnly,
      config.Modified,
      config.Interval,
      (const char*)config.Name,
      (const char*)config.Description,
      config.Items,
      config.Scale,
      config.Min,
      config.Max,
      (const char*)config.Units);

    success = true;
  }

  if (success)
  {
    strncpy(_ResponseBuffer, output, _MaxResponseSize);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'task' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_Tasks(char* StreamTokens)
{
  int entry, items;
  char output[TECH_RESPONSE_MAXSIZE];
  bool success = false;

  items = sscanf(_Tokens[1], "%i", &entry);

  if (items < 1)
  {
    // Print summary
    if (TASK_PrintStatus((tTask)0, true, output, _MaxResponseSize) > 0)
    {
      success = true;
    }
  }
  else if (entry < eTask_NUM)
  {
    if (TASK_PrintStatus((tTask)entry, false, output, _MaxResponseSize) > 0)
    {
      success = true;
    }
  }

  if (success)
  {
    strncpy(_ResponseBuffer, output, _MaxResponseSize);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'daq' command
 @param     StreamTokens: command parameters if streaming
 @return    True if successful
 *******************************************************************/
static bool _Handler_Get(char* StreamTokens)
{
  bool success = false;
  int items;
  int v[2];

  if (StreamTokens == NULLPTR)
  {
    // Important: There must be no spaces between items or they would
    // have been broken into seperate tokens in _Tokenize()
    items = sscanf(_Tokens[1], "%i,%i", &v[0], &v[1]);
  }
  else
  {
    // Handler is being called by a streaming process
    items = sscanf(StreamTokens, "%i,%i", &v[0], &v[1]);
  }

  if (items < 1)
  {
    // No items
  }
  else if (items > 2)
  {
    // Too many items
  }
  else
  {
    tDAQ_Entry entry = (tDAQ_Entry)v[0];
    uint8_t index = (items == 2) ? v[1] : 0;
    char item[TECH_RESPONSE_MAXSIZE];
    char output[TECH_RESPONSE_MAXSIZE] = { '\0' };
    char note[MAX_NOTE_SIZE] = { '\0' };

    if (entry < _NumDAQEntries)
    {
      tDataValue value;
      if (DAQ_GetItem(entry, index, &value))
      {
        char buf[20];
        success = true;
        tDAQ_Config config;

        DAQ_GetMetadata(entry, &config);
        strncpy(note, config.Name, MAX_NOTE_SIZE - 1);

        sprintf(buf, "%i[%i]", entry, index);
        switch (entry)
        {
          case eDAQ_NOP:
            snprintf(item, _MaxResponseSize, "\"%s\":%lu", buf, value.U32);
            break;

          case eDAQ_Test:
            snprintf(item, _MaxResponseSize, "\"%s\":%.2f", buf, value.Float);
            break;

          case eDAQ_TempPCB:
            snprintf(item, _MaxResponseSize, "\"%s\":%.2f", buf, value.Float);
            break;

          case eDAQ_Vref:
            snprintf(item, _MaxResponseSize, "\"%s\":%.2f", buf, value.Float);
            break;

          case eDAQ_Uptime:
            snprintf(item, _MaxResponseSize, "\"%s\":%lu", buf, value.U32);
            break;

          case eDAQ_Sleep:
            snprintf(item, _MaxResponseSize, "\"%s\":%lu", buf, value.U32);
            break;

          case eDAQ_ClassBTest:
            strncpy(note, ClassB_GetRuntimeName(index), MAX_NOTE_SIZE - 1);
            snprintf(item, _MaxResponseSize, "\"%s\":%lu", buf, value.U32);
            break;

          default:
            // Should never be here. Did you forget add a new DAQ item as a case statement?
            success = false;
            snprintf(item, _MaxResponseSize, "\"%s\":\"%s\"", buf, "error");
            break;
        }
      }
    }

    if (success)
    {
      strcat(output, item);

      // Append note if there is one
      if (strlen(note) > 0)
      {
        char buf[TECH_RESPONSE_MAXSIZE];
        sprintf(buf, ",\"note\":\"%s\"", note);
        strncat(output, buf, TECH_RESPONSE_MAXSIZE - strlen(output) - 1);
      }

      // Copy final output to response buffer
      strncpy(_ResponseBuffer, output, _MaxResponseSize);
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'set' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful

 @note      Only numeric values allowed. No strings or pointers
 *******************************************************************/
static bool _Handler_Set(char* StreamTokens)
{
  bool success = false;
  int entry, items;
  float value;

  // Value must be numeric
  items = sscanf(_Tokens[1], "%i,%f", &entry, &value);

  if (items < 2)
  {
    // Invalid format
  }
  else if (entry < 0)
  {
    // Invalid
  }
  else if (entry < (signed)_NumDAQEntries)
  {
    tDAQ_Config config;
    DAQ_GetMetadata((tDAQ_Entry)entry, &config);

    if (!config.ReadOnly)
    {
      tDataValue val;

      // Value is always scanned and sent as a float. The callback should cast as appropriate
      val.Float = value;

      success = DAQ_WriteItem((tDAQ_Entry)entry, 0, val);
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'readmem' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful

 @note      Only numeric values allowed. No strings or pointers
 *******************************************************************/
static bool _Handler_ReadMemory(char* StreamTokens)
{
  bool success = false;
  int items;
  uint32_t address, len;

  // Scan for hex address and number of bytes to read. Address must be in hex, value must be numeric
  items = sscanf(_Tokens[1], "%lx,%lu", &address, &len);

  if (items < 2)
  {
    // Invalid format
  }
  else if (len == 0)
  {
    // Invalid length
  }
  else
  {
    if (len > MAX_MEMREAD_LEN)
    {
      // Too many bytes requested to read
      len = MAX_MEMREAD_LEN;
    }

    // Read memory and build response string
    uint8_t buffer[MAX_MEMREAD_LEN];
    if (ReadMemory((uintptr_t)address, buffer, len) > 0)
    {
      char output[TECH_RESPONSE_MAXSIZE];
      char data[TECH_RESPONSE_MAXSIZE];
      char* ptr = data;

      // Fill text buffer with LSB on the right (little-endian format)
      for (int32_t i = len - 1; i >= 0; i--)
      {
        ptr += sprintf(ptr, "%02X", buffer[i]);
      }

      snprintf(output, _MaxResponseSize, "\"address\":%08lX,\"data\":\"%s\"", address, data);
      strncpy(_ResponseBuffer, output, _MaxResponseSize);

      success = true;
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'log' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful

 @note      Only numeric values allowed. No strings or pointers
 *******************************************************************/
static bool _Handler_GetLog(char* StreamTokens)
{
  char output[TECH_RESPONSE_MAXSIZE];
  char log[MAX_DEBUGLOG_CHARS_TOT];
  int entry, items;
  bool success = false;

  items = sscanf(_Tokens[1], "%i", &entry);

  if (items == 1)
  {
    if (LOG_GetCachedEntry(eLogger_Sys, entry, log) > 0)
    {
      snprintf(output, _MaxResponseSize, "\"entry\":%i, \"log\":%s", entry, log);
      success = true;
    }
    else
    {
      snprintf(output, _MaxResponseSize, "\"entry\":%i, \"log\":<invalid>", entry);
      success = true;
    }
  }

  if (success)
  {
    strncpy(_ResponseBuffer, output, _MaxResponseSize);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'ssitems' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_StreamItems(char* StreamTokens)
{
  bool success = false;
  uint32_t items = 0;
  int v[MAX_STREAM_ITEMS];
  int s[MAX_STREAM_ITEMS];
  tDAQ_Config config;
  int32_t index, subindex;
  char buf_header[COMMAND_BUFFER_SIZE_RX];
  char tmp[20];

  buf_header[0] = '\0';

  // Important: There must be no spaces between items or they would have been broken into seperate tokens in _Tokenize()
  // Number of scanned items must coordinate with MAX_STREAM_ITEMS
  items = sscanf(_Tokens[1],
                 "%i[%i],%i[%i],%i[%i],%i[%i],%i[%i]",
                 &v[0],
                 &s[0],
                 &v[1],
                 &s[1],
                 &v[2],
                 &s[2],
                 &v[3],
                 &s[3],
                 &v[4],
                 &s[4]);

  LOG_Write(eLogger_Sys, eLogLevel_Med, _Module, false, "sitems: %s", _Tokens[1]);

  if (_Runtime.Stream.Active)
  {
    // Already streaming
  }
  else if (items < 2)
  {
    // No items parsed
  }
  else if (items % 2 != 0)
  {
    // Items should be in pairs of value[subvalue]
  }
  else if (items > _MaxStreamItems * 2)  // Items are in pairs value[subvalue], so max items is 2x MAX_STREAM_ITEMS
  {
    // Too many items
  }
  else
  {
    _ResetStream(true);

    items /= 2;  // Convert from number of parsed values to number of item pairs
    success = true;
    for (uint32_t i = 0; i < _MaxStreamItems; i++)
    {
      if (i < items)
      {
        if (v[i] < (signed)_NumDAQEntries)
        {
          // Valid
          _Runtime.Stream.Items[i] = v[i];
          _Runtime.Stream.SubItems[i] = s[i];
        }
      }
      else
      {
        // Invalid item. Can't proceed
        break;
      }
    }

    // If successful, build token string that will be used in the command handler instead if
    // the scanned (scanf) tokens, and build header string
    if (success && (_Runtime.Stream.Items[0] > 0))
    {
      index = _Runtime.Stream.Items[0];
      subindex = _Runtime.Stream.SubItems[0];

      // First header item
      DAQ_GetMetadata((tDAQ_Entry)index, &config);
      sprintf(buf_header, "\"%li[%li]\":\"%s\"", index, subindex, config.Name);
      strcpy(_Runtime.Stream.Header, buf_header);

      // Additional tokens and header
      for (uint32_t i = 1; i < _MaxStreamItems; i++)
      {
        index = _Runtime.Stream.Items[i];
        subindex = _Runtime.Stream.SubItems[i];
        if (index > 0)
        {
          // Header data response string
          DAQ_GetMetadata((tDAQ_Entry)index, &config);
          snprintf(tmp, 19, ",\"%li[%li]\":\"%s\"", index, subindex, config.Name);
          strcat(buf_header, tmp);
          strcpy(_Runtime.Stream.Header, buf_header);
        }
        else
        {
          // No more items
          break;
        }
      }
    }

    // Return the header as response data
    strcpy(_ResponseBuffer, buf_header);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'sstart' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_StreamStart(char* StreamTokens)
{
  bool success = false;
  int items;
  char command[MAX_CMD_SIZE];
  int32_t freq;  // Hertz (updates per second)

  items = sscanf(_Tokens[1], "%[^,],%li", command, &freq);
  LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "sstart: command=%s, freq=%li", command, freq);

  if (_Runtime.Stream.Active)
  {
    // Already streaming something
  }
  else if (items < 2)
  {
    // Need 2 items, command and freq
  }
  else if (freq < _StreamMinFreq)
  {
    // Streaming freq too small
  }
  else if (freq > _StreamMaxFreq)
  {
    // Streaming freq too big
  }
  else
  {
    for (tTechCommands i = (tTechCommands)0; i < NUM_TECH_COMMANDS; i++)
    {
      if (strcmp(command, _Config[i].Name) != 0)
      {
        // No match. Keep comparing...
      }
      else if (_Config[i].Streamable)
      {
        // Matched and streamable. Everything looks good. Set the handler
        strcpy(_Runtime.Stream.Command, command);
        _Runtime.Stream.Handler = _GetHandlerFromCommand(command);

        if (!IsPointerValid((uintptr_t)_Runtime.Stream.Handler))
        {
          assert_always();
        }
        else if (_Runtime.Stream.Items[0] >= 0)
        {
          // Proceed with streaming
          _Runtime.Stream.Active = true;
          _Runtime.Stream.Timestamp = 0;
          _Runtime.Stream.Interval = (uint32_t)(1000.0f / freq);
          _Runtime.Stream.Time = TIMER_GetTick();

          // Disable log output while streaming
          LOG_EnableOutput(eLogger_Tech, false);
          if (_DisableSysLogging)
          {
            LOG_EnableOutput(eLogger_Sys, false);
          }

          success = true;
        }
        else
        {
          LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, true, "No stream items specified.");
        }

        break;
      }
      else
      {
        LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, true, "Command '%s' is not streamable.", command);
        break;
      }
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'sstop' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_StreamStop(char* StreamTokens)
{
  if (_Runtime.Stream.Active)
  {
    _ResetStream(false);
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Command handler for 'fault' command
 @param     StreamTokens: command parameters if streaming (not used for this command)
 @return    True if successful
 *******************************************************************/
static bool _Handler_Fault(char* StreamTokens)
{
  char output[TECH_RESPONSE_MAXSIZE];
  int entry, items;
  bool success = false;

  items = sscanf(_Tokens[1], "%i", &entry);

  if (items == 1)
  {
    success = ClassB__SetFault((tClassBRunItem)entry);

    snprintf(output, _MaxResponseSize, "%s:{\"fault\":%i,\"}", _Meta, entry);
  }

  if (success)
  {
    strncpy(_ResponseBuffer, output, _MaxResponseSize);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Get the handler of the specified command
 @param     Cmd: The command
 @return    A pointer to the handler function
 *******************************************************************/
static fnHandler* _GetHandlerFromCommand(const char* Cmd)
{
  // For now, the only handler is get()
  return _Handler_Get;
}

/*******************************************************************/
/*!
 @brief     Write a message to the port
 @param     Message: The message
 @param     InsertCR: Insert pre cr when true
 *******************************************************************/
static void _WriteMessage(const char* Message, bool InsertCR)
{
  if (Message != NULLPTR)
  {
    SERIAL_SendString(_Port, Message);

    if (InsertCR)
    {
      SERIAL_SendString(_Port, "\r\n");
    }
  }
}
