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

 @file    classb_startup.h
 @brief   Interface header file for Class B startup tests
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#ifndef __CLASSB_STARTUP_H
#define __CLASSB_STARTUP_H

#include "common.h"
#include "classb.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  eClassBStartItem_INIT1,
  eClassBStartItem_INIT2,
  eClassBStartItem_RAM,
  eClassBStartItem_CPU,
  eClassBStartItem_IWDG,
  eClassBStartItem_CRC,
  eClassBStartItem_CLK,
  eClassBStartItem_ADC,
  eClassBStartItem_UART,
  eClassBStartItem_FLOW,
  eClassBStartItem_COMPLETE,
  eClassBStartItem_NUM,
} tClassBStartItem;

typedef bool(fnClassBHandler_Startup)(bool Enabled, uint32_t* Status);

/* Exported constants --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* Exported vars ------------------------------------------------------------ */

/* Exported functions ------------------------------------------------------- */

fnClassBHandler_Startup Start_Init1;
fnClassBHandler_Startup Start_Init2;
fnClassBHandler_Startup Start_RAMTest;
fnClassBHandler_Startup Start_CPUTest;
fnClassBHandler_Startup Start_IWDGTest;
fnClassBHandler_Startup Start_CRCTest;
fnClassBHandler_Startup Start_CLKTest;
fnClassBHandler_Startup Start_ADCTest;
fnClassBHandler_Startup Start_FLOWTest;
fnClassBHandler_Startup Start_UARTTest;
fnClassBHandler_Startup Start_Complete;

#endif /* __CLASSB_STARTUP_H */
