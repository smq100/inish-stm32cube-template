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

 @file    classb_startup.c
 @brief   Source file for Class B start-up tests
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#include "main.h"
#include "util.h"
#include "classb.h"
#include "classb_params.h"
#include "classb_vars.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

// Hold state across functions where registers and/or stack are destroyed
static uint32_t* _Hold;
static bool _Success;

/* Private function prototypes -----------------------------------------------*/
static uint32_t _RtcBkupRead(uint32_t BackupRegister);
static void _RtcBkupWrite(uint32_t BackupRegister, uint32_t Data);
static bool _ControlFlowCheckpoint(uint32_t Check);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Returns the Init1 test status (run every time)
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_Init1(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);
  UNUSED(Enabled);

  CRITICAL_SECTION_START();
  ClassB_SetVar(eClassBVar_RUN_LSI_PERIOD_VALUE_V32, (tDataValue){ .V32 = 0u });
  ClassB_SetVar(eClassBVar_RUN_LSI_PERIOD_FLAG_V32, (tDataValue){ .V32 = 0u });
  CRITICAL_SECTION_END();

  *Status = TEST_CLASSB_PASS;
  return true;
}

/*******************************************************************/
/*!
 @brief     Returns the Init2 test status (run only once unless 'always' flag set)
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_Init2(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);
  UNUSED(Enabled);

  ClassB_ControlFlowInit();

  *Status = TEST_CLASSB_PASS;
  return true;
}

/*******************************************************************/
/*!
 @brief     Returns the RAM test status (destructive)
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_RAMTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);
  UNUSED(Enabled);

  // Note: No control_flow calls here as RAM test is destructive
  // Note: Use static _Success var because stack gets corrupted

  uint32_t value = _RtcBkupRead(RTC_RAMCHECK_STATUS_REG);

  if (value == TEST_CLASSB_PASS)
  {
    // RAM test previously passed
    _Success = true;
  }
  else if (value == TEST_CLASSB_FAIL)
  {
    // RAM test previously failed
    _Success = false;
  }
  else
  {
    // RAM test not yet performed; disable interrupts and run it now
    // All memory content will be destroyed during the test
    CRITICAL_SECTION_START();
    _Success = asm_FullRamMarchC(RAM_START, RAM_END, BCKGRND);
    CRITICAL_SECTION_END();

    // Write status to non-volatile backup register to be read after reset
    _RtcBkupWrite(RTC_RAMCHECK_STATUS_REG, _Success ? TEST_CLASSB_PASS : TEST_CLASSB_FAIL);

    // Ensure RTC backup register write completes before forcing system reset
    __DSB();
    __ISB();

    // Clear control flow variables after RAM test
    ClassB_ControlFlowInit();

    // Can't do much here after destructive RAM test other than reboot
    // Can't call printf() as it uses RAM
    // Perform a system reset to reload the program and read back the result
    HAL_NVIC_SystemReset();
  }

  return _Success;
}

/*******************************************************************/
/*!
 @brief     Returns the CPU test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_CPUTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);
  UNUSED(Enabled);

  ClassB_ControlFlowEnter(FLOW_START_CPU1);

  // WARNING: all registers destroyed by asm_StartUpCPUTest() (including
  // preserved registers R4 to R11) while excluding stack pointer R13).
  // Make sure no critical data is stored in registers. We will cache
  // the 'passed' pointer in a static variable because registers will be
  // destroyed by the test. We also cannot return to the caller directly.
  _Hold = Status;
  if (asm_StartUpCPUTest() == CPUTEST_SUCCESS)
  {
    *_Hold = TEST_CLASSB_PASS;  // Write to copy of status pointer ('status' destroyed by test)
  }
  else
  {
    *_Hold = TEST_CLASSB_FAIL;  // Write to copy of status pointer ('status' destroyed by test)
  }

  ClassB_ControlFlowExit(FLOW_START_CPU1);

  printf("Rebooting after CPU test...\n\r");
  (void)HAL_Delay(100);
  HAL_NVIC_SystemReset();

  return (*_Hold == TEST_CLASSB_PASS);
}

/*******************************************************************/
/*!
 @brief     Returns the IWDG test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_IWDGTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);

  uint32_t reset_cause = GetResetCause();

  ClassB_ControlFlowEnter(FLOW_START_IWDG);

  if (!Enabled)
  {
    ClassB_ControlFlowEnter(FLOW_START_IWDG);
    printf("*** IWDG startup test disabled\n\r");
    *Status = TEST_CLASSB_PASS;
  }
  else if (*Status == TEST_CLASSB_NOINIT)
  {
    *Status = TEST_CLASSB_INPROGRESS;
    ClassB_ExpireIWDG();
    while (1)
    {
      // Wait for an independent watchdog reset
    }
  }
  else if (*Status == TEST_CLASSB_INPROGRESS)
  {
    if (reset_cause & RCC_CSR_IWDGRSTF)
    {
      // Independent watchdog reset detected; test passed
      *Status = TEST_CLASSB_PASS;
    }
    else
    {
      // Independent watchdog reset not detected; test failed
      *Status = TEST_CLASSB_FAIL;
    }
  }

  // Call 2 times to compensate for the timeout on first pass
  ClassB_ControlFlowExit(FLOW_START_IWDG);
  ClassB_ControlFlowExit(FLOW_START_IWDG);

  return (*Status == TEST_CLASSB_PASS);
}

/*******************************************************************/
/*!
 @brief     Returns the CRC test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_CRCTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);

  ClassB_ControlFlowEnter(FLOW_START_CRC);

  if (!Enabled)
  {
    printf("*** CRC startup test disabled\n\r");
    *Status = TEST_CLASSB_PASS;
  }
  else
  {
    uint32_t crc_result;

    bool is_debug_load = ClassB_LoadedViaDebugger();

    *Status = TEST_CLASSB_INPROGRESS;
    for (uint32_t index = 0; index < (uint32_t)ROM_SIZE_WORDS; index++)
    {
      CRC->DR = __REV(*((uint32_t*)ROM_START + index));
    }
    crc_result = CRC->DR;

    printf("Computed CRC=0x%08lX, Expected CRC=0x%08lX, debug=%s\n\r",
           crc_result,
           REF_CRC32,
           is_debug_load ? "true" : "false");
    if (is_debug_load)
    {
      // Debug load - CRC failure expected, don't fail the test
      *Status = TEST_CLASSB_PASS;
      printf("*** CRC check skipped (debug load)\n\r");
    }
    else if (crc_result == REF_CRC32)
    {
      *Status = TEST_CLASSB_PASS;
    }
    else
    {
      *Status = TEST_CLASSB_FAIL;
    }
  }

  ClassB_ControlFlowExit(FLOW_START_CRC);

  return (*Status == TEST_CLASSB_PASS);
}

/*******************************************************************/
/*!
 @brief     Returns the CLK test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_CLKTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);

  bool success = false;

  ClassB_ControlFlowEnter(FLOW_START_CLK1);

  tClockStatus clk_status = FREQ_OK;

  if (Enabled)
  {
    clk_status = ClassB_RunCLKTest(true);
  }
  else
  {
    ClassB_ControlFlowEnter(FLOW_START_CLK2);
    ClassB_ControlFlowExit(FLOW_START_CLK2);
  }

  switch (clk_status)
  {
    case MCO_START_TIM_FAIL:
      printf("MCO start-up failure\n\r");
      break;

    case SYSCLK_LOW:
      printf("System clock too low\n\r");
      break;

    case SYSCLK_HIGH:
      printf("System clock too high\n\r");
      break;

    case XCROSS_CONFIG_FAIL:
      printf("Clock Xcross measurement failure\n\r");
      break;

    case VARS_CORRUPT:
      printf("Clock test variable corruption detected\n\r");
      break;

    case FREQ_OK:
      success = true;
      break;

    default:
      printf("Abnormal exit from clock test\n\r");
  }

  *Status = success ? TEST_CLASSB_PASS : TEST_CLASSB_FAIL;

  ClassB_ControlFlowExit(FLOW_START_CLK1);

  return success;
}

/*******************************************************************/
/*!
 @brief     Returns the ADC test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_ADCTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);

  bool success = false;

  ClassB_ControlFlowEnter(FLOW_START_ADC);

  if (!Enabled)
  {
    success = true;
    printf("*** ADC startup test disabled\n\r");
  }
  else
  {
    success = ClassB_RunADCTest(true);
  }

  *Status = success ? TEST_CLASSB_PASS : TEST_CLASSB_FAIL;

  ClassB_ControlFlowExit(FLOW_START_ADC);

  return success;
}

/*******************************************************************/
/*!
 @brief     Returns the ADC test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_UARTTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);

  bool success = false;

  ClassB_ControlFlowEnter(FLOW_START_UART);

  if (!Enabled)
  {
    success = true;
    printf("*** UART startup test disabled\n\r");
  }
  else
  {
    // TODO: Implement UART startup test
    success = true;
  }

  *Status = success ? TEST_CLASSB_PASS : TEST_CLASSB_FAIL;

  ClassB_ControlFlowExit(FLOW_START_UART);

  return success;
}

/*******************************************************************/
/*!
 @brief     Returns the CLK test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_FLOWTest(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);
  UNUSED(Enabled);

  if (_ControlFlowCheckpoint(CHECKPOINT_START))
  {
    *Status = TEST_CLASSB_PASS;
  }
  else
  {
    *Status = TEST_CLASSB_FAIL;
  }

  return (*Status == TEST_CLASSB_PASS);
}

/*******************************************************************/
/*!
 @brief     Returns the CLK test status
 @param     enabled: Indicates whether the test is enabled
 @param     status: Pointer to store the test status
 @return    Status indicating whether the test passed (true) or failed (false)
 *******************************************************************/
