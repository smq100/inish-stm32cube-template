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
******************************************************************************`
 @file    eeprom.c
 @brief   Implementation of STM32L1 Data EEPROM access helpers
 @author  Steve Quinlan
 @date    2026-March

 @note EEPROM Memory Map:
    0x08080000 - 0x0808003F: General purpose registers (16 x 32-bit). See tEEPROM_Register
    0x08080040 - 0x080801FF: Reserved for future use
    0x08080200 - Varies:     tHCTN_Model table (size varies based on the number of models)
    Varies     - 0x08080FFF: Reserved for future use

******************************************************************************/

#define EEPROM_PROTECTED

#include "main.h"
#include "eeprom.h"
#include "util.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "EEPROM";

/* Private function prototypes -----------------------------------------------*/

static bool _WriteByte(uint32_t Address, uint8_t Value);
static bool _GetRegisterAddress(tEEPROM_Register Reg, uint32_t* Address);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Checks if an EEPROM address is valid
 @param     address: Absolute target address
 @return    true if in data EEPROM region
 *******************************************************************/
bool EEPROM_IsAddressValid(uint32_t Address)
{
  return ((Address >= EEPROM_BASE_ADDR) && (Address <= EEPROM_END_ADDR));
}

/*******************************************************************/
/*!
 @brief     Checks if an EEPROM range is valid
 @param     address: Absolute target address
 @param     length: Number of bytes in range
 @return    true if full range is valid
 *******************************************************************/
bool EEPROM_IsRangeValid(uint32_t Address, uint32_t Length)
{
  bool success = false;

  uint32_t end = Address + Length - 1u;

  if (Length == 0u)
  {
  }
  else if (end < Address)
  {
  }
  else
  {
    success = EEPROM_IsAddressValid(Address) && EEPROM_IsAddressValid(end);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Reads bytes from data EEPROM
 @param     address: Absolute EEPROM address
 @param     data: Destination buffer
 @param     length: Number of bytes to read
 @return    true if successful
 *******************************************************************/
bool EEPROM_Read(uint32_t Address, uint8_t* Data, uint32_t Length)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Data))
  {
    assert_always();
  }
  else if (!EEPROM_IsRangeValid(Address, Length))
  {
  }
  else
  {
    for (uint32_t i = 0; i < Length; i++)
    {
      Data[i] = *(volatile uint8_t*)(Address + i);
    }

    success = true;
  }

  if (!success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to read EEPROM address 0x%08X", Address);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Writes bytes to data EEPROM
 @param     address: Absolute EEPROM address
 @param     data: Source buffer
 @param     length: Number of bytes to write
 @return    true if successful
 *******************************************************************/
bool EEPROM_Write(uint32_t Address, const uint8_t* Data, uint32_t Length)
{
  bool success = false;

  if (!IsPointerValid((uintptr_t)Data))
  {
    assert_always();
  }
  else if (!EEPROM_IsRangeValid(Address, Length))
  {
  }
  else if (HAL_FLASHEx_DATAEEPROM_Unlock() != HAL_OK)
  {
  }
  else
  {
    success = true;

    for (uint32_t i = 0; i < Length; i++)
    {
      if (!_WriteByte(Address + i, Data[i]))
      {
        success = false;
        break;
      }
    }

    success = (HAL_FLASHEx_DATAEEPROM_Lock() == HAL_OK) && success;
  }

  if (!success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to read EEPROM address 0x%08X", Address);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Sets entire data EEPROM bank to 0xFF
 @param     RegsOnly: If true, erase only registers, otherwise erase entire EEPROM
 @return    true if successful
 *******************************************************************/
bool EEPROM_Erase(bool RegsOnly)
{
  bool success = true;

  if (RegsOnly)
  {
    // Clear regs to zeros (will be 0xFF after full erase, but more appropriate to work with zeros as regs)
    for (tEEPROM_Register i = 0; i < eEEPROM_Reg_NUM; i++)
    {
      if (!EEPROM_WriteReg(i, 0u))
      {
        success = false;
        break;
      }
    }
  }
  else
  {
    if (HAL_FLASHEx_DATAEEPROM_Unlock() == HAL_OK)
    {
      for (uint32_t address = EEPROM_BASE_ADDR; address <= EEPROM_END_ADDR; address += 4u)
      {
        if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, address, 0xFFFFFFFFu) != HAL_OK)
        {
          success = false;
          break;
        }
      }

      if (success)
      {
        success = (HAL_FLASHEx_DATAEEPROM_Lock() == HAL_OK);

        if (success)
        {
          // Recursive call to clear regs to zeros
          success = EEPROM_Erase(true);
        }
        else
        {
          // If we failed to erase, attempt to lock anyway (recursive call) to avoid leaving EEPROM unlocked
          assert_always();
        }
      }
    }
    else
    {
      success = false;
    }
  }

  if (!success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Error erasing EEPROM (RegsOnly=%u)", RegsOnly);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Reads a 32-bit EEPROM register
 @param     reg: Register index
 @param     value: Output value
 @return    true if successful
 *******************************************************************/
bool EEPROM_ReadReg(tEEPROM_Register Reg, uint32_t* Value)
{
  bool success = false;
  uint32_t address = 0u;

  if (_GetRegisterAddress(Reg, &address))
  {
    success = EEPROM_ReadU32(address, Value);
  }

  if (!success)
  {
    *Value = 0u;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Writes a 32-bit EEPROM register
 @param     reg: Register index
 @param     value: Value to write
 @return    true if successful
 *******************************************************************/
bool EEPROM_WriteReg(tEEPROM_Register Reg, uint32_t Value)
{
  bool success = false;
  uint32_t address = 0u;

  if (_GetRegisterAddress(Reg, &address))
  {
    success = EEPROM_WriteU32(address, Value);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Increments a 32-bit EEPROM register
 @param     reg: Register index
 @return    true if successful
 *******************************************************************/
bool EEPROM_IncrementReg(tEEPROM_Register Reg)
{
  bool success = false;
  uint32_t value = 0u;

  if (EEPROM_ReadReg(Reg, &value))
  {
    value = (value == UINT32_MAX) ? 1u : (value + 1u);
    success = EEPROM_WriteReg(Reg, value);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Reads a 32-bit value from data EEPROM
 @param     address: Absolute EEPROM address
 @param     value: Output value
 @return    true if successful
 *******************************************************************/
bool EEPROM_ReadU32(uint32_t Address, uint32_t* Value)
{
  bool success = false;
  uint8_t buf[4];

  if (!IsRAM((uintptr_t)Value))
  {
    assert_always();
  }
  else if (EEPROM_Read(Address, buf, sizeof(buf)))
  {
    *Value = ((uint32_t)buf[0]) | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    success = true;
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Writes a 32-bit value to data EEPROM
 @param     address: Absolute EEPROM address
 @param     value: Value to write
 @return    true if successful
 *******************************************************************/
bool EEPROM_WriteU32(uint32_t Address, uint32_t Value)
{
  uint8_t buf[4];

  buf[0] = (uint8_t)(Value & 0xFFu);
  buf[1] = (uint8_t)((Value >> 8) & 0xFFu);
  buf[2] = (uint8_t)((Value >> 16) & 0xFFu);
  buf[3] = (uint8_t)((Value >> 24) & 0xFFu);

  return EEPROM_Write(Address, buf, sizeof(buf));
}

/* Private Implementation ----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Writes a byte to data EEPROM
 @param     Address: Absolute EEPROM address
 @param     Value: Value to write
 @return    true if successful
 *******************************************************************/
static bool _WriteByte(uint32_t Address, uint8_t Value)
{
  HAL_StatusTypeDef status;

  if (*(volatile uint8_t*)Address == Value)
  {
    return true;
  }

  status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, Address, Value);

  return (status == HAL_OK);
}

/*******************************************************************/
/*!
 @brief     Calculates the absolute EEPROM address of a register
 @param     Reg: Register index
 @param     Address: Absolute EEPROM address
 @return    true if successful
 *******************************************************************/
static bool _GetRegisterAddress(tEEPROM_Register Reg, uint32_t* Address)
{
  bool success = false;

  if (!IsRAM((uintptr_t)Address))
  {
    assert_always();
  }
  else if (((uint32_t)Reg) >= ((uint32_t)eEEPROM_Reg_NUM))
  {
    assert_always();
  }
  else
  {
    uint32_t regAddress = EEPROM_BASE_ADDR + (((uint32_t)Reg) * sizeof(uint32_t));

    if (EEPROM_IsRangeValid(regAddress, sizeof(uint32_t)))
    {
      *Address = regAddress;
      success = true;
    }
  }

  return success;
}
