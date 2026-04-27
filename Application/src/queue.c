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

 @file    queue.c
 @brief   Implementation of a basic application queue
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#include "main.h"
#include "queue.h"
#include "util.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "QUEUE";  //!< Module name for debug logging

/* Private function prototypes -----------------------------------------------*/
/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initialize the queue structure
 @param     Queue: pointer to a QUEUE structure
 @param     Buffer: pointer to an element array
 @param     ElementSize: element data size (in uint8-t), for example, if it is a uint16_t array, set to
 sizeof(uint_16_t)
 @param     Capacity: maximum number of elements in the queue, for example, if its a uint16_t array[5], set to 5
 @param     Name: Name of queue
 @return    True if initialized

 The queue is considered full when 'Capacity - 1' elements are written to it.

 *******************************************************************/
bool QUEUE_Init(tQueue* Queue, uint8_t* Buffer, uint16_t ElementSize, uint16_t Capacity, const char* Name)
{
  bool success = true;

  // First, verify that Buffer is a valid RAM address
  if (!IsRAM((uintptr_t)Buffer))
  {
    assert_always();
    success = false;
  }
  else if (!IsRAM((uintptr_t)(Buffer + (ElementSize * Capacity - 1))))
  {
    assert_always();
    success = false;
  }
  else
  {
    Queue->QueuePtr = Buffer;
    Queue->MaxPtr = Buffer + (ElementSize * Capacity);
    Queue->InPtr = Buffer;
    Queue->OutPtr = Buffer;
    Queue->ElementSize = ElementSize;
    Queue->Capacity = Capacity;

    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Initialized: %s", Name);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Add an element to the end of the queue
 @param     Queue: Pointer to a queue structure
 @param     Data: Data to write to queue
 @return    True if queued

 Discards new element if an overflow will occur.
 The queue is considered full when 'Capacity - 1' elements are written to it

 *******************************************************************/
bool QUEUE_Enque(tQueue* Queue, const void* Data)
{
  bool success = false;
  uint8_t* new_in_ptr;

  // Pre-adjust in pointer to facilitate overflow check
  new_in_ptr = Queue->InPtr + Queue->ElementSize;
  if (new_in_ptr >= Queue->MaxPtr)
  {
    new_in_ptr = Queue->QueuePtr;
  }

  if (new_in_ptr == Queue->OutPtr)
  {
    // Don't allow overflow
  }
  else
  {
    // Put data in buffer and update in-pointer
    memcpy(Queue->InPtr, Data, Queue->ElementSize);
    Queue->InPtr = new_in_ptr;

    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Get an element from the queue
 @param     Queue: Pointer to a queue structure
 @param     Data: Data to write to queue
 @return    True if element is available

 *******************************************************************/
bool QUEUE_Deque(tQueue* Queue, void* Data)
{
  bool success = false;
  uint8_t* newOutPtr;

  // Pre-adjust pointer to facilitate underflow check
  newOutPtr = Queue->OutPtr + Queue->ElementSize;
  if (newOutPtr >= Queue->MaxPtr)
  {
    newOutPtr = Queue->QueuePtr;
  }

  if (Queue->OutPtr == Queue->InPtr)
  {
    // Nothing to deque; buffer is empty
  }
  else
  {
    // Get data from buffer and update out-pointer
    memcpy(Data, Queue->OutPtr, Queue->ElementSize);
    Queue->OutPtr = newOutPtr;

    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Flush (empty) the queue
 @param     Queue: Pointer to a queue structure


 *******************************************************************/
void QUEUE_Flush(tQueue* Queue)
{
  Queue->OutPtr = Queue->InPtr = Queue->QueuePtr;
}

/*******************************************************************/
/*!
 @brief     Check if the queue is empty
 @param     Queue: Pointer to a queue structure
 @return    True if empty

 *******************************************************************/
bool QUEUE_IsEmpty(const tQueue* Queue)
{
  return (Queue->OutPtr == Queue->InPtr);
}

/*******************************************************************/
/*!
 @brief     Check if the queue is full
 @param     Queue: Pointer to a queue structure
 @return    True if full

 *******************************************************************/
bool QUEUE_IsFull(const tQueue* Queue)
{
  uint8_t* NewInPtr;

  // Pre-adjust in pointer to facilitate check
  NewInPtr = Queue->InPtr + Queue->ElementSize;
  if (NewInPtr >= Queue->MaxPtr)
  {
    NewInPtr = Queue->QueuePtr;
  }

  return (NewInPtr == Queue->OutPtr);
}

/*******************************************************************/
/*!
 @brief     Get the number of elements in the queue
 @param     Queue: Pointer to a queue structure
 @return    Number of elements in queue

 *******************************************************************/
uint16_t QUEUE_GetCount(const tQueue* Queue)
{
  uint16_t count;
  uint8_t* new_in_ptr = Queue->InPtr;
  uint8_t* new_out_ptr = Queue->OutPtr;

  if (new_in_ptr == new_out_ptr)
  {
    count = 0;
  }
  else if (new_in_ptr > new_out_ptr)
  {
    count = (new_in_ptr - new_out_ptr) / Queue->ElementSize;
  }
  else
  {
    count = Queue->Capacity - ((new_out_ptr - new_in_ptr) / Queue->ElementSize);
  }

  return count;
}
