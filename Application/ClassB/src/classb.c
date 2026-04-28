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

 @file    classb.c
 @brief   Source file for ClassB functionality
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#define CLASSB_PROTECTED

#include "stm32l1xx_ll_adc.h"

#include "main.h"
#include "task.h"
#include "task_system.h"
#include "task_led.h"
#include "classb.h"
#include "classb_startup.h"
#include "classb_vars.h"
#include "classb_params.h"
#include "eeprom_mcu.h"
#include "watchdog.h"
#include "rtc_app.h"
#include "log.h"
#include "util.h"
#include "timer.h"
#include "dma.h"
#include "adc.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  bool Enabled;                      ///!< Indicates whether the startup test is enabled
  bool PreInit;                      ///!< Indicates whether the test should run during pre-initialization
  bool Always;                       ///!< Indicates whether the test should always run regardless of previous results
  fnClassBHandler_Startup* Handler;  ///!< Function pointer to the test handler
  const char* Name;                  ///!< Name of the startup test
} tClassbStartConfig;

typedef struct
{
  bool Enabled;      ///!< Indicates whether the run test is enabled
  const char* Name;  ///!< Name of the run test
} tClassbRunConfig;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "CLSB";  //!< Module Name for debug logging

static bool _FaultInjection[eClassBRunItem_NUM] = { false };  //!< Fault injection flags

// clang-format off
// Order must match tClassBStartItem enum
static const tClassbStartConfig _StartConfig[] = {
  { true, true,  true,  Start_Init1,     "sInit1" },
  { true, true,  false, Start_Init2,     "sInit2" },
  { true, true,  false, Start_RAMTest,   "sRAM" },
  { true, true,  false, Start_CPUTest,   "sCPU" },

#ifndef TEST__DISABLE_IWDG
  { true, false, false, Start_IWDGTest,  "sIWDG" },
#else
  { false, false, false, Start_IWDGTest, "sIWDG" },
#endif

  { true, false, false, Start_CRCTest,   "sCRC" },
  { true, false, false, Start_CLKTest,   "sCLK" },
  { true, false, false, Start_ADCTest,   "sADC" },
  { true, false, false, Start_UARTTest,  "sUART" },
  { true, false, false, Start_FLOWTest,  "sFlow" },
  { true, false, true,  Start_Complete,  "sComplete" },
};

// Order must match tClassBRunItem enum
static const tClassbRunConfig _RunConfig[] = {
  { true, "rCPU" },
  { true, "rRAM" },
  { true, "rCRC" },
  { true, "rADC" },
  { true, "rCLK" },

#ifndef TEST__DISABLE_IWDG
  { true, "rWDG" },
#else
  { false, "rWDG" },
#endif

  { true, "rStack" },
  { true, "rApp" },
  { true, "rFlow" },
};
// clang-format on

static_assert(sizeof(_StartConfig) / sizeof(tClassbStartConfig) == eClassBStartItem_NUM, "start size mismatch");
static_assert(sizeof(_RunConfig) / sizeof(tClassbRunConfig) == eClassBRunItem_NUM, "run size mismatch");

// Startup test status (stored in .noinit RAM section so preserved across sw resets)
static uint32_t _StartTestStatus[eClassBStartItem_NUM] __attribute__((section(".noinit")));

/* Private function prototypes -----------------------------------------------*/
/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes ClassB variables
 @param     force : true to force initialization of variables, false to only initialize if not already set
             (e.g. from previous classb startup test before reset)
 @return    Status indicating whether variables were initialized (true) or
            preserved from previous run (false)
 *******************************************************************/
