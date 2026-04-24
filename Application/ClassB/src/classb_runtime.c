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

 @file    classb_runtime.c
 @brief   Source file for Class B runtime tests
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#define CLASSB_PROTECTED

#include "main.h"
#include "test.h"
#include "task_app.h"
#include "classb.h"
#include "classb_runtime.h"
#include "classb_params.h"
#include "classb_vars.h"
#include "timer.h"
#include "eeprom_mcu.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

uint32_t CPURunFault = 0;  ///< Force CPU fault in asm_RuntimeCPUTest when non-zero
uint32_t RAMRunFault = 0;  ///< Force RAM fault in asm_TranspRamMarchCXStep when non-zero

/* Private variables ---------------------------------------------------------*/
static const char* _Module = "CBRT";  ///< Module name for debug logging

static bool _IsDebugLoad = false;  ///< Flag to detect debug vs flash load

/* Private function prototypes -----------------------------------------------*/

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Returns the initialization status
 @param     None
 @return    Status of the operation (always eClassBRunStatus_PASS)
 *******************************************************************/
bool Runtime_Init(void)
{
  tDataValue value;

  ClassB_ControlFlowInit();

  value.V32 = 0u;
  CRITICAL_SECTION_START();
  // Atomically initialize LSI period measurement variables
  ClassB_SetVar(eClassBVar_RUN_LSI_PERIOD_VALUE_V32, value);
  ClassB_SetVar(eClassBVar_RUN_LSI_PERIOD_FLAG_V32, value);

  // Atomically initialize stack overflow patterns
  extern volatile uint32_t StackOverFlowPtrn[4];
  StackOverFlowPtrn[0] = STACK_OVERFLOW_PATTERN_1;
  StackOverFlowPtrn[1] = STACK_OVERFLOW_PATTERN_2;
  StackOverFlowPtrn[2] = STACK_OVERFLOW_PATTERN_3;
  StackOverFlowPtrn[3] = STACK_OVERFLOW_PATTERN_4;
  CRITICAL_SECTION_END();

  for (int i = 0; i < eClassBRunItem_NUM; i++)
  {
    char name_buf[10];
    if (!ClassB_IsRuntimeTestEnabled((tClassBRunItem)i, name_buf))
    {
      LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, true, "ClassB runtime test '%s' disabled", name_buf);
    }
  }

  // Detect if loaded via debugger (used to skip CRC checks when debugging)
  _IsDebugLoad = ClassB_LoadedViaDebugger();

  // Start address of the RAM test has to be aligned to 16 address range
  // asm_TranspRamMarchCXStep accesses [R0-4 .. R0+16], so start at CLASS_B_START+4
  ClassB_SetVar(eClassBVar_RUN_RAMCHK_U32p, (tDataValue){ .U32 = ((uint32_t)CLASS_B_START + 4u) & 0xFFFFFFFCuL });

  // Initialize CRC check pointers
  ClassB_SetVar(eClassBVar_RUN_CRC32CHK_U32p, (tDataValue){ .U32 = (uint32_t)ROM_START });

  // Reset CRC peripheral accumulator for the runtime CRC test (does not affect the startup CRC test which runs earlier)
  // HAL_CRC_Init() does NOT reset CRC->DR; __HAL_CRC_DR_RESET writes the CRC_CR_RESET bit.
  __HAL_CRC_DR_RESET(&hCRC_APP);

  return true;
}

