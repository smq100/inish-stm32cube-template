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

 @file    uart.c
 @brief   Implementation of the UART interface (abstraction of STM32 UARTs)
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#define SERIAL_PROTECTED
#define UART_PROTECTED

#include "main.h"
#include "uart.h"
#include "util.h"
#include "task_serial.h"

/* Private typedef -----------------------------------------------------------*/

typedef void(IRQHandler)(void);

//! UART configuration data structure
typedef struct
{
  bool RxEnabled;  //!< True if UART will receive chars
} tUARTConfig;

//! UART runtime data structure
typedef struct
{
  const UART_HandleTypeDef* Handle;  //!< HAL UART handle
  bool Initialized;                  //!< True if the UART is initialized
  bool Busy;                         //!< True if the UART is busy
  uint32_t Error;                    //!< Error count
} tUARTRuntime;

/* Private define ------------------------------------------------------------*/

#define BUFFER_UART_RX_SZ 256
#define BUFFER_UART_TX_SZ 1

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//! UART configuration
static const tUARTConfig _Config[NUM_SERIAL_PORTS] = {
  { .RxEnabled = false },  // eSerial_Debug
  { .RxEnabled = true },   // eSerial_Tech
};

static_assert(sizeof(_Config) / sizeof(_Config[0]) == NUM_SERIAL_PORTS, "UART config size mismatch");

static tUARTRuntime _Runtime[NUM_SERIAL_PORTS];
static uint8_t _RxBuffer[NUM_SERIAL_PORTS][BUFFER_UART_RX_SZ];
static uint8_t _TxCharBuffer[NUM_SERIAL_PORTS][BUFFER_UART_TX_SZ];

/* Private function prototypes -----------------------------------------------*/

static tSerialPort _HandleToPort(const UART_HandleTypeDef* Handle);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initialize the connection to the UART
 @param     port: The desired uart
 @param     handle: The HAL UART handle
 @return    True if successful
 *******************************************************************/