bool ClassB_InitVars(bool Force)
{
  bool reset = false;
  uint32_t reset_cause = Force ? 0 : GetResetCause();

  if (reset_cause & RCC_CSR_SFTRSTF)
  {
    // Software reset detected; ignore
  }
  else if (reset_cause & RCC_CSR_IWDGRSTF)
  {
    // IWDG reset detected; ignore
  }
  else
  {
    for (int i = 0; i < eClassBStartItem_NUM; i++)
    {
      _StartTestStatus[i] = TEST_CLASSB_NOINIT;
    }

    for (int i = 0; i < eClassBRunItem_NUM; i++)
    {
      _FaultInjection[i] = false;
    }

    // Clear RAM test status in RTC backup register
    RTC_HandleTypeDef hrtc = { 0 };
    hrtc.Instance = RTC;
    (void)HAL_RTCEx_BKUPWrite(&hrtc, RTC_RAMCHECK_STATUS_REG, TEST_CLASSB_NOINIT);

    reset = true;
  }

  // Make sure all tests have a valid state regardless of reset cause
  for (int i = 0; i < eClassBStartItem_NUM; i++)
  {
    if (!_StartConfig[i].Enabled)
    {
      _StartTestStatus[i] = TEST_CLASSB_DISABLED;
    }
    else if (_StartTestStatus[i] == TEST_CLASSB_NOINIT)
    {
      // Ignore
    }
    else if (_StartTestStatus[i] == TEST_CLASSB_INPROGRESS)
    {
      // Ignore
    }
    else if (_StartTestStatus[i] == TEST_CLASSB_PASS)
    {
      // Ignore
    }
    else if (_StartTestStatus[i] == TEST_CLASSB_FAIL)
    {
      // Ignore
    }
    else
    {
      _StartTestStatus[i] = TEST_CLASSB_NOINIT;
    }
  }

  return reset;
}

/*******************************************************************/
/*!
 @brief     Performs ClassB startup tests
 @param     PreInit : true to run pre-initialization tests
                      false to run post-initialization tests
 @return    None
*******************************************************************/
void ClassB_DoStartUpTests(bool PreInit)
{
  for (tClassBStartItem i = 0; i < eClassBStartItem_NUM; i++)
  {
    if (_StartConfig[i].PreInit != PreInit)
    {
      // Skip tests not matching the preInit specification
    }
    else if (_StartTestStatus[i] == TEST_CLASSB_PASS && !_StartConfig[i].Always)
    {
      printf("Skipping startup test previously PASSED: %s\n\r", _StartConfig[i].Name);
    }
    else if (_StartTestStatus[i] == TEST_CLASSB_FAIL && !_StartConfig[i].Always)
    {
      printf("Skipping startup test previously FAILED: %s\n\r", _StartConfig[i].Name);
      break;
    }
    else
    {
      printf("Running startup test: %s (pre=%d)\n\r", _StartConfig[i].Name, PreInit);
      bool success = _StartConfig[i].Handler(_StartConfig[i].Enabled, &_StartTestStatus[i]);
      _StartTestStatus[i] = success ? TEST_CLASSB_PASS : TEST_CLASSB_FAIL;

      if (success)
      {
        printf("Startup test PASSED: %s\n\r", _StartConfig[i].Name);
      }
      else
      {
        ClassB_Fail_Startup(i);  // Does not return. TODO?
      }
    }
  }
}

/*******************************************************************/
/*!
 @brief   Returns whether specified startup test passed
 @param   test : Test to check
 @return  true if test passed, false otherwise
*******************************************************************/
bool ClassB_IsStartupPass(tClassBStartItem Test)
{
  if (Test >= eClassBStartItem_NUM)
  {
    return false;
  }
  return (_StartTestStatus[Test] == TEST_CLASSB_PASS);
}

