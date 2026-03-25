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

 @file    classb_vars.c
 @brief   Internal Class B variables implementation
 @author  Steve Quinlan
          Portions adapted from STMicroelectronics Class B library
          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 @date    2026-March

 ******************************************************************************/

#include "classb_vars.h"
#include "classb_params.h"
#include "util.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  tDataType Type;
  tDataValue Init;
  bool Pointer;
  const char* Name;
} tClassBData_Config;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/

// Storage for Class B variables and their inverted values for integrity check
tDataValue ClassBData[eClassBVar_NUM] __attribute__((section(".class_b_ram")));
tDataValue ClassBDataInv[eClassBVar_NUM] __attribute__((section(".class_b_ram_rev")));

// RAM location for temporary storage of original values at runtime RAM transparent test
uint32_t RuntimeRamBuf[RT_RAM_BLOCKSIZE + 2] __attribute__((section(".runtime_ram_buf")));

// Magic patterns for Stack overflow
volatile uint32_t StackOverFlowPtrn[4] __attribute__((section(".stack_bottom")));

/* Private variables ---------------------------------------------------------*/

static const char* _Module = "CBVAR";  //!< Module name for debug logging

// Must be in same order as eClassBVar_t enum
static const tClassBData_Config _Config[] = {
  { eDataType_U32, { .U32 = 0u }, false, "RUN_FLOW_CNT" },
  { eDataType_V32, { .V32 = 0u }, false, "RUN_LSI_PERIOD_VALUE" },
  { eDataType_V32, { .V32 = 0u }, false, "RUN_LSI_PERIOD_FLAG" },
  { eDataType_U32, { .U32 = 0u }, true, "RUB_RAMCHK" },
  { eDataType_U32, { .U32 = 0u }, true, "RUN_CRC32CHK" },
  { eDataType_S16, { .S16 = 0 }, false, "TEMP_PCB_S16" },
  { eDataType_S16, { .S16 = 0 }, false, "VREF_MV_S16" },
  { eDataType_S16, { .S16 = 0 }, false, "TRISE_X10_S16" },
};

/* Private function prototypes -----------------------------------------------*/

static void _Vars_Fail(tClassBVars var);

static_assert(sizeof(_Config) / sizeof(tClassBData_Config) == eClassBVar_NUM, "config size mismatch");
static_assert(sizeof(ClassBData) / sizeof(tDataValue) == eClassBVar_NUM, "data size mismatch");
static_assert(sizeof(ClassBDataInv) / sizeof(tDataValue) == eClassBVar_NUM, "inv size mismatch");

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Initializes Class B variables to default values
  @return    true if initialization successful, false if failure
 *******************************************************************/
bool ClassB_VarsInit(void)
{
  return true;
}

/*******************************************************************/
/*!
 @brief     Returns the WDG test status
 @param     var: enum of the variable to get
 @return    Value of the variable
  *******************************************************************/
tDataValue ClassB_GetVar(tClassBVars var)
{
  tDataValue retVal = { 0 };
  tDataValue invVal = { 0 };

  if (var < eClassBVar_NUM)
  {
    // Read value and its inverse for integrity check atomicly to prevent torn reads
    retVal = ClassBData[var];
    invVal = ClassBDataInv[var];

    // Simple integrity check
    switch (_Config[var].Type)
    {
      case eDataType_Float:
        if ((*(uint32_t*)&retVal.Float ^ *(uint32_t*)&invVal.Float) != 0xFFFFFFFFu)
        {
          // Data corrupted
          _Vars_Fail(var);
          retVal.Float = 0.0f;
        }
        break;

      case eDataType_U32:
      case eDataType_S32:
        if ((retVal.U32 ^ invVal.U32) != 0xFFFFFFFFu)
        {
          // Data corrupted
          _Vars_Fail(var);
          retVal.U32 = 0u;
        }
        break;

      case eDataType_V32:
        // Reload (close to) atomically for volatile data
        CRITICAL_SECTION_START();
        retVal = ClassBData[var];
        invVal = ClassBDataInv[var];
        CRITICAL_SECTION_END();

        if ((retVal.V32 ^ invVal.V32) != 0xFFFFFFFFu)
        {
          // Data corrupted
          _Vars_Fail(var);
          retVal.V32 = 0u;
        }
        break;

      case eDataType_U16:
      case eDataType_S16:
        if ((retVal.U16 ^ invVal.U16) != (uint16_t)0xFFFFu)
        {
          // Data corrupted
          _Vars_Fail(var);
          retVal.S16 = 0u;
        }
        break;

      case eDataType_Bool:
      case eDataType_U8:
      case eDataType_S8:
        if ((retVal.U8 ^ invVal.U8) != (uint8_t)0xFFu)
        {
          // Data corrupted
          _Vars_Fail(var);
          retVal.U8 = 0u;
        }
        break;

      default:
        // Unsupported type
        retVal.U32 = 0u;
        assert_always();
        break;
    }
  }

  return retVal;
}

/*******************************************************************/
/*!
 @brief     Sets the Class B variable
 @param     var: enum of the variable to set
 @param     value: value to set
 @return    Result of the set operation
  *******************************************************************/
tClassB_SetResult ClassB_SetVar(tClassBVars var, tDataValue value)
{
  tClassB_SetResult result = eClassBSet_Success;

  if (var < eClassBVar_NUM)
  {
    if (ClassBData[var].U32 != value.U32)
    {
      // Use U32 for comparison to becasue it is the largest type and all types are stored in a tDataValue union
      result = eClassBSet_Changed;
    }

    ClassBData[var] = value;
    switch (_Config[var].Type)
    {
      case eDataType_Float:
        ClassBDataInv[var].U32 = ~(*(uint32_t*)&value.Float);
        break;

      case eDataType_U32:
      case eDataType_S32:
        ClassBDataInv[var].U32 = ~(value.U32);
        break;

      case eDataType_V32:
        // Write (close to) atomically for volatiles
        CRITICAL_SECTION_START();
        ClassBData[var] = value;
        ClassBDataInv[var].V32 = ~(value.V32);
        CRITICAL_SECTION_END();
        break;

      case eDataType_U16:
      case eDataType_S16:
        ClassBDataInv[var].U16 = ~(value.U16);
        break;

      case eDataType_Bool:
      case eDataType_U8:
      case eDataType_S8:
        ClassBDataInv[var].U8 = ~(value.U8);
        break;

      default:
        // Unsupported type
        result = eClassBSet_Error;
        assert_always();
        break;
    }
  }
  else
  {
    result = eClassBSet_Error;
    assert_always();
  }

  return result;
}

/* Public Implementation -----------------------------------------------------*/

static void _Vars_Fail(tClassBVars var)
{
  if (LOG_IsInit(eLogger_Sys))
  {
    LOG_Write(eLogger_Sys,
              eLogLevel_Error,
              _Module,
              false,
              "ClassB var '%s' integrity failed (0x%X / 0x%X)",
              _Config[var].Name,
              ClassBData[var].U32,
              ClassBDataInv[var].U32);
  }
  else
  {
    // If logging is not initialized use the low-level print()
    print("ClassB var '%s' integrity failed: 0x%X / 0x%X (0x%X)\n\r",
          _Config[var].Name,
          ClassBData[var].U32,
          ClassBDataInv[var].U32,
          ~ClassBDataInv[var].U32);
  }
}
