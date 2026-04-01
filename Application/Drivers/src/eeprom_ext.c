/*!
******************************************************************************
* @copyright
* ### COPYRIGHT(c) 2026 Design Solutions, Incorporated (DSI)
*
* Created in 2026 as an unpublished copyrighted work. This program and the
* information contained in it are confidential and proprietary to
* DSI and may not be used, copied, or reproduced without
* the prior written permission of DSI
*
******************************************************************************
* @file    eeprom_ext.c
* @brief   Implementation of Microchip I2C EEPROM access helpers
* @author  Steve Quinlan
* @date    2026-Apr
*
* @note    Microchip 24LC256 EEPROM (32KB) accessed via I2C.
*          256-Kbit, 3.4 MHz I2C Serial EEPROM with 128-Bit Serial
*          Number and Enhanced Software Write Protection
*          Addressing is 16-bit, so max address is 0x7FFF (32KB)
*          See datasheet for more details.
******************************************************************************/

#define EEPROM_EXT_PROTECTED

#include "main.h"
#include "eeprom_ext.h"
#include "i2c.h"
#include "util.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "EPRM_X";
static const uint16_t _PageSizeBytes = 64u;                    ///! EEPROM page size in bytes
static const uint16_t _LogSizeBytes = MAX_DEBUGLOG_CHARS_TOT;  ///! Must be multiple of EEPROM page size
static const uint32_t _MaxEEPROMSizeBytes = 32768u;            ///! 32KB total size of the EEPROM
static const uint32_t _I2Ctimeout = 100u;                      ///! Timeout for I2C operations in ms

//!< true to emulate EEPROM in RAM (for testing without hardware). false to use actual EEPROM.
static const bool _Emulate = true;

static I2C_HandleTypeDef* _I2C_Handle = &hi2c1;
static uint16_t _DeviceAddress = 0x50 << 1;  // 7-bit address shifted for HAL

/* Private function prototypes -----------------------------------------------*/

static bool _IsAddressValid(uint32_t Address, uint16_t Length);

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Reads bytes from data EEPROM
 @param     address: Absolute EEPROM address
 @param     data: Destination buffer
 @param     length: Number of bytes to read
 @return    true if successful
 *******************************************************************/