/*******************************************************************/
/*!
 @brief   Returns whether all startup tests passed
 @param
 @return  true if all tests passed, false otherwise
*******************************************************************/
bool ClassB_IsAllStartupPass(void)
{
  bool success = true;
  for (tClassBStartItem i = 0; i < eClassBStartItem_NUM; i++)
  {
    if (_StartConfig[i].Enabled && (_StartTestStatus[i] != TEST_CLASSB_PASS))
    {
      success = false;
      break;
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief   Returns the status of a specific startup test
 @param   Test : Test to check
 @return  Status of the specified test
*******************************************************************/
uint32_t ClassB_GetStartupStatus(tClassBStartItem Test)
{
  uint32_t status = TEST_CLASSB_NOINIT;

  if (Test < eClassBStartItem_NUM)
  {
    status = _StartTestStatus[Test];
  }

  return status;
}

/*******************************************************************/
/*!
 @brief   Prints the status of all startup tests
 @param   None
 @return  None
*******************************************************************/
void ClassB_PrintStatus(void)
{
  for (int i = 0; i < eClassBStartItem_NUM; i++)
  {
    const char* status_str;
    switch (_StartTestStatus[i])
    {
      case TEST_CLASSB_NOINIT:
        status_str = "PENDING";
        break;
      case TEST_CLASSB_INPROGRESS:
        status_str = "IN PROGRESS";
        break;
      case TEST_CLASSB_PASS:
        status_str = "PASS";
        break;
      case TEST_CLASSB_FAIL:
        status_str = "FAIL";
        break;
      case TEST_CLASSB_DISABLED:
        status_str = "DISABLED";
        break;
      default:
        status_str = "UNKNOWN";
        break;
    }

    LOG_WriteDirect(
      eLogger_Sys, eLogLevel_High, _Module, false, "Startup test '%s': %s", _StartConfig[i].Name, status_str);
  }
}

/*******************************************************************/
/*!
 @brief   Detect if firmware was loaded via debugger vs flash programmer
 @param   None
 @return  true if loaded via debugger, false if flashed normally
*******************************************************************/
bool ClassB_LoadedViaDebugger(void)
{
  bool debugger_present;

  // Check if CRC reference is erased flash pattern. Linker sets REF_CRC32 to 0xFFFFFFFF
  uint32_t crc_value = *(uint32_t*)(&REF_CRC32);
  debugger_present = (crc_value == 0xFFFFFFFF);

  return debugger_present;
}

/*******************************************************************/
/*!
 @brief   Checks if a runtime test is Enabled
 @param   test : Test to check
 @return  true if test is Enabled, false otherwise
*******************************************************************/
bool ClassB_IsRuntimeTestEnabled(tClassBRunItem Test, char* Name)
{
  bool enabled = false;

  if (Test < eClassBRunItem_NUM)
  {
    enabled = _RunConfig[Test].Enabled;

    if (Name != NULLPTR)
    {
      strcpy(Name, _RunConfig[Test].Name);
    }
  }

  return enabled;
}

/*******************************************************************/
/*!
 @brief   Executed in case of failure is detected by one of self-test startup routines
 @param   test : Test that failed
 @return  None
*******************************************************************/
void ClassB_Fail_Startup(tClassBStartItem Test)
{
  printf("Startup test '%s' FAILED. Halting execution.\n\r", _StartConfig[Test].Name);

  while (true)
  {
#ifndef TEST__DISABLE_IWDG
    HAL_IWDG_Refresh(&hIWDG_APP);
#endif

    HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
    HAL_Delay(100);
  }
}

/*******************************************************************/
/*!
 @brief   Executed in case of failure is detected by one of self-test runtime routines
 @param   test : Test that failed
 @return  None
*******************************************************************/
void ClassB_Fail_Runtime(tClassBRunItem Test)
{
  LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "Runtime test '%s' failed.", _RunConfig[Test].Name);

  // Flash the LED based on the test that failed. test+1 so test 0 pulses once.
#ifndef HW_IS_NUCLEO
  LED_Pulse(eLED_Error, 1.0f, -1, (int32_t)(Test + 1), "");
#else
  SYSTEM_SetStatusLED(eStatusLED_Error, (int8_t)(Test + 1));
#endif
}

/*******************************************************************/
/*!
 @brief   Executed in case of entering a runtime test. Used for control flow testing
 @return  None
*******************************************************************/
void ClassB_ControlFlowInit(void)
{
  ClassB_SetVar(eClassBVar_RUN_FLOW_CNT_U32, (tDataValue){ .U32 = 0u });
}

/*******************************************************************/
/*!
 @brief   Executed in case of entering a runtime test. Used for control flow testing
 @param   a : value to add to control flow counter
 @return  None
*******************************************************************/
void ClassB_ControlFlowEnter(uint32_t A)
{
  extern tDataValue ClassBData[];
  ClassBData[eClassBVar_RUN_FLOW_CNT_U32].U32 += A;
}

/*******************************************************************/
/*!
 @brief   Executed in case of exiting a runtime test. Used for control flow testing
 @param   a : value to subtract from control flow counter
 @return  None
*******************************************************************/
void ClassB_ControlFlowExit(uint32_t A)
{
  extern tDataValue ClassBDataInv[];
  ClassBDataInv[eClassBVar_RUN_FLOW_CNT_U32].U32 -= A;
}

/*******************************************************************/
/*!
 @brief   Returns the name of a runtime test
 @param   test : Test to get the name of
 @return  Name of the runtime test
*******************************************************************/
const char* ClassB_GetRuntimeName(tClassBRunItem Test)
{
  const char* name = NULLPTR;

  if (Test < eClassBRunItem_NUM)
  {
    name = _RunConfig[Test].Name;
  }

  return name;
}

/*******************************************************************/
/*!
 @brief   Runs the clock test, which measures the LSI period using
          TIM10 input capture and checks if it's within expected limits
 @param   startup : Indicates if the test is run during startup or runtime
 @return  Status of the clock test
*******************************************************************/
tClockStatus ClassB_RunCLKTest(bool Startup)
{
  tClockStatus clck_sts = TEST_ONGOING;

  ClassB_ControlFlowEnter(Startup ? FLOW_START_CLK2 : FLOW_RUNTIME_CLK2);

  __HAL_TIM_CLEAR_FLAG(&hTIM_CLKTEST, TIM_FLAG_CC1);
  if (HAL_TIM_IC_Start_IT(&hTIM_CLKTEST, TIM_CLKTEST_CH) == HAL_OK)
  {
    // Wait for two subsequent LSI periods measurements
    // Use sequence counter approach to ensure we capture fresh measurements
    // and avoid race conditions with the timer interrupt updating the flags

    uint32_t start_count = ClassB_GetVar(eClassBVar_RUN_LSI_PERIOD_FLAG_V32).V32;

    // Wait for first new measurement (flag increment)
    uint32_t timeout = TIMER_GetTick() + 150u;  // 150 ms timeout
    while (ClassB_GetVar(eClassBVar_RUN_LSI_PERIOD_FLAG_V32).V32 == start_count)
    {
      if (TIMER_GetTick() > timeout)
      {
        LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "CLK test timeout waiting for LSI period 1");
        clck_sts = MCO_START_TIM_FAIL;
        break;
      }
    }

    if (clck_sts == TEST_ONGOING)
    {
      start_count = ClassB_GetVar(eClassBVar_RUN_LSI_PERIOD_FLAG_V32).V32;

      // Wait for second new measurement (another flag increment)
      timeout = TIMER_GetTick() + 150u;  // 150 ms timeout
      while (ClassB_GetVar(eClassBVar_RUN_LSI_PERIOD_FLAG_V32).V32 == start_count)
      {
        if (TIMER_GetTick() > timeout)
        {
          LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "CLK test timeout waiting for LSI period 2");
          clck_sts = MCO_START_TIM_FAIL;
          break;
        }
      }
    }

    uint32_t period = ClassB_GetVar(eClassBVar_RUN_LSI_PERIOD_VALUE_V32).V32;

    // LSI_MEASURE_PRESCSALER must the same as TIM10_CLK_TEST_FREQ IC prescaler
    const uint32_t limit_low = SYS_LIMIT_LOW(SYSCLK_AT_RUN);
    const uint32_t limit_high = SYS_LIMIT_HIGH(SYSCLK_AT_RUN);

    if (_FaultInjection[eClassBRunItem_CLK] && !Startup)
    {
      // Force clock failure by injecting out-of-range value
      period = limit_high + 1000uL;
      _FaultInjection[eClassBRunItem_CLK] = false;
    }

    // Assign status
    if (clck_sts == MCO_START_TIM_FAIL)
    {
      // Already failed to get LSI periods
    }
    else if (period < limit_low)
    {
      // Below expected
      LOG_Write(
        eLogger_Sys, eLogLevel_Error, _Module, true, "CLK test FAIL: period %lu below limit %lu", period, limit_low);
      clck_sts = SYSCLK_LOW;
    }
    else if (period > limit_high)
    {
      LOG_Write(
        eLogger_Sys, eLogLevel_Error, _Module, true, "CLK test FAIL: period %lu above limit %lu", period, limit_high);
      clck_sts = SYSCLK_HIGH;
    }
    else
    {
      LOG_Write(eLogger_Sys,
                eLogLevel_Low,
                _Module,
                true,
                "CLK test PASS: period %lu within limits %lu/%lu",
                period,
                limit_low,
                limit_high);

      clck_sts = FREQ_OK;
    }
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "CLK test FAIL: unable to start timer");
    clck_sts = MCO_START_TIM_FAIL;
  }

  // Stop timer after test completes
  HAL_TIM_IC_Stop_IT(&hTIM_CLKTEST, TIM_CLKTEST_CH);

  ClassB_ControlFlowExit(Startup ? FLOW_START_CLK2 : FLOW_RUNTIME_CLK2);

  return clck_sts;
}

