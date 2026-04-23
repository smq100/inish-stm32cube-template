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

 @file    task_serial.h
 @brief   Interface of a RS232 serial interface
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#ifndef __TASK_SERIAL_H
#define __TASK_SERIAL_H

#include "common.h"
#include "task.h"

/* Exported types ------------------------------------------------------------*/

//! Task function prototypes (See task.c/h)
fnTaskInit SERIAL_Init;
fnTaskExec SERIAL_Exec;
fnTaskShutdown SERIAL_Shutdown;
fnTaskTest SERIAL_Test;

/* Exported constants --------------------------------------------------------*/

#define PACKET_MAX 255  //!< Max size to UART packet

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

void SERIAL_SetEnabled(tSerialPort Port, bool Enable);
bool SERIAL_IsEnabled(tSerialPort Port);
uint32_t SERIAL_SendPacket(tSerialPort Port, const uint8_t* Packet, uint16_t Size);
uint32_t SERIAL_SendByte(tSerialPort Port, uint8_t C);
bool SERIAL_SendString(tSerialPort Port, char const* Str);
uint32_t SERIAL_ReceiveByte(tSerialPort Port, uint8_t* C);
uint32_t SERIAL_ReceivePacket(tSerialPort Port, uint8_t* Packet, uint16_t Size);
uint16_t SERIAL_GetRxCount(tSerialPort Port);
uint16_t SERIAL_GetTxCount(tSerialPort Port);
uint32_t SERIAL_GetRxOverflowCount(tSerialPort Port);
uint32_t SERIAL_GetTxOverflowCount(tSerialPort Port);
bool SERIAL_FlushRx(tSerialPort Port);
bool SERIAL_FlushTx(tSerialPort Port);

#ifdef SERIAL_PROTECTED

uint32_t SERIAL__PushRxChar(tSerialPort Uart, uint8_t C);

#endif /* SERIAL_PROTECTED */

#endif /* __TASK_SERIAL_H */
