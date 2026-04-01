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

 @file    uart.h
 @brief   Interface of the UART interface
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __UART_H
#define __UART_H

#include "main.h"
#include "common.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* Exported vars ------------------------------------------------------------ */

/* Exported functions ------------------------------------------------------- */

bool UART_Init(tSerialPort Port, const UART_HandleTypeDef* Handle);
bool UART_IsTxReady(tSerialPort Port);
bool UART_SendByte(tSerialPort Port, uint8_t Byte);
bool UART_GetByte(tSerialPort Port, uint8_t* Data);
bool UART_SendString(tSerialPort Port, const char* Str, bool Direct);
int32_t UART_SendPacket(tSerialPort Port, uint8_t* Packet, uint32_t Size);
int32_t UART_ReceivePacket(tSerialPort Port, uint8_t* Packet, uint8_t Size);
void UART_ClearQueues(tSerialPort Port);

#ifdef UART_PROTECTED

/* Protected functions ------------------------------------------------------ */

#endif /* UART_PROTECTED */

#endif /* __UART_H */
