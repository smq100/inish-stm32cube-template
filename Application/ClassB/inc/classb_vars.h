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

 @file    classb_vars.h
 @brief   Internal Class B variables header file
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#ifndef __CLASSB_VARS_H
#define __CLASSB_VARS_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  eClassBVar_RUN_FLOW_CNT_U32,  ///< Control Flow. Keep as the first entry so assembly code can use without offset
  eClassBVar_RUN_LSI_PERIOD_VALUE_V32,  ///< LSI period value for CLK testing
  eClassBVar_RUN_LSI_PERIOD_FLAG_V32,   ///< LSI period flag for CLK testing
  eClassBVar_RUN_RAMCHK_U32p,           ///< pointer to original values for runtime RAM transparent test
  eClassBVar_RUN_CRC32CHK_U32p,         ///< pointer to original CRC32
  eClassBVar_PCBTEMP_ADC_S16,           ///< last CPU temperature variable
  eClassBVar_VREF_ADC_S16,              ///< last CPU voltage-reference variable
  eClassBVar_LAST_ERROR_U32,            ///< last error code for Class B failures
  eClassBVar_NUM,                       ///< number of items in the Class B data structure
} tClassBVars;

typedef enum
{
  eClassBSet_Success,
  eClassBSet_Changed,
  eClassBSet_Error
} tClassB_SetResult;

/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

bool ClassB_VarsInit(void);
tDataValue ClassB_GetVar(tClassBVars Var);
tClassB_SetResult ClassB_SetVar(tClassBVars Var, tDataValue Value);

#endif /* __CLASSB_VARS_H */
