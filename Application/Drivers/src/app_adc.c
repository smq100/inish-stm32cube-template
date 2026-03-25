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

 @file    app_adc.c
 @brief   ADC module source file
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#define ADC_PROTECTED

#include "stm32l1xx_ll_adc.h"

#include "main.h"
#include "adc.h"
#include "classb_vars.h"
#include "task_system.h"
#include "app_adc.h"
#include "timer.h"
#include "task_daq.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  bool IsActive;       ///< Current state of button GPIO state
  bool IsActive_Prev;  ///< Previous state of button GPIO state
} tModuleStatus;

/* Private define ------------------------------------------------------------*/
#define ADC_DMA_DATA_SIZE eADC_ID_NUM
#define ADC_FULL_SCALE 4095
#define ADC_FULL_VOLTAGE 3300
#define ADC_HW_LOW 0x0000   ///< minimum value for ADC values
#define ADC_HW_HIGH 0x0FFF  ///< maximum value for ADC values

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/

volatile bool _ADCDMAComplete;

/* Private variables ---------------------------------------------------------*/

static const char* _Module = "ADC";          //!< Name of the module. Used for debug logging
static int16_t _ADCData[ADC_DMA_DATA_SIZE];  ///< holds the raw ADC values, indexed by HCTN_ADC_INDEX_XXX
static const uint32_t _TimeoutDMA_ms = 20;   ///< timeout for ADC DMA read to complete, in milliseconds

/* Private function prototypes -----------------------------------------------*/

static int16_t _CalcPCBTemp_deci_C(int16_t TempData, int16_t VrefData);
static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes the ADC module
 @return    true if initialization was successful
 *******************************************************************/
bool ADC_Init(void)
{
  // Initialize ClassB variables to default values to ensure valid values
  // for any ClassB tests that run before the ClassB task is initialized
  ClassB_SetVar(eClassBVar_PCBTEMP_ADC_S16, (tDataValue){ .S16 = 0 });
  ClassB_SetVar(eClassBVar_VREF_ADC_S16, (tDataValue){ .S16 = 0 });

  // Register DAQ items
  tDAQ_Config daq;
  daq.CallbackRead = _DAQReadCallback;
  daq.CallbackWrite = NULLPTR;  // Read-only
  DAQ_RegisterItem(eDAQ_TempPCB, &daq, false);
  DAQ_RegisterItem(eDAQ_Vref, &daq, false);

  return true;
}

/*******************************************************************/
/*!
 @brief     Refreshes the ADC values by performing a DMA read of the ADC channels.
 @return    true if ADC refresh was successful
 *******************************************************************/
bool ADC_Refresh(void)
{
  uint32_t dma_len = hadc.Init.NbrOfConversion;

  // Mark that we're updating the array of ADC values
  _ADCDMAComplete = false;

  for (int n = 0; n < ADC_DMA_DATA_SIZE; n++)
  {
    _ADCData[n] = 0;
  }

  if (dma_len == 0u)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, false, "ADC DMA length is zero");
  }
  else if (HAL_ADC_Start_DMA(&hadc, (uint32_t*)&_ADCData[0], dma_len) != HAL_OK)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, false, "ADC start DMA error");
  }

  uint32_t timeout = TIMER_GetTick();
  while (!_ADCDMAComplete)
  {
    if (TIMER_GetElapsed_ms(timeout) > _TimeoutDMA_ms)
    {
      LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, false, "ADC read timeout");
      break;
    }
  }

  if (HAL_ADC_Stop_DMA(&hadc) != HAL_OK)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Warning, _Module, false, "ADC stop DMA error");
  }

  int16_t temp_adc = (int16_t)_ADCData[eADC_PCBTemp];
  int16_t vref_adc = (int16_t)_ADCData[eADC_Vref];

  ClassB_SetVar(eClassBVar_PCBTEMP_ADC_S16, (tDataValue){ .S16 = temp_adc });
  ClassB_SetVar(eClassBVar_VREF_ADC_S16, (tDataValue){ .S16 = vref_adc });

  return true;
}

/*******************************************************************/
/*!
 @brief     Gets the raw ADC value for a specified channel
 @param     Channel: ADC channel identifier
 @return    Raw ADC value
 *******************************************************************/
uint16_t ADC_GetRaw(tADC_ID Channel)
{
  uint16_t ret = 0;

  if (Channel < eADC_ID_NUM)
  {
    ret = (uint16_t)_ADCData[Channel];
  }
  else
  {
    assert_param(false);
  }

  return ret;
}

/* Private Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief Calculate PCB temperature in deci-C from TSENSE and VREFINT ADC values
 @param TempData Raw TSENSE ADC value
 @param VrefData Raw VREFINT ADC value
 @retval Calculated PCB temperature in deci-C (e.g. 253 = 25.3 C)
********************************************************************/
static int16_t _CalcPCBTemp_deci_C(int16_t TempData, int16_t VrefData)
{
  const int32_t min_temp_deci_c = -400;  // -40.0 C
  const int32_t max_temp_deci_c = 1500;  // 150.0 C

  // LOG_Write(eLogger_Sys, eLogLevel_Debug, _Module, false, "TempData: %d, VrefData: %d\r\n", TempData, VrefData);

  uint32_t vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE((uint32_t)VrefData, LL_ADC_RESOLUTION_12B);
  int32_t temperature_deci_c = __LL_ADC_CALC_TEMPERATURE(vdda_mv, (uint32_t)TempData, LL_ADC_RESOLUTION_12B);
  temperature_deci_c *= 10;

  // Clamp to plausible MCU die-temperature range
  if (temperature_deci_c < min_temp_deci_c)
  {
    temperature_deci_c = min_temp_deci_c;
  }
  else if (temperature_deci_c > max_temp_deci_c)
  {
    temperature_deci_c = max_temp_deci_c;
  }

  return (int16_t)temperature_deci_c;
}

/*******************************************************************/
/*!
 @brief     Callback for DAQ reads of system variables
 @return    True if successful
 *******************************************************************/
static bool _DAQReadCallback(tDAQ_Entry Entry, uint8_t Item)
{
  bool success = false;

  int16_t pcbtemp_adc = ClassB_GetVar(eClassBVar_PCBTEMP_ADC_S16).S16;
  int16_t vref_adc = ClassB_GetVar(eClassBVar_VREF_ADC_S16).S16;

  tDataValue value;
  if (Entry == eDAQ_TempPCB)
  {
    int16_t temp = _CalcPCBTemp_deci_C(pcbtemp_adc, vref_adc);
    value.Float = temp / 10.0f;
    DAQ_UpdateItem(Entry, Item, value);
    success = true;
  }
  else if (Entry == eDAQ_Vref)
  {
    int16_t vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE((uint32_t)vref_adc, LL_ADC_RESOLUTION_12B);
    value.Float = vdda_mv / 1000.0f;
    DAQ_UpdateItem(Entry, Item, value);
    success = true;
  }

  return success;
}

/* Protected Implementation -----------------------------------------------------*/

#ifdef TEMPLATE_PROTECTED

#endif /* TEMPLATE_PROTECTED */
