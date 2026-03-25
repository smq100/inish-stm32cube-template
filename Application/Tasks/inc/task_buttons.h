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

 @file    task_buttons.h
 @brief   Interface for supporting hardware buttons
 @author  Steve Quinlan
 @date    2026-March

******************************************************************************/

#ifndef __TASK_BUTTONS_H
#define __TASK_BUTTONS_H

#include "common.h"
#include "task.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  eButtonID_Nucleo,  //!< Prime button
  eButtonID_NUM      //!< Max is 32
} tButtonID;

//! Button states
typedef enum
{
  eButtonState_Pressed,   //!< Button is pressed
  eButtonState_Released,  //!< Button is released
  eButtonState_Hold,      //!< Button is being held
  eButtonState_Repeat,    //!< Button is repeat-clicked
  eButtonState_Click,     //!< Button click state
  eButtonState_NUM        //!< Max is 7
} tButtonState;

// Callback prototype for button events
typedef void(fnButtonEventCB)(tButtonID Button, tButtonState State);

/* Exported constants --------------------------------------------------------*/
#define BUTTON_BUFFER_SIZE 20   //!< Maximum number of button states in buffer
#define BUTTON_STATE_MASK 0x07  //!< Mask for getting button state (vs button ID)
#define BUTTON_STATE_BITS 3     //!< Button state in first 3 bits

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

//! Task function prototypes (See task.c/h)
fnTaskInit BUTTON_Init;
fnTaskExec BUTTON_Exec;
fnTaskShutdown BUTTON_Shutdown;
fnTaskTest BUTTON_Test;

bool BUTTON_GetButtonEvent(tButtonID* const Button, tButtonState* const State);
bool BUTTON_IsPressedRaw(tButtonID Button);
tButtonState BUTTON_GetState(tButtonID Button);
uint32_t BUTTON_GetChord(void);
bool BUTTON_IsStuck(void);
void BUTTON_RegisterCallback(fnButtonEventCB* Callback);

#ifdef BUTTON_PROTECTED

#endif /* BUTTON_PROTECTED */

#endif /* __TASK_BUTTONS_H */