bool EEPROM_EXT_Read(uint32_t Address, uint8_t* Data, uint16_t Length)
{
  bool success = false;
  uint32_t currentAddress = Address;
  uint8_t* currentData = Data;
  uint16_t remaining = Length;

  if (Length == 0u)
  {
    success = true;
  }
  else if (!IsRAM((uintptr_t)Data))
  {
    assert_always();
  }
  else if (!_IsAddressValid(Address, Length))
  {
  }
  else
  {
    success = true;

    while (remaining > 0u)
    {
      uint16_t pageOffset = currentAddress % _PageSizeBytes;
      uint16_t bytesToPageEnd = _PageSizeBytes - pageOffset;
      uint16_t chunkLength = (remaining < bytesToPageEnd) ? remaining : bytesToPageEnd;

      if (_Emulate)
      {
        // Just fill with dummy data for testing without hardware
        for (uint16_t i = 0; i < chunkLength; i++)
        {
          currentData[i] = 0xAAu;
        }
      }
      else if (HAL_I2C_Mem_Read(_I2C_Handle,
                                _DeviceAddress,
                                currentAddress,
                                I2C_MEMADD_SIZE_16BIT,
                                currentData,
                                chunkLength,
                                _I2Ctimeout) != HAL_OK)
      {
        success = false;
        break;
      }

      currentAddress += chunkLength;
      currentData += chunkLength;
      remaining -= chunkLength;
    }
  }

  if (success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "Read from EXT EEPROM 0x%04X (%u bytes)", Address, Length);
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to read EXT EEPROM 0x%04X", Address);
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
bool EEPROM_EXT_Write(uint32_t Address, const uint8_t* Data, uint16_t Length)
{
  bool success = false;
  uint32_t currentAddress = Address;
  const uint8_t* currentData = Data;
  uint16_t remaining = Length;

  if (Length == 0u)
  {
    success = true;
  }
  else if (!IsPointerValid((uintptr_t)Data))
  {
    assert_always();
  }
  else if (!_IsAddressValid(Address, Length))
  {
  }
  else
  {
    success = true;

    while (remaining > 0u)
    {
      uint16_t pageOffset = currentAddress % _PageSizeBytes;
      uint16_t bytesToPageEnd = _PageSizeBytes - pageOffset;
      uint16_t chunkLength = (remaining < bytesToPageEnd) ? remaining : bytesToPageEnd;

      if (_Emulate)
      {
        // Ignore
      }
      else if (HAL_I2C_Mem_Write(_I2C_Handle,
                                 _DeviceAddress,
                                 currentAddress,
                                 I2C_MEMADD_SIZE_16BIT,
                                 (uint8_t*)currentData,
                                 chunkLength,
                                 _I2Ctimeout) != HAL_OK)
      {
        success = false;
        break;
      }

      if (_Emulate)
      {
      }
      else if (HAL_I2C_IsDeviceReady(_I2C_Handle, _DeviceAddress, _I2Ctimeout, 10u) != HAL_OK)
      {
        success = false;
        break;
      }

      currentAddress += chunkLength;
      currentData += chunkLength;
      remaining -= chunkLength;
    }
  }

  if (success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Low, _Module, false, "Wrote to EXT EEPROM 0x%04X (%u bytes)", Address, Length);
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to write EXT EEPROM 0x%04X", Address);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Sets entire data EEPROM bank to 0xFF
 @return    true if successful
 *******************************************************************/
bool EEPROM_EXT_Erase(void)
{
  bool success = true;
  uint32_t currentAddress = 0u;
  uint8_t pageData[64u];

  for (uint16_t i = 0u; i < _PageSizeBytes; i++)
  {
    pageData[i] = 0xFFu;
  }

  while (currentAddress < _MaxEEPROMSizeBytes)
  {
    if (_Emulate)
    {
      // Just ignore for testing without hardware
    }
    else if (HAL_I2C_Mem_Write(_I2C_Handle,
                               _DeviceAddress,
                               currentAddress,
                               I2C_MEMADD_SIZE_16BIT,
                               pageData,
                               _PageSizeBytes,
                               _I2Ctimeout) != HAL_OK)
    {
      success = false;
      break;
    }

    if (_Emulate)
    {
      // Just ignore for testing without hardware
    }
    else if (HAL_I2C_IsDeviceReady(_I2C_Handle, _DeviceAddress, _I2Ctimeout, 10u) != HAL_OK)
    {
      success = false;
      break;
    }

    currentAddress += _PageSizeBytes;
  }

  if (!success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to erase EXT EEPROM");
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Reads a 32-bit register from data EEPROM
 @param     Index: Register index
 @param     Log: Pointer to store the read
 @return    true if successful
 *******************************************************************/
bool EEPROM_EXT_ReadLog(uint32_t Index, uint8_t* Log)
{
  bool success = false;
  uint32_t address = Index * _LogSizeBytes;

  if (!IsRAM((uintptr_t)Log))
  {
    assert_always();
  }
  else if (!_IsAddressValid(address, _LogSizeBytes))
  {
    assert_always();
  }
  else
  {
    success = EEPROM_EXT_Read(address, Log, _LogSizeBytes);
  }

  if (success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Med, _Module, false, "Read log entry %u from EXT EEPROM", Index);
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to read log entry %u from EXT EEPROM", Index);
  }

  return success;
}

/*******************************************************************/
/*!
 @brief     Writes a 32-bit register to data EEPROM
 @param     Index: Register index
 @param     Log: Pointer to the value to write
 @return    true if successful
 *******************************************************************/
bool EEPROM_EXT_WriteLog(uint32_t Index, const uint8_t* Log)
{
  bool success = false;
  uint32_t address = Index * _LogSizeBytes;

  if (!IsRAM((uintptr_t)Log))
  {
    assert_always();
  }
  else if (!_IsAddressValid(address, _LogSizeBytes))
  {
    assert_always();
  }
  else
  {
    success = EEPROM_EXT_Write(address, Log, _LogSizeBytes);
  }

  if (success)
  {
    LOG_Write(eLogger_Sys, eLogLevel_Med, _Module, false, "Wrote log entry %u to EXT EEPROM", Index);
  }
  else
  {
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Failed to write log entry %u to EXT EEPROM", Index);
  }

  return success;
}

/* Private Implementation ----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Checks if an EEPROM address is valid
 @param     address: Absolute target address
 @param     length: Number of bytes to access
 @return    true if in data EEPROM region
 *******************************************************************/
static bool _IsAddressValid(uint32_t Address, uint16_t Length)
{
  bool success = true;

  if ((Address + Length) > _MaxEEPROMSizeBytes)
  {
    success = false;
    LOG_Write(eLogger_Sys, eLogLevel_Error, _Module, false, "Invalid address 0x%04X (length %u)", Address, Length);
  }

  return success;
}
