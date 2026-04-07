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

@file    eeprom.h
@brief   Interface for STM32L1 Data EEPROM access helpers
@author  Steve Quinlan
@date    2026-March

******************************************************************************/

#ifndef __EEPROM_MCU_H
#define __EEPROM_MCU_H

#include "main.h"
#include "common.h"

/* Exported types ------------------------------------------------------------*/

//! General purpose registers. Rename as 'Reserved' needed
typedef enum
{
  eEEPROM_Reg_BootCount,
  eEEPROM_Reg_ModelIndex,
  eEEPROM_Reg_LogIndex,
  eEEPROM_Reg_ClassBError_CPU,
  eEEPROM_Reg_ClassBError_RAM,
  eEEPROM_Reg_ClassBError_CRC,
  eEEPROM_Reg_ClassBError_ADC,
  eEEPROM_Reg_ClassBError_CLK,
  eEEPROM_Reg_ClassBError_WDG,
  eEEPROM_Reg_ClassBError_Stack,
  eEEPROM_Reg_ClassBError_Flow,
  eEEPROM_Reg11_Reserved,
  eEEPROM_Reg12_Reserved,
  eEEPROM_Reg13_Reserved,
  eEEPROM_Reg14_Reserved,
  eEEPROM_Reg15_Reserved,
  eEEPROM_Reg_NUM,
} tEEPROM_Register;

/* Exported constants --------------------------------------------------------*/

#define EEPROM_MCU_BASEADDR FLASH_EEPROM_BASE                                  ///< 0x08080000
#define EEPROM_MCU_ENDADDR FLASH_EEPROM_END                                    ///< 0x08081FFF
#define EEPROM_MCU_SIZE_BYTES (EEPROM_MCU_ENDADDR - EEPROM_MCU_BASEADDR + 1u)  ///< 8192 bytes

#define EEPROM_MODEL_TABLE_ADDR (EEPROM_MCU_BASEADDR + 0x200u)  ///< Start of model table in EEPROM (after registers)

/* Exported macros ------------------------------------------------------------*/

/* Exported vars ------------------------------------------------------------ */

/* Exported functions ------------------------------------------------------- */

bool EEPROM_MCU_Read(uint32_t Address, uint8_t* Data, uint32_t Length);
bool EEPROM_MCU_Write(uint32_t Address, const uint8_t* Data, uint32_t Length);
bool EEPROM_MCU_ReadReg(tEEPROM_Register Reg, uint32_t* Value);
bool EEPROM_MCU_WriteReg(tEEPROM_Register Reg, uint32_t Value);
bool EEPROM_MCU_IncrementReg(tEEPROM_Register Reg);
bool EEPROM_MCU_Erase(bool RegsOnly);

#ifdef EEPROM_MCU_PROTECTED

/* Protected functions ------------------------------------------------------ */

#endif /* EEPROM_MCU_PROTECTED */

#endif /* __EEPROM_MCU_H */