bool UART_Init(tSerialPort Port, const UART_HandleTypeDef* Handle)
{
  bool success = false;

  if (Port < NUM_SERIAL_PORTS)
  {
    _Runtime[Port].Handle = Handle;
    _Runtime[Port].Initialized = true;
    _Runtime[Port].Busy = false;
    _Runtime[Port].Error = 0;

    HAL_UART_Receive_IT((UART_HandleTypeDef*)_Runtime[Port].Handle, _RxBuffer[Port], 1);

    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Send byte over UART
 @param     port: The desired UART
 @return    True if successful
 *******************************************************************/
bool UART_IsTxReady(tSerialPort Port)
{
  bool ready = false;

  if (Port >= NUM_SERIAL_PORTS)
  {
  }
  else if (!_Runtime[Port].Initialized)
  {
  }
  else if (_Runtime[Port].Error > 0)
  {
  }
  else
  {
    ready = (_Runtime[Port].Handle->gState == HAL_UART_STATE_READY);
  }

  return ready;
}

/*******************************************************************/
/*!
 @brief     Send byte over UART
 @param     port: The desired UART
 @param     byte: The character to be sent
 @return    True if successful
 *******************************************************************/
bool UART_SendByte(tSerialPort Port, uint8_t Byte)
{
  bool success = false;

  if (_Runtime[Port].Initialized)
  {
    _TxCharBuffer[Port][0] = Byte;
    success = HAL_UART_Transmit_IT((UART_HandleTypeDef*)_Runtime[Port].Handle, _TxCharBuffer[Port], 1);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Pull the next byte off the receive queue for UARTx
 @param     port: The desired uart
 @param     data: The character pulled from the receive queue
 @return    True if successful
 *******************************************************************/
bool UART_GetByte(tSerialPort Port, uint8_t* Data)
{
  UNUSED(Port);
  UNUSED(Data);

  // TODO
  return false;
}

/*******************************************************************/
/*!
 @brief     Send string over UARTx
 @param     port: The desired port
 @param     Str: the string to be sent
 *******************************************************************/
bool UART_SendString(tSerialPort Port, const char* Str)
{
  HAL_StatusTypeDef ret = HAL_ERROR;

  if (!IsPointerValid((uintptr_t)Str))
  {
    assert_always();
  }
  else if (*Str == '\0')
  {
  }
  else
  {
    _Runtime[Port].Busy = true;
    ret = HAL_UART_Transmit_IT((UART_HandleTypeDef*)_Runtime[Port].Handle, (const uint8_t*)Str, strlen(Str));
  }

  return (ret == HAL_OK);
}

/*******************************************************************/
/*!
 @brief     Send packet over UART
 @param     port: the UART to use
 @param     Packet: the buffer to be sent
 @param     Size: the number of bytes to send
 *******************************************************************/
int32_t UART_SendPacket(tSerialPort Port, uint8_t* Packet, uint32_t Size)
{
  HAL_StatusTypeDef ret = HAL_ERROR;

  if (!IsPointerValid((uintptr_t)Packet))
  {
    assert_always();
  }
  else if (Size > 0)
  {
    _Runtime[Port].Busy = true;
    ret = HAL_UART_Transmit_IT((UART_HandleTypeDef*)_Runtime[Port].Handle, (const uint8_t*)Packet, Size);
  }

  return (ret == HAL_OK) ? Size : 0;
}

/*******************************************************************/
/*!
 @brief     Rcv packet from UART
 @param     port: the UART to use
 @param     Packet: the buffer to use to receive
 @param     Size: the number of bytes to receive
 *******************************************************************/
int32_t UART_ReceivePacket(tSerialPort Port, uint8_t* Packet, uint8_t Size)
{
  UNUSED(Port);
  UNUSED(Packet);

  int i;

  for (i = 0; i < Size; i++)
  {
    // TODO
  }

  return i;
}

/* Override Implementation -------------------------------------------------- */

/*******************************************************************/
/*!
 @brief     HAL Tx complete callback
 @param     huart: UART handle
 *******************************************************************/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
  tSerialPort port = _HandleToPort(huart);
  if (port < NUM_SERIAL_PORTS)
  {
    _Runtime[port].Busy = false;
  }
}

/*******************************************************************/
/*!
 @brief     HAL Rx complete callback
 @param     huart: UART handle
 *******************************************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
  tSerialPort port = _HandleToPort(huart);
  if (port < NUM_SERIAL_PORTS)
  {
    if (_Config[port].RxEnabled)
    {
      // Start receiving the next byte
      HAL_UART_Receive_IT((UART_HandleTypeDef*)_Runtime[port].Handle, _RxBuffer[port], 1);

      if (SERIAL_IsEnabled(port))
      {
        SERIAL__PushRxChar(port, _RxBuffer[port][0]);
      }
    }
  }
}

/*******************************************************************/
/*!
 @brief     HAL Tx complete callback
 @param     huart: UART handle
 *******************************************************************/
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
  tSerialPort port = _HandleToPort(huart);
  if (port < NUM_SERIAL_PORTS)
  {
    // _Runtime[port].Error++;
  }
}

/* Private Implementation ---------------------------------------------------- */

/*******************************************************************/
/*!
 @brief     Returns the serial port enum from the UART handle
 @param     Handle: The handle of the UART
 @return    The serial port enum. Value = NUM_SERIAL_PORTS if not found

 @note      Returns first port. Need to modify if multiple Rx ports on the same UART
 *******************************************************************/
static tSerialPort _HandleToPort(const UART_HandleTypeDef* Handle)
{
  tSerialPort port = NUM_SERIAL_PORTS;  // Set to invalid port
  for (tSerialPort p = (tSerialPort)0; p < NUM_SERIAL_PORTS; p++)
  {
    if (_Runtime[p].Handle == Handle)
    {
      port = p;
      break;
    }
  }

  return port;
}