/*******************************************************************/
/*!
 @brief   Run ADC test
 @param   startup : Indicates if the test is run during startup or runtime
 @return  Status of the test
*******************************************************************/
bool ClassB_RunADCTest(bool Startup)
{
  bool success = true;
  uint16_t vref_raw = 0u;
  uint16_t temp_raw = 0u;

  if (Startup)
  {
    MX_ADC_Init();
  }

  // Stop any active DMA conversion before reconfiguring.
  // This does not disturb the DMA peripheral itself or the DMA handle link.
  (void)HAL_ADC_Stop_DMA(&hADC_APP);

  // Switch temporarily to single-channel polling mode so we can read VREFINT and
  // TEMPSENSOR without touching any TGEN state or DMA buffers.
  // HAL_ADC_Init() skips MspInit (and therefore does not touch the DMA link) when
  // the handle state is not HAL_ADC_STATE_RESET, which is the case here.
  hADC_APP.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hADC_APP.Init.NbrOfConversion = 1u;
  hADC_APP.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

  if (HAL_ADC_Init(&hADC_APP) != HAL_OK)
  {
    if (Startup)
    {
      printf("ADC test FAIL: unable to reconfigure ADC\r\n");
    }
    else
    {
      LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "ADC test FAIL: unable to reconfigure ADC");
    }
    success = false;
  }

  // Read VREFINT
  if (success)
  {
    ADC_ChannelConfTypeDef ch = { 0 };
    ch.Channel = ADC_CHANNEL_VREFINT;
    ch.Rank = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_384CYCLES;

    if (HAL_ADC_ConfigChannel(&hADC_APP, &ch) != HAL_OK || HAL_ADC_Start(&hADC_APP) != HAL_OK ||
        HAL_ADC_PollForConversion(&hADC_APP, 10u) != HAL_OK)
    {
      if (Startup)
      {
        printf("ADC test FAIL: VREFINT read error\r\n");
      }
      else
      {
        LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "ADC test FAIL: VREFINT read error");
      }
      success = false;
    }
    else
    {
      vref_raw = (uint16_t)HAL_ADC_GetValue(&hADC_APP);
      (void)HAL_ADC_Stop(&hADC_APP);
    }
  }

  // Read TEMPSENSOR
  if (success)
  {
    ADC_ChannelConfTypeDef ch = { 0 };
    ch.Channel = ADC_CHANNEL_TEMPSENSOR;
    ch.Rank = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_384CYCLES;

    if (HAL_ADC_ConfigChannel(&hADC_APP, &ch) != HAL_OK || HAL_ADC_Start(&hADC_APP) != HAL_OK ||
        HAL_ADC_PollForConversion(&hADC_APP, 10u) != HAL_OK)
    {
      if (Startup)
      {
        printf("ADC test FAIL: TEMPSENSOR read error\r\n");
      }
      else
      {
        LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, true, "ADC test FAIL: TEMPSENSOR read error");
      }
      success = false;
    }
    else
    {
      temp_raw = (uint16_t)HAL_ADC_GetValue(&hADC_APP);
      (void)HAL_ADC_Stop(&hADC_APP);
    }
  }

  // Restore the full 5-channel scan+DMA configuration unconditionally so that
  // TGEN ADC DMA reads work correctly regardless of test outcome.
  MX_ADC_Init();

  // Validate
  if (success)
  {
    int16_t vdda_mv = CalcVDDA_mv(vref_raw);
    int16_t temp_decic = CalcCoreTemp_Cx10(temp_raw, vref_raw);

    if (vdda_mv < CLASSB_ADC_VREF_LOW_MV || vdda_mv > CLASSB_ADC_VREF_HIGH_MV)
    {
      if (Startup)
      {
        printf("ADC test FAIL: Vref %d mV out of range [%d, %d]\r\n",
               vdda_mv,
               CLASSB_ADC_VREF_LOW_MV,
               CLASSB_ADC_VREF_HIGH_MV);
      }
      else
      {
        LOG_Write(eLogger_Sys,
                  eLogLevel_Error,
                  _Module,
                  true,
                  "ADC test FAIL: Vref %d mV out of range [%d, %d]",
                  vdda_mv,
                  CLASSB_ADC_VREF_LOW_MV,
                  CLASSB_ADC_VREF_HIGH_MV);
      }

      success = false;
    }

    if (temp_decic < CLASSB_ADC_TEMP_LOW_DECIC || temp_decic > CLASSB_ADC_TEMP_HIGH_DECIC)
    {
      if (Startup)
      {
        printf("ADC test FAIL: MCU temp %d deci-C out of range [%d, %d]\r\n",
               temp_decic,
               CLASSB_ADC_TEMP_LOW_DECIC,
               CLASSB_ADC_TEMP_HIGH_DECIC);
      }
      else
      {
        LOG_Write(eLogger_Sys,
                  eLogLevel_Error,
                  _Module,
                  true,
                  "ADC test FAIL: MCU temp %d deci-C out of range [%d, %d]",
                  temp_decic,
                  CLASSB_ADC_TEMP_LOW_DECIC,
                  CLASSB_ADC_TEMP_HIGH_DECIC);
      }

      success = false;
    }

    if (success)
    {
      LOG_Write(eLogger_Sys,
                eLogLevel_Low,
                _Module,
                false,
                "ADC test PASS: vref_raw=%u temp_raw=%u Vref=%.2fV MCU temp=%.1fC",
                vref_raw,
                temp_raw,
                vdda_mv / 1000.0f,
                temp_decic / 10.0f);
    }
  }

  return success;
}