/*******************************************************************/
/*!
 @brief     Returns the CPU test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_CPUTest(bool Enabled)
{
  bool success = true;

  ClassB_ControlFlowEnter(FLOW_RUNTIME_CPU1);

  if (!Enabled)
  {
    // Mimic control flow from asm_RuntimeCPUTest
    ClassB_ControlFlowEnter(FLOW_RUNTIME_CPU2);
    ClassB_ControlFlowExit(FLOW_RUNTIME_CPU2);
  }
  else
  {
    if (ClassB__GetFault(eClassBRunItem_CPU))
    {
      CPURunFault = 1;
      ClassB__ClearFault(eClassBRunItem_CPU);
    }

    success = asm_RuntimeCPUTest();

    CPURunFault = 0;  // Clear fault injection flag (only fail once as the test completes)
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_CPU1);

  if (!success)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_CPU);
  }

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the RAM test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_RAMTest(bool Enabled)
{
  bool success = false;

  ClassB_ControlFlowEnter(FLOW_RUNTIME_RAM1);

  if (Enabled)
  {
    if (ClassB__GetFault(eClassBRunItem_RAM))
    {
      RAMRunFault = 1;
      ClassB__ClearFault(eClassBRunItem_RAM);
    }

    uint32_t* pRunTimeRamChk = (uint32_t*)ClassB_GetVar(eClassBVar_RUN_RAMCHK_U32p).U32;
    uint32_t* const classb_start = CLASS_B_START;
    uint32_t* const classb_end = CLASS_B_END;
    uint32_t* const min_block_start = classb_start + 1u;
    uint32_t* const max_block_start = classb_end - (RT_RAM_BLOCKSIZE - 1u);

    if (pRunTimeRamChk < min_block_start)
    {
      pRunTimeRamChk = min_block_start;
    }

    extern uint32_t RuntimeRamBuf[];
    if (pRunTimeRamChk > max_block_start)
    {
      // March test applied on the RAM Buffer itself
      // Disable interrupts during RAM test so that no access occurs to Class B RAM area
      CRITICAL_SECTION_START();
      success = asm_TranspRamMarchCXStep(&RuntimeRamBuf[0], &RuntimeRamBuf[0], BCKGRND);
      CRITICAL_SECTION_END();

      // Init next cycle of the transparent RAM test starting from the begin of the Class B area
      pRunTimeRamChk = min_block_start;
      ClassB_SetVar(eClassBVar_RUN_RAMCHK_U32p, (tDataValue){ .U32 = (uint32_t)pRunTimeRamChk });
    }
    else
    {
      // March test applied on Class B data area
      CRITICAL_SECTION_START();
      success = asm_TranspRamMarchCXStep(pRunTimeRamChk, &RuntimeRamBuf[1], BCKGRND);
      CRITICAL_SECTION_END();

      if (success)
      {
        // Prepare next Row Transparent RAM test
        pRunTimeRamChk += RT_RAM_BLOCKSIZE - (2u * RT_RAM_BLOCK_OVERLAP);
        ClassB_SetVar(eClassBVar_RUN_RAMCHK_U32p, (tDataValue){ .U32 = (uint32_t)pRunTimeRamChk });
      }
    }

    // Clear fault injection flag (only fail once)
    RAMRunFault = 0;
  }
  else
  {
    // Mimic control flow from asm_TranspRamMarchCXStep
    ClassB_ControlFlowEnter(FLOW_RUNTIME_RAM2);
    ClassB_ControlFlowExit(FLOW_RUNTIME_RAM2);

    // Test skipped, but still consider PASS since we're not actively failing
    success = true;
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_RAM1);

  if (!success)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_RAM);
  }

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the CRC test status
 @param     None
 @return    Status of the operation
 *******************************************************************/
