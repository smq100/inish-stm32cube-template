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

 @file    task_serial.c
 @brief   Implementation of serial interface task (and LOG module)
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#include "main.h"
#include "task_serial.h"
#include "uart.h"
#include "usart.h"
#include "queue.h"
#include "timer.h"
#include "log.h"
#include "util.h"

/* Private typedef -----------------------------------------------------------*/

//! UART configuration data structure
typedef struct
{
  tQueue* RxQueue;        ///< Rx queue
  uint8_t* RxBuffer;      ///< Rx write buffer
  uint32_t RxBufferSize;  ///< Rx buffer size
  tQueue* TxQueue;        ///< Tx queue
  uint8_t* TxBuffer;      ///< Tx write buffer
  uint32_t TxBufferSize;  ///< Tx buffer size
} tQueueConfig;

/* Private define ------------------------------------------------------------*/

#define BUFFER_DEBUG_RX_SZ 8  // No Rx on debug port
#define BUFFER_DEBUG_TX_SZ 2048
#define BUFFER_TECH_RX_SZ 2048
#define BUFFER_TECH_TX_SZ 2048

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "T_SER";           //!< Module name for debug logging
static uint32_t const _PacketMax = PACKET_MAX;  //!< Max size to UART packet
static uint32_t const _Timeout_ms = 100;        //!< Timeout in ms

#ifdef TEST__ENABLE_LOGGING
static const tLogLevel _LoggingLevel = LOGGING_DEFAULT_LEVEL;  //!< Default debug serial logging level
#else
static const tLogLevel _LoggingLevel = eLogLevel_None;  //!< Default debug
#endif

static const UART_HandleTypeDef* _Handle[NUM_SERIAL_PORTS] = { &huart1, &huart2 };  //!< HAL UART handles for each port

//! Rx write buffer size
static uint32_t const _RxBufferSize[NUM_SERIAL_PORTS] = {
  BUFFER_DEBUG_RX_SZ,
  BUFFER_TECH_RX_SZ,
};

//! Rx write buffer size
static uint32_t const _TxBufferSize[NUM_SERIAL_PORTS] = {
  BUFFER_DEBUG_TX_SZ,
  BUFFER_TECH_TX_SZ,
};

static_assert(sizeof(_Handle) / sizeof(_Handle[0]) == NUM_SERIAL_PORTS, "UART handle array size mismatch");
static_assert(sizeof(_RxBufferSize) / sizeof(_RxBufferSize[0]) == NUM_SERIAL_PORTS,
              "Rx buffer size array size mismatch");
static_assert(sizeof(_TxBufferSize) / sizeof(_TxBufferSize[0]) == NUM_SERIAL_PORTS,
              "Tx buffer size array size mismatch");

static bool _Initialized[NUM_SERIAL_PORTS] = { false };  //!< True if module is initialized
static bool _Enabled[NUM_SERIAL_PORTS] = { false };      //!< True if port is enabled

// Buffers
static uint8_t _RxBuffer_Debug[BUFFER_DEBUG_RX_SZ];  //!< The Rx buffer used by the queue
static uint8_t _TxBuffer_Debug[BUFFER_DEBUG_TX_SZ];  //!< The Tx buffer used by the queue
static uint8_t _RxBuffer_Tech[BUFFER_TECH_RX_SZ];    //!< The Rx buffer used by the queue
static uint8_t _TxBuffer_Tech[BUFFER_TECH_TX_SZ];    //!< The Tx buffer used by the queue

//! Pointers to the Rx buffers used by the queues
static uint8_t* _RxBuffer[NUM_SERIAL_PORTS] = {
  _RxBuffer_Debug,
  _RxBuffer_Tech,
};

//! Pointers to the Tx buffers used by the queues
static uint8_t* _TxBuffer[NUM_SERIAL_PORTS] = {
  _TxBuffer_Debug,
  _TxBuffer_Tech,
};

// Queues
static tQueueConfig _Queue[NUM_SERIAL_PORTS];
static tQueue _RxQueue[NUM_SERIAL_PORTS];  //!< The receive queues
static tQueue _TxQueue[NUM_SERIAL_PORTS];  //!< The transmit queues

