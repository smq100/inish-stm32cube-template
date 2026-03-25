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

 @file    queue.h
 @brief   Interface of a basic application queue
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __QUEUE_H
#define __QUEUE_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

//! Queue data structure
typedef struct
{
  uint8_t* QueuePtr;     //!< Pointer to queue
  uint8_t* MaxPtr;       //!< Last address of queue
  uint8_t* InPtr;        //!< Next available queue location
  uint8_t* OutPtr;       //!< Next element to be read from queue
  uint16_t ElementSize;  //!< Data size of each element in the queue
  uint16_t Capacity;     //!< Maximum number of elements allowed in the queue
} tQueue;

/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

bool QUEUE_Init(tQueue* Queue, uint8_t* Buffer, uint16_t ElementSize, uint16_t Capacity, const char* Name);
bool QUEUE_Enque(tQueue* Queue, const void* Data);
bool QUEUE_Deque(tQueue* Queue, void* Data);
void QUEUE_Flush(tQueue* Queue);
bool QUEUE_IsEmpty(const tQueue* Queue);
bool QUEUE_IsFull(const tQueue* Queue);
uint16_t QUEUE_GetCount(const tQueue* Queue);

#ifdef QUEUE_PROTECTED

#endif /* QUEUE_PROTECTED */

#endif /* __QUEUE_H */