/*******************************************************************/
/*!
 @brief   Prints the error counts for all runtime tests by reading from EEPROM registers
 @return  None
*******************************************************************/
void ClassB_PrintErrorCounts(void)
{
  uint32_t cpu_errors = 0;
  uint32_t ram_errors = 0;
  uint32_t crc_errors = 0;
  uint32_t adc_errors = 0;
  uint32_t clk_errors = 0;
  uint32_t wdg_errors = 0;
  uint32_t stack_errors = 0;
  uint32_t app_errors = 0;
  uint32_t flow_errors = 0;
  uint32_t wdg_expected_resets = 0;
  uint32_t wdg_unexpected_resets = 0;

  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_CPU, &cpu_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_RAM, &ram_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_CRC, &crc_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_ADC, &adc_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_CLK, &clk_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_WDG, &wdg_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_Stack, &stack_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_App, &app_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_ClassBError_Flow, &flow_errors);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_WdgResetExpected, &wdg_expected_resets);
  EEPROM_MCU_ReadReg(eEEPROM_Reg_WdgResetUnexpected, &wdg_unexpected_resets);

  LOG_Write(eLogger_Sys,
            eLogLevel_High,
            _Module,
            true,
            "ClassB Errors: CPU=%lu, RAM=%lu, CRC=%lu,  ADC=%lu, CLK=%lu, WDG=%lu, STK=%lu, APP=%lu, FLO=%lu",
            cpu_errors,
            ram_errors,
            crc_errors,
            adc_errors,
            clk_errors,
            adc_errors,
            wdg_errors,
            stack_errors,
            app_errors,
            flow_errors);

  LOG_Write(eLogger_Sys,
            eLogLevel_High,
            _Module,
            false,
            "WDG resets: expected=%lu unexpected=%lu",
            wdg_expected_resets,
            wdg_unexpected_resets);
}