//! Final buffer for Tx
static char _TxUARTBuffer[NUM_SERIAL_PORTS][PACKET_MAX] = { 0 };  //!< Tx packet buffer

//! Errors
static uint32_t _RxOverflow[NUM_SERIAL_PORTS] = { 0 };  //!< Number of times Rx queue overlflowed
static uint32_t _TxOverflow[NUM_SERIAL_PORTS] = { 0 };  //!< Number of times Tx queue overlflowed

/* Private function prototypes -----------------------------------------------*/

static uint32_t _FillTxUARTBuffer(tSerialPort Port);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the specified serial port
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SERIAL_Init(void)
{
  bool success = true;

  for (tSerialPort port = (tSerialPort)0; port < NUM_SERIAL_PORTS; port++)
  {
    _Initialized[port] = UART_Init(port, _Handle[port]);

    if (_Initialized[port])
    {
      // Initialize queues for receiving and transmitting characters
      _Queue[port].RxQueue = &(_RxQueue[port]);
      _Queue[port].RxBuffer = _RxBuffer[port];
      _Queue[port].RxBufferSize = _RxBufferSize[port];
      bool init =
        QUEUE_Init(_Queue[port].RxQueue, _Queue[port].RxBuffer, sizeof(uint8_t), _Queue[port].RxBufferSize, "Rx");
      assert(init);

      _Queue[port].TxQueue = &(_TxQueue[port]);
      _Queue[port].TxBuffer = _TxBuffer[port];
      _Queue[port].TxBufferSize = _TxBufferSize[port];
      init = QUEUE_Init(_Queue[port].TxQueue, _Queue[port].TxBuffer, sizeof(uint8_t), _Queue[port].TxBufferSize, "Tx");
      assert(init);

      _Enabled[port] = true;
    }
    else
    {
      QUEUE_Flush(_Queue[port].TxQueue);
      QUEUE_Flush(_Queue[port].RxQueue);
    }
  }

  // Initialize the system debug logger
  if (_Initialized[eSerial_Debug])
  {
    if (!LOG_Init(eLogger_Sys, eSerial_Debug, _LoggingLevel, true))
    {
      _Initialized[eSerial_Debug] = false;
    }
  }
  else
  {
    success = false;
  }

  // Initialize the tech logger
  if (_Initialized[eSerial_Tech])
  {
    if (!LOG_Init(eLogger_Tech, eSerial_Tech, _LoggingLevel, true))
    {
      _Initialized[eSerial_Tech] = false;
    }
  }
  else
  {
    success = false;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Executive loop of the SERIAL task
 @return    True if successful

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SERIAL_Exec(void)
{
  tSerialPort port;
  uint32_t count;
  static uint32_t txBusySinceMs = 0;

  // Process Tx
  for (port = (tSerialPort)0; port < NUM_SERIAL_PORTS; port++)
  {
    if (QUEUE_GetCount(_Queue[port].TxQueue) == 0)
    {
      // Nothing to send
    }
    else if (!UART_IsTxReady(port))
    {
      // UART is busy; detect prolonged busy and recover
      if (txBusySinceMs == 0)
      {
        txBusySinceMs = TIMER_GetTick();
      }
      else if (TIMER_GetElapsed_ms(txBusySinceMs) > 200)
      {
        HAL_UART_Abort((UART_HandleTypeDef*)_Handle[port]);
        HAL_UART_DeInit((UART_HandleTypeDef*)_Handle[port]);

        if (_Handle[port] == &huart1)
        {
          MX_USART1_UART_Init();
        }
        else if (_Handle[port] == &huart2)
        {
          MX_USART2_UART_Init();
        }
        else
        {
          // Should never be here
          assert_always();
        }

        UART_Init(port, _Handle[port]);
        txBusySinceMs = 0;
      }
    }
    else
    {
      txBusySinceMs = 0;
      count = _FillTxUARTBuffer(port);
      if (count > 0)
      {
        // Buffer filled successfully; Send it
        UART_SendPacket(port, (uint8_t*)_TxUARTBuffer[port], count);
      }
    }
  }

  return true;
}

/*******************************************************************/
/*!
 @brief     Shutdown processing of the SERIAL task
 @return    True if ok for the system to powerdown

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SERIAL_Shutdown(void)
{
  // No shutdown processing required
  return true;
}

/*******************************************************************/
/*!
 @brief     Performs a test of the SENSOR task
 @return    True if successful; no errors detected

 @note      Function takes the form of the TASK prototypes (see task.c/h)
 *******************************************************************/
bool SERIAL_Test(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Enabled or disables the port
 @param     Port: The desired serial port
 @param     Enable: Enable the port when true
 *******************************************************************/
void SERIAL_SetEnabled(tSerialPort Port, bool Enable)
{
  if (Port < NUM_SERIAL_PORTS)
  {
    _Enabled[Port] = Enable;
    LOG_Write(eLogger_Sys,
              eLogLevel_Low,
              _Module,
              false,
              "%s %s",
              Enable ? "Enabled" : "Disabled",
              (Port == eSerial_Debug) ? "Debug" : "Tech");
  }
}

/*******************************************************************/
/*!
 @brief     Returns enabled status of port
 @param     Port: The desired serial port
 @return    True if enabled
 *******************************************************************/
bool SERIAL_IsEnabled(tSerialPort Port)
{
  return (Port < NUM_SERIAL_PORTS) ? _Enabled[Port] : false;
}

/*******************************************************************/
/*!
 @brief     Sends a packet to the serial port
 @param     Port: The desired serial port
 @param     Packet: The pointer to the array of data
 @param     Size: The number of bytes to send
 @return    The number of bytes sent
 *******************************************************************/
int32_t SERIAL_SendPacket(tSerialPort Port, const uint8_t* Packet, uint8_t Size)
{
  int32_t count;
  int32_t sent = 0;

  if (!IsPointerValid((uintptr_t)Packet))
  {
    assert_always();
    count = -1;
  }
  else if (Size == 0)
  {
    // Nothing to do
    count = 0;
  }
  else
  {
    for (count = 0, sent = 0; count < Size; count++)
    {
      if (SERIAL_SendByte(Port, Packet[count]) > 0)
      {
        sent++;
      }
      else
      {
        _TxOverflow[Port]++;
      }
    }
  }

  return sent;
}

/*******************************************************************/
/*!
 @brief     Sends a char to the serial port
 @param     Port: The desired serial port
 @param     C: The char to send
 @return    1 if successful, otherwise 0
 *******************************************************************/
int32_t SERIAL_SendByte(tSerialPort Port, uint8_t C)
{
  int32_t count = 0;

  if (Port >= NUM_SERIAL_PORTS)
  {
    // No
  }
  else if (!_Enabled[Port])
  {
    // Ignore
  }
  else if (QUEUE_Enque(_Queue[Port].TxQueue, &C) > 0)
  {
    count = 1;
  }

  return count;
}

/*******************************************************************/
/*!
 @brief     Sends a zero-terminated string to the serial port
 @param     Port: The desired serial port
 @param     Str: The string
 @return    True if successful
 *******************************************************************/
bool SERIAL_SendString(tSerialPort Port, char const* Str)
{
  int32_t count = SERIAL_SendPacket(Port, (const uint8_t*)Str, strlen(Str));

  return (count >= 0);
}

/*******************************************************************/
/*!
 @brief     Retreives a char from the serial port
 @param     Port: The desired serial port
 @param     C: The char to receive
 @return    1 if successful, otherwise 0
 *******************************************************************/
int32_t SERIAL_ReceiveByte(tSerialPort Port, uint8_t* C)
{
  int32_t count = 0;

  if (Port >= NUM_SERIAL_PORTS)
  {
    // No
  }
  else if (!_Enabled[Port])
  {
    // Ignore
  }
  else if (QUEUE_Deque(_Queue[Port].RxQueue, C))
  {
    count = 1;
  }

  return count;
}

/*******************************************************************/
/*!
 @brief     Retrieves a packet from the serial port
 @param     Port: The desired serial port
 @param     Packet: The pointer to the array of data
 @param     Size: The number of bytes to retrieve
 @return    The number of bytes recieved
 *******************************************************************/
int32_t SERIAL_ReceivePacket(tSerialPort Port, uint8_t* Packet, uint8_t Size)
{
  uint8_t c;
  uint32_t count = 0;
  uint32_t time = TIMER_GetTick();

  while (count < Size)
  {
    if (SERIAL_ReceiveByte(Port, &c) > 0)
    {
      Packet[count++] = c;
      time = TIMER_GetTick();
    }
    else if (TIMER_GetElapsed_ms(time) >= _Timeout_ms)
    {
      // Timeout error
      break;
    }
  }

  return count;
}

/*******************************************************************/
/*!
 @brief     Gets the number of bytes in the Rx queue
 @param     Port: The desired serial port
 @return    The number of bytes in the Rx queue
 *******************************************************************/
int16_t SERIAL_GetRxCount(tSerialPort Port)
{
  return QUEUE_GetCount(_Queue[Port].RxQueue);
}

/*******************************************************************/
/*!
 @brief     Gets the number of bytes in the Tx queue
 @param     Port: The desired serial port
 @return    The number of bytes in the Tx queue
 *******************************************************************/
int16_t SERIAL_GetTxCount(tSerialPort Port)
{
  return QUEUE_GetCount(_Queue[Port].TxQueue);
}

/*******************************************************************/
/*!
 @brief     Gets the number times Rx buffer has overflowed
 @param     Port: The desired serial port
 @return    The number of overflows
 *******************************************************************/
int32_t SERIAL_GetRxOverflowCount(tSerialPort Port)
{
  return (Port < NUM_SERIAL_PORTS) ? _RxOverflow[Port] : 0;
}

/*******************************************************************/
/*!
 @brief     Gets the number times Tx buffer has overflowed
 @param     Port: The desired serial port
 @return    The number of overflows
 *******************************************************************/
int32_t SERIAL_GetTxOverflowCount(tSerialPort Port)
{
  return (Port < NUM_SERIAL_PORTS) ? _TxOverflow[Port] : 0;
}

/*******************************************************************/
/*!
 @brief     Flush the Rx queue
 @param     Port: The desired serial port
 @return    True if successful
 *******************************************************************/
bool SERIAL_FlushRx(tSerialPort Port)
{
  bool success = false;

  if (Port < NUM_SERIAL_PORTS)
  {
    QUEUE_Flush(_Queue[Port].RxQueue);
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Flush the Rx queue
 @param     Port: The desired serial port
 @return    True if successful
 *******************************************************************/
bool SERIAL_FlushTx(tSerialPort Port)
{
  bool success = false;

  if (Port < NUM_SERIAL_PORTS)
  {
    QUEUE_Flush(_Queue[Port].TxQueue);
    success = true;
  }

  return success;
}

/* Private Implementation -----------------------------------------------------*/

static uint32_t _FillTxUARTBuffer(tSerialPort Port)
{
  uint32_t count = 0;
  uint8_t c;

  while (count < _PacketMax)
  {
    if (QUEUE_GetCount(_Queue[Port].TxQueue) == 0)
    {
      // Nothing in the queue
      break;
    }
    else if (QUEUE_Deque(_Queue[Port].TxQueue, &c))
    {
      _TxUARTBuffer[Port][count++] = c;
    }
    else
    {
      // Problem de-queueing? Should never be here
      assert_always();
      break;
    }
  }

  return count;
}

/* Protected Implementation -----------------------------------------------------*/

uint32_t SERIAL__PushRxChar(tSerialPort Port, uint8_t c)
{
  uint32_t ret = 0;

  if (_Enabled[Port])
  {
    if (QUEUE_Enque(_Queue[Port].RxQueue, &c))
    {
      ret = 1;
    }
    else
    {
      _RxOverflow[Port]++;
    }
  }

  return ret;
}
