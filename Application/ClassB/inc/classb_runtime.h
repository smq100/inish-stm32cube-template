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

 @file    classb_runtime.h
 @brief   Interface header file for Class B runtime tests
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#ifndef __CLASSB_RUNTIME_H
#define __CLASSB_RUNTIME_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  eClassBRunStatus_NOT_STARTED,
  eClassBRunStatus_IN_PROGRESS,
  eClassBRunStatus_PASS,
  eClassBRunStatus_FAIL
} tClassBRunStatus;

typedef enum
{
  eClassBRunItem_CPU,
  eClassBRunItem_RAM,
  eClassBRunItem_CRC,
  eClassBRunItem_ADC,
  eClassBRunItem_CLK,
  eClassBRunItem_WDG,
  eClassBRunItem_Stack,
  eClassBRunItem_Flow,
  eClassBRunItem_NUM
} tClassBRunItem;

typedef tClassBRunStatus(fnClassBHandler_Runtime)(bool Enabled);

/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/

/* Exported vars ------------------------------------------------------------ */

/* Exported functions ------------------------------------------------------- */

bool Runtime_Init(void);

fnClassBHandler_Runtime Runtime_CPUTest;
fnClassBHandler_Runtime Runtime_RAMTest;
fnClassBHandler_Runtime Runtime_CRCTest;
fnClassBHandler_Runtime Runtime_ADCTest;
fnClassBHandler_Runtime Runtime_CLKTest;
fnClassBHandler_Runtime Runtime_WDGTest;
fnClassBHandler_Runtime Runtime_STKTest;
fnClassBHandler_Runtime Runtime_FLOWTest;

bool asm_RuntimeCPUTest(void);
bool asm_TranspRamMarchCXStep(uint32_t* Ram_addr, uint32_t* Buf_addr, uint32_t Pat);

#endif /* __CLASSB_RUNTIME_H */
