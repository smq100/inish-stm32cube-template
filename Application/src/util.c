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

 @file    util.c
 @brief   Source file for utility functions
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#include <stdarg.h>

#include "stm32l1xx_ll_adc.h"

#include "main.h"
#include "util.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

// clang-format off
static const uint16_t _Crc16CcittTable[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
// clang-format on

// Timeout for UART transmit
static const uint32_t _Timeout_ms = 500;

/* Private function prototypes -----------------------------------------------*/

/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief     Calculates CRC-16-CCITT for the given data buffer.
            We do not use the STM32 HAL CRC functions because the CRC peripheral is used for ClassB CRC calcs.
 @param     Data: pointer to the data buffer
 @param     Len: length of the data buffer in bytes
 @return    The calculated CRC value

 @note      Parameters:
              Width : 16
              Poly  : 0x1021
              Init  : 0xFFFF
              RefIn : false
              RefOut: false
              XorOut: 0x0000
*******************************************************************/
uint16_t Calculate_CRC16(const void* Data, size_t Len)
{
  const uint8_t* p = (const uint8_t*)Data;
  uint16_t crc = 0xFFFF;

  while (Len--)
  {
    uint8_t idx = (uint8_t)((crc >> 8) ^ *p++);
    crc = (uint16_t)((crc << 8) ^ _Crc16CcittTable[idx]);
  }

  return crc;  // xorout = 0x0000
}

/*******************************************************************/
/*!
 @brief     Checks if the given address is within RAM
 @param     Addr: address to check
 @return    True if address is in RAM
*******************************************************************/
bool IsRAM(uintptr_t Addr)
{
  extern uint32_t _ram_start;  // Defined in the linker script, marks the start of RAM
  extern uint32_t _ram_end;    // Defined in the linker script, marks one-past-end of RAM

  return (Addr >= (uintptr_t)&_ram_start && Addr < (uintptr_t)&_ram_end);
}

/*******************************************************************/
/*!
 @brief     Checks if the given address is within ROM
 @param     Addr: address to check
 @return    True if address is in ROM
*******************************************************************/
bool IsROM(uintptr_t Addr)
{
  extern uint32_t _rom_start;  // Defined in the linker script, marks the start of ROM
  extern uint32_t _rom_end;    // Defined in the linker script, marks one-past-end of ROM

  return (Addr >= (uintptr_t)&_rom_start && Addr < (uintptr_t)&_rom_end);
}

/*******************************************************************/
/*!
 @brief     Checks if the given address is within valid memory (RAM or ROM)
 @param     Addr: address to check
 @return    True if address is in valid memory
*******************************************************************/
bool IsPointerValid(uintptr_t Addr)
{
  return IsRAM(Addr) || IsROM(Addr);
}

/*******************************************************************/
/*!
 @brief     Calculates VDDA in millivolts from the given VREFINT ADC value
 @param     Measure: raw ADC value from VREFINT channel
 @return    Calculated VDDA in millivolts
*******************************************************************/
int16_t CalcVDDA_mv(uint16_t Measure)
{
  int16_t vdda_mv = (int16_t)__LL_ADC_CALC_VREFANALOG_VOLTAGE((uint32_t)Measure, LL_ADC_RESOLUTION_12B);

  return vdda_mv;
}

/*******************************************************************/
/*!
 @brief     Calculates core temperature in deci-C from TSENSE and VREFINT ADC values
 @param     TempData: raw ADC value from TSENSE channel
 @param     VRefData: raw ADC value from VREFINT channel
 @return    Calculated core temperature in deci-C
*******************************************************************/
int16_t CalcCoreTemp_Cx10(uint16_t TempData, uint16_t VRefData)
{
  const int32_t min_temp_deci_c = -400;  // -40.0 C
  const int32_t max_temp_deci_c = 1500;  // 150.0 C

  int16_t vdda_mv = CalcVDDA_mv(VRefData);
  int32_t temperature_deci_c = __LL_ADC_CALC_TEMPERATURE(vdda_mv, (uint32_t)TempData, LL_ADC_RESOLUTION_12B);
  temperature_deci_c *= 10;

  // Clamp to plausible MCU die-temperature range to avoid wrap/NaN propagation.
  if (temperature_deci_c < min_temp_deci_c)
  {
    temperature_deci_c = min_temp_deci_c;
  }
  else if (temperature_deci_c > max_temp_deci_c)
  {
    temperature_deci_c = max_temp_deci_c;
  }

  return temperature_deci_c;
}

/*******************************************************************/
/*!
 @brief     Reads memory from the given address into the provided buffer
 @param     Addr: Address to read from
 @param     Buffer: Buffer to store the read data
 @param     Len: Length of the data to read (32 max)
 @return    Number of bytes read
*******************************************************************/
uint8_t ReadMemory(uintptr_t Addr, uint8_t* Buffer, size_t Len)
{
  uint8_t read = 0;

  if (Addr > 0x20020000)  // Ram end is 0x20020000 for 128KB of RAM, so any address above that is invalid
  {
    assert_always();
  }
  else if (Len > 0)
  {
    if (Len > MAX_MEMREAD_LEN)
    {
      Len = MAX_MEMREAD_LEN;  // Limit max read length
    }

    uint8_t* src = (uint8_t*)Addr;
    for (size_t i = 0; i < Len; i++)
    {
      Buffer[i] = src[i];
    }

    read = (uint8_t)Len;
  }

  return read;
}

/*******************************************************************/
/*!
 @brief     Writes a character to UART for debugging
 @param     Label: Label to print before the hex dump
 @param     Data: Pointer to the data to dump
 @param     Len: Length of the data in bytes
 @param     Buffer: Optional buffer to receive the string. If null, the output will be printed directly to the console
 @param     BufferLen: Length of the buffer in bytes
*******************************************************************/
void DumpHex(const char* Label, const void* Data, size_t Len, char* Buffer, size_t BufferLen)
{
  const uint8_t* bytes = (const uint8_t*)Data;
  size_t offset = 0;

  if (!IsPointerValid((uintptr_t)Data) || Len == 0)
  {
    assert_always();
  }
  else if (!IsPointerValid((uintptr_t)Label))
  {
    assert_always();
  }
  else if (Buffer == NULLPTR)
  {
    printf("*** %s (addr=0x%08X, len=%u): ", Label, (uintptr_t)Data, Len);
  }
  else if (IsRAM((uintptr_t)Buffer) && BufferLen > 0)
  {
    snprintf(Buffer, BufferLen, "%s (addr=0x%08X, len=%u): ", Label, (uintptr_t)Data, Len);
  }
  else
  {
    Buffer = NULLPTR;  // Invalid buffer, fallback to direct print
    assert_always();
  }

  while (offset < Len)
  {
    // Print hex
    for (size_t i = 0; i < 16u; i++)
    {
      if (offset + i < Len)
      {
        if (Buffer == NULLPTR)
        {
          printf("%02X ", bytes[i]);
        }
        else
        {
          snprintf(Buffer + strlen(Buffer), BufferLen - strlen(Buffer), "%02X ", bytes[i]);
        }
      }
      else
      {
        if (Buffer == NULLPTR)
        {
          printf("   ");
        }
        else
        {
          snprintf(Buffer + strlen(Buffer), BufferLen - strlen(Buffer), "   ");
        }
      }
    }

    // Print ASCII
    if (Buffer == NULLPTR)
    {
      printf(" ");
    }
    else
    {
      snprintf(Buffer + strlen(Buffer), BufferLen - strlen(Buffer), " ");
    }
    for (size_t i = 0; i < 16u; i++)
    {
      if (offset + i < Len)
      {
        uint8_t b = bytes[i];
        if (Buffer == NULLPTR)
        {
          printf("%c", (b >= 32u && b <= 126u) ? b : '.');
        }
        else
        {
          snprintf(Buffer + strlen(Buffer), BufferLen - strlen(Buffer), "%c", (b >= 32u && b <= 126u) ? b : '.');
        }
      }
    }

    if (Buffer == NULLPTR)
    {
      printf("\n");
    }

    offset += 16u;
    bytes += 16u;
  }
}

/*******************************************************************/
/*!
 @brief     Writes a character to UART for debugging
            Overrides the weak __io_putchar function used by printf
 @param     ch: character to write
 @return    The character written
*******************************************************************/
int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;

  HAL_UART_Transmit(&BOOT_UART_HANDLE, &c, 1u, _Timeout_ms);

  return ch;
}
