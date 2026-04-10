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

 @file    classb.h
 @brief   Header file for ClassB functionality
 @author  Steve Quinlan
          Adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#ifndef __CLASSB_H
#define __CLASSB_H

#include "common.h"
#include "classb_startup.h"
#include "classb_runtime.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
  TEST_ONGOING,
  MCO_START_TIM_FAIL,
  SYSCLK_LOW,
  SYSCLK_HIGH,
  XCROSS_CONFIG_FAIL,
  VARS_CORRUPT,
  FREQ_OK
} tClockStatus;

//! Command handler function prototypes

/* Exported constants --------------------------------------------------------*/

#define FLOW_START_CPU1 ((uint32_t)1)
#define FLOW_START_CPU2 ((uint32_t)3)  // Do not modify; hard coded in assembly file
#define FLOW_START_IWDG ((uint32_t)5)
#define FLOW_START_CLK1 ((uint32_t)9)
#define FLOW_START_CLK2 ((uint32_t)11)
#define FLOW_START_CRC ((uint32_t)13)
#define FLOW_START_ADC ((uint32_t)15)
#define FLOW_START_ADC ((uint32_t)15)
#define FLOW_START_UART ((uint32_t)17)

#define FLOW_RUNTIME_CPU1 ((uint32_t)1)
#define FLOW_RUNTIME_CPU2 ((uint32_t)3)  // Do not modify; hard coded in assembly file
#define FLOW_RUNTIME_RAM1 ((uint32_t)5)
#define FLOW_RUNTIME_RAM2 ((uint32_t)7)  // Do not modify; hard coded in assembly file
#define FLOW_RUNTIME_CRC ((uint32_t)9)
#define FLOW_RUNTIME_WDG ((uint32_t)11)
#define FLOW_RUNTIME_CLK1 ((uint32_t)13)
#define FLOW_RUNTIME_CLK2 ((uint32_t)15)
#define FLOW_RUNTIME_ADC ((uint32_t)17)
#define FLOW_RUNTIME_STACK ((uint32_t)19)
#define FLOW_RUNTIME_APP ((uint32_t)21)

#define TEST_CLASSB_NOINIT ((uint32_t)0x33333333)
#define TEST_CLASSB_INPROGRESS ((uint32_t)0x55555555)
#define TEST_CLASSB_PASS ((uint32_t)0xAAAAAAAA)
#define TEST_CLASSB_FAIL ((uint32_t)0xCCCCCCCC)
#define TEST_CLASSB_DISABLED ((uint32_t)0xDDDDDDDD)

// clang-format off
#define CHECKPOINT_START    \
  ( FLOW_START_CPU1 +       \
    FLOW_START_CPU2 +       \
    (2 * FLOW_START_IWDG) + \
    FLOW_START_CRC +        \
    FLOW_START_CLK1 +       \
    FLOW_START_CLK2 +       \
    FLOW_START_ADC +        \
    FLOW_START_UART)

#define CHECKPOINT_RUNTIME  \
  ( FLOW_RUNTIME_CPU1 +     \
    FLOW_RUNTIME_CPU2 +     \
    FLOW_RUNTIME_CRC +      \
    FLOW_RUNTIME_RAM1 +     \
    FLOW_RUNTIME_RAM2 +     \
    FLOW_RUNTIME_ADC +      \
    FLOW_RUNTIME_CLK1 +     \
    FLOW_RUNTIME_CLK2 +     \
    FLOW_RUNTIME_WDG +      \
    FLOW_RUNTIME_STACK +    \
    FLOW_RUNTIME_APP)
// clang-format on

/* Exported macros ------------------------------------------------------------*/

/* Exported vars ------------------------------------------------------------ */

/* Exported functions ------------------------------------------------------- */

bool ClassB_InitVars(bool Force);
void ClassB_DoStartUpTests(bool PreInit);
bool ClassB_IsStartupPass(tClassBStartItem Test);
bool ClassB_IsAllStartupPass(void);
void ClassB_ExpireIWDG(void);
bool ClassB_IsTestingIWDG(void);
void ClassB_PrintStatus(void);
bool ClassB_LoadedViaDebugger(void);
bool ClassB_IsRuntimeTestEnabled(tClassBRunItem Test, char* Name);
void ClassB_Fail_Startup(tClassBStartItem Test);
void ClassB_Fail_Runtime(tClassBRunItem Test);
void ClassB_ControlFlowInit(void);
void ClassB_ControlFlowEnter(uint32_t A);
void ClassB_ControlFlowExit(uint32_t A);
const char* ClassB_GetRuntimeName(tClassBRunItem Test);

tClockStatus ClassB_RunCLKTest(bool Startup);
bool ClassB_RunADCTest(bool Startup);

void ClassB_PrintErrorCounts(void);

// Assembler function prototypes
bool asm_RuntimeCPUTest(void);
bool asm_TranspRamMarchCXStep(uint32_t* ram_addr, uint32_t* buf_addr, uint32_t pat);
bool asm_StartUpCPUTest(void);
bool asm_FullRamMarchC(uint32_t* beg, uint32_t* end, uint32_t pat);

/* Protected functions ------------------------------------------------------ */

#ifdef CLASSB_PROTECTED
bool ClassB__SetFault(tClassBRunItem Test);
bool ClassB__ClearFault(tClassBRunItem Test);
bool ClassB__GetFault(tClassBRunItem Test);
void ClassB__IC_Callback(TIM_HandleTypeDef* Htim);

#endif /* CLASSB_PROTECTED */

#endif /* __CLASSB_H */
