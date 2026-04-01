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
* @file    eeprom_ext.h
* @brief   Interface for STM32L1xx Data EEPROM access helpers
* @author  Steve Quinlan
* @date    2026-Apr
*
******************************************************************************/

#ifndef __EEPROM_EXT_H
#define __EEPROM_EXT_H

#include "main.h"
#include "common.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

bool EEPROM_EXT_Read(uint32_t Address, uint8_t* Data, uint16_t Length);
bool EEPROM_EXT_Write(uint32_t Address, const uint8_t* Data, uint16_t Length);
bool EEPROM_EXT_Erase(void);
bool EEPROM_EXT_ReadLog(uint32_t Index, uint8_t* Log);
bool EEPROM_EXT_WriteLog(uint32_t Index, const uint8_t* Log);

#ifdef EEPROM_EXT_PROTECTED

/* Protected functions ------------------------------------------------------ */

#endif /* EEPROM_EXT_PROTECTED */

#endif /* __EEPROM_EXT_H */
