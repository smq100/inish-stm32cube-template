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

 @file    template.c
 @brief   Source file template
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#define TEMPLATE_PROTECTED

#include "main.h"
#include "template.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  bool IsActive;       ///< Current state of button GPIO state
  bool IsActive_Prev;  ///< Previous state of button GPIO state
} tModuleStatus;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "TEMPLATE";  //!< Name of the module. Used for debug logging
static uint32_t _Private = 1ul;           //!< Example of a private variable

/* Private function prototypes -----------------------------------------------*/

static void _BtnCallback(void);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief Template public function
 @param Param Description of the parameter
 @retval Description of the return value
*******************************************************************/
uint32_t TEMPLATE_Function(tTemplate_ID Param)
{
  UNUSED(Param);  // Mark parameter as unused to avoid compiler warning

  uint32_t local_var = _Private + 1;

  _BtnCallback();

  LOG_Write(eLogger_Sys, eLogLevel_Debug, _Module, false, "Param=%u, returning %u", Param, local_var);

  return local_var;
}

/* Private Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief Template private function
 @param None
 @retval None
********************************************************************/
static void _BtnCallback(void)
{
}

/* Protected Implementation -----------------------------------------------------*/

#ifdef TEMPLATE_PROTECTED

uint32_t TEMPLATE__Protected(uint32_t Param)
{
  uint32_t local_var;  // Local variable example
  local_var = ++Param;

  return local_var;
}

#endif /* TEMPLATE_PROTECTED */