tClassBRunStatus Runtime_CRCTest(bool Enabled)
{
  tClassBRunStatus status = eClassBRunStatus_PASS;

  ClassB_ControlFlowEnter(FLOW_RUNTIME_CRC);

  uint32_t* pRunCrc32Chk = (uint32_t*)ClassB_GetVar(eClassBVar_RUN_CRC32CHK_U32p).U32;

  if (!Enabled)
  {
    // Skip previously failed CRC test, but still do control flow updates
  }
  else if (ClassB__GetFault(eClassBRunItem_CRC))
  {
    // Force CRC failure. Error will be caught on the next pass
    CRC->DR = 0;
    pRunCrc32Chk = (uint32_t*)ROM_END;
    ClassB__ClearFault(eClassBRunItem_CRC);
  }
  else if (pRunCrc32Chk < (uint32_t*)ROM_END)
  {
    status = eClassBRunStatus_IN_PROGRESS;

    // Calculate how many words to check in this block
    uint32_t remaining_words = (uint32_t)((uint32_t*)ROM_END - pRunCrc32Chk);
    uint32_t words_to_check = (remaining_words < FLASH_BLOCK_WORDS) ? remaining_words : FLASH_BLOCK_WORDS;

    LOG_Write(eLogger_Sys,
              eLogLevel_Low,
              _Module,
              false,
              "CRC block 0x%08lX (%lu remaining, end=0x%08lX)",
              (uint32_t)pRunCrc32Chk,
              remaining_words,
              (uint32_t*)ROM_END);

    // Process one block of ROM
    for (uint32_t index = 0; index < words_to_check; index++)
    {
      CRC->DR = __REV(*(pRunCrc32Chk + index));
    }

    // Move to next block
    pRunCrc32Chk += words_to_check;
  }
  else
  {
    // All blocks checked - verify final CRC
    if (CRC->DR != *(uint32_t*)(&REF_CRC32))
    {
      if (_IsDebugLoad)
      {
        // Debug load - CRC failure expected, don't fail the test
        status = eClassBRunStatus_PASS;
        LOG_Write(eLogger_Sys,
                  eLogLevel_Low,
                  _Module,
                  false,
                  "CRC skipped (debug): 0x%08lX (expected=0x%08lX)",
                  CRC->DR,
                  *(uint32_t*)(&REF_CRC32));
      }
      else
      {
        // Production flash load - CRC failure is critical
        status = eClassBRunStatus_FAIL;
        LOG_Write(eLogger_Sys,
                  eLogLevel_Error,
                  _Module,
                  false,
                  "CRC FAIL: 0x%08lX (expected=0x%08lX)",
                  CRC->DR,
                  *(uint32_t*)(&REF_CRC32));
      }
    }
    else
    {
      status = eClassBRunStatus_PASS;
      LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "CRC check complete: PASS");
    }

    // Reset for next full cycle
    pRunCrc32Chk = (uint32_t*)ROM_START;

    __HAL_CRC_DR_RESET(&hcrc);  // HAL_CRC_Init() does NOT reset CRC->DR
  }

  ClassB_SetVar(eClassBVar_RUN_CRC32CHK_U32p, (tDataValue){ .U32 = (uint32_t)pRunCrc32Chk });

  ClassB_ControlFlowExit(FLOW_RUNTIME_CRC);

  if (status == eClassBRunStatus_FAIL)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_CRC);
  }

  return status;
}