/* Private Implementation ----------------------------------------------------*/
/* Protected functions ------------------------------------------------------ */

#ifdef CLASSB_PROTECTED

/*******************************************************************/
/*!
 @brief   Sets a fault for a specific runtime test
 @param   Test : The runtime test to set the fault for
 @return  Success status of the operation
*******************************************************************/
bool ClassB__SetFault(tClassBRunItem Test)
{
  bool success = false;

  if (Test < eClassBRunItem_NUM)
  {
    _FaultInjection[Test] = true;
    LOG_Write(eLogger_Sys, eLogLevel_High, _Module, false, "Fault injection set for test '%s'", _RunConfig[Test].Name);
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief   Clears a fault for a specific runtime test
 @param   Test : The runtime test to clear the fault for
 @return  Success status of the operation
*******************************************************************/
bool ClassB__ClearFault(tClassBRunItem Test)
{
  bool success = false;

  if (Test < eClassBRunItem_NUM)
  {
    _FaultInjection[Test] = false;
    LOG_Write(
      eLogger_Sys, eLogLevel_High, _Module, false, "Fault injection cleared for test '%s'", _RunConfig[Test].Name);
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief   Gets the fault status for a specific runtime test
 @param   Test : The runtime test to get the fault status for
 @return  Fault status of the operation
*******************************************************************/
bool ClassB__GetFault(tClassBRunItem Test)
{
  bool fault = false;

  if (Test < eClassBRunItem_NUM)
  {
    fault = _FaultInjection[Test];
  }

  return fault;
}

/*******************************************************************/
/*!
 @brief   TIM Input Capture callback. Measures LSI period for CLK testing
 @param   htim : TIM handle
 @return  None
 @note    Use inline and read/Write data directly to ClassB vars for ISR effiency
*******************************************************************/
void ClassB__IC_Callback(TIM_HandleTypeDef* Htim)
{
  assert_param(Htim->Instance == hTIM_CLKTEST.Instance);

  // Read/Write data directly to ClassB vars for ISR effiency
  extern tDataValue ClassBData[];
  extern tDataValue ClassBDataInv[];

  static uint32_t prev = 0;
  uint32_t now = HAL_TIM_ReadCapturedValue(Htim, TIM_CLKTEST_CH);

  uint32_t period;
  if (now >= prev)
    period = now - prev;
  else
    period = (Htim->Instance->ARR - prev + 1u) + now;  // timer overflow

  uint32_t period_inv = ~period;
  uint32_t flag = ClassBData[eClassBVar_RUN_LSI_PERIOD_FLAG_V32].V32 + 1u;
  uint32_t flag_inv = ~flag;

  ClassBData[eClassBVar_RUN_LSI_PERIOD_VALUE_V32].V32 = period;
  ClassBDataInv[eClassBVar_RUN_LSI_PERIOD_VALUE_V32].V32 = period_inv;
  ClassBData[eClassBVar_RUN_LSI_PERIOD_FLAG_V32].V32 = flag;
  ClassBDataInv[eClassBVar_RUN_LSI_PERIOD_FLAG_V32].V32 = flag_inv;

  prev = now;
}

#endif /* CLASSB_PROTECTED */
