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

 @file    task_tech.h
 @brief   Interface for the serial Tech Port
 @author  Steve Quinlan
 @date    2026-March

 @note    See TECHMODE.md for a complete description of the technician interface

 ******************************************************************************/

#ifndef __TASK_TECH_H
#define __TASK_TECH_H

#include "common.h"
#include "task.h"
#include "task_serial.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

//! Task function prototypes (See task.c/h)
fnTaskInit TECH_Init;
fnTaskExec TECH_Exec;
fnTaskShutdown TECH_Shutdown;
fnTaskTest TECH_Test;

#ifdef TASK_TECH_PROTECTED

#define TECH_AUTH_REQUIRED true     ///< Require auth before processing other commands,
#define TECH_AUTH_USERPW_SZ 8       ///< Max size of the user and password for authentication
#define TECH_AUTH_USER "admin"      ///< The username required for login
#define TECH_AUTH_PASSWORD "pandt"  ///< The password required for login

#endif /* TASK_TECH_PROTECTED */

#endif /* __TASK_TECH_H */