/*******************************************************************/
/*!
 @brief     Returns the ADC test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_ADCTest(bool Enabled)
{
  bool success = true;

  ClassB_ControlFlowEnter(FLOW_RUNTIME_ADC);

  if (Enabled)
  {
    if (ClassB__GetFault(eClassBRunItem_ADC))
    {
      success = false;
      ClassB__ClearFault(eClassBRunItem_ADC);
    }
    else
    {
      success = ClassB_RunADCTest(false);
    }
  }

  if (!success)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_ADC);
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_ADC);

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the CLK test status. Includes minor filtering of transient errors
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_CLKTest(bool Enabled)
{
  ClassB_ControlFlowEnter(FLOW_RUNTIME_CLK1);

  tClockStatus clck_sts;
  uint32_t current_tick = TIMER_GetTick();

  // If test disabled, exit immediately
  if (!Enabled)
  {
    ClassB_ControlFlowEnter(FLOW_RUNTIME_CLK2);
    ClassB_ControlFlowExit(FLOW_RUNTIME_CLK2);
    clck_sts = FREQ_OK;
  }
  else if (current_tick > (UINT32_MAX - 200u))  // Within 200ms of overflow
  {
    // We're too close to tick overflow
    // Skip this test cycle; will run again after tick wraps
    // Only occurs once every ~49 days so this acceptable
    clck_sts = FREQ_OK;
  }
  else
  {
    clck_sts = ClassB_RunCLKTest(false);
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_CLK1);

  if (clck_sts != FREQ_OK)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_CLK);
  }

  return (clck_sts == FREQ_OK) ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the WDG test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_WDGTest(bool Enabled)
{
  bool success = true;

  ClassB_ControlFlowEnter(FLOW_RUNTIME_WDG);

  if (Enabled)
  {
    if (ClassB__GetFault(eClassBRunItem_WDG))
    {
      success = false;
      ClassB__ClearFault(eClassBRunItem_WDG);
    }
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_WDG);

  if (!success)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_WDG);
  }

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the Stack test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_STKTest(bool Enabled)
{
  bool success = true;
  extern volatile uint32_t StackOverFlowPtrn[4];

  ClassB_ControlFlowEnter(FLOW_RUNTIME_STACK);

  if (Enabled)
  {
    if (ClassB__GetFault(eClassBRunItem_Stack))
    {
      // Force stack overflow by corrupting the pattern
      StackOverFlowPtrn[0] = 0xDEADC0DE;
    }

    // Atomically read stack overflow patterns
    CRITICAL_SECTION_START();
    uint32_t pat0 = StackOverFlowPtrn[0];
    uint32_t pat1 = StackOverFlowPtrn[1];
    uint32_t pat2 = StackOverFlowPtrn[2];
    uint32_t pat3 = StackOverFlowPtrn[3];
    CRITICAL_SECTION_END();

    if (pat0 != STACK_OVERFLOW_PATTERN_1)
    {
      success = false;
    }
    else if (pat1 != STACK_OVERFLOW_PATTERN_2)
    {
      success = false;
    }
    else if (pat2 != STACK_OVERFLOW_PATTERN_3)
    {
      success = false;
    }
    else if (pat3 != STACK_OVERFLOW_PATTERN_4)
    {
      success = false;
    }

    if (ClassB__GetFault(eClassBRunItem_Stack))
    {
      // Clear fault injection flag (only fail once)
      StackOverFlowPtrn[0] = STACK_OVERFLOW_PATTERN_1;
      ClassB__ClearFault(eClassBRunItem_Stack);
    }
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_STACK);

  if (!success)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_Stack);
  }

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the Stack test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_APPTest(bool Enabled)
{
  bool success = true;

  ClassB_ControlFlowEnter(FLOW_RUNTIME_APP);

  if (Enabled)
  {
    success = APP_ClassBTest();
  }

  ClassB_ControlFlowExit(FLOW_RUNTIME_APP);

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}

/*******************************************************************/
/*!
 @brief     Returns the Flow test status
 @param     None
 @return    Status of the operation (eClassBRunStatus_PASS or eClassBRunStatus_FAIL)
 *******************************************************************/
tClassBRunStatus Runtime_FLOWTest(bool Enabled)
{
  bool success = true;

  if (ClassB__GetFault(eClassBRunItem_Flow))
  {
    // Force control flow failure
    tDataValue v;
    v = ClassB_GetVar(eClassBVar_RUN_FLOW_CNT_U32);
    v.U32 += 1;
    ClassB_SetVar(eClassBVar_RUN_FLOW_CNT_U32, v);

    ClassB__ClearFault(eClassBRunItem_Flow);
  }

  if (!Enabled)
  {
    // Test skipped, but still consider PASS since we're not actively failing
  }
  else if (ClassB_GetVar(eClassBVar_RUN_FLOW_CNT_U32).U32 != CHECKPOINT_RUNTIME)
  {
    success = false;
  }

  if (!success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Control flow test failed");
  }

  // Re-initialize control flow for next cycle
  ClassB_ControlFlowInit();

  if (!success)
  {
    // Persist error counts
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_ClassBError_Flow);
  }

  return success ? eClassBRunStatus_PASS : eClassBRunStatus_FAIL;
}