bool Start_Complete(bool Enabled, uint32_t* Status)
{
  assert(Status != NULL);
  UNUSED(Enabled);

  *Status = TEST_CLASSB_PASS;
  return true;
}

/* Private Implementation ----------------------------------------------------*/

static uint32_t _RtcBkupRead(uint32_t BackupRegister)
{
  assert_param(IS_RTC_BKP(BackupRegister));

  uint32_t tmp = (uint32_t)&(RTC->BKP0R);
  tmp += (BackupRegister * 4u);

  return (*(__IO uint32_t*)tmp);
}

static void _RtcBkupWrite(uint32_t BackupRegister, uint32_t Data)
{
  assert_param(IS_RTC_BKP(BackupRegister));

  uint32_t tmp = (uint32_t)&(RTC->BKP0R);
  tmp += (BackupRegister * 4u);

  *(__IO uint32_t*)tmp = Data;
}

/******************************************************************************/
/**
 @brief  Verifies the consistency and value of control flow counters
 @param  check : check value of the positive counter
 @retval : true if the checkpoint is passed, false if failed
 *******************************************************************/
static bool _ControlFlowCheckpoint(uint32_t Check)
{
  bool success = true;

  tDataValue v = ClassB_GetVar(eClassBVar_RUN_FLOW_CNT_U32);
  success = (v.U32 == Check);

  return success;
}
