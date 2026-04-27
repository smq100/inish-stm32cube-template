/*!
 ******************************************************************************
 * @copyright Copyright (c) 2026 Phillips And Termo. All rights reserved.
 *
 * Created in 2026 as an unpublished copyrighted work. This program and the
 * information contained in it are confidential and proprietary to
 * Phillips And Termo and may not be used, copied, or reproduced without
 * the prior written permission of Phillips And Termo
 *
 ******************************************************************************

 @file    rtc_app.c
 @brief   Source file for RTC functions
 @author  Steve Quinlan (Design Solutions, Inc.)
 @date    2026-Apr

 ******************************************************************************/

#define RTC_PROTECTED

#include "main.h"
#include "rtc_app.h"
#include "log.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
  bool IsActive;       ///< Current state of button GPIO state
  bool IsActive_Prev;  ///< Previous state of button GPIO state
} tModuleStatus;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static const char* _Module = "RTC";

/* Private function prototypes -----------------------------------------------*/
/* Public Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief   Reads a value from the specified RTC backup register
 @param   BackupRegister: The backup register to read from
 @return  The value read from the backup register
*******************************************************************/
uint32_t RTC_BkupRead(uint32_t BackupRegister)
{
  assert_param(IS_RTC_BKP(BackupRegister));

  uint32_t tmp = (uint32_t)&(RTC->BKP0R);
  tmp += (BackupRegister * 4u);

  LOG_Write(eLogger_Sys,
            eLogLevel_Low,
            _Module,
            false,
            "Reading RTC backup register %lu: 0x%08lX",
            BackupRegister,
            (*(__IO uint32_t*)tmp));

  return (*(__IO uint32_t*)tmp);
}

/*******************************************************************/
/*!
 @brief   Writes a value to the specified RTC backup register
 @param   BackupRegister: The backup register to write to
 @param   Data: The value to write to the backup register
 @return  None
*******************************************************************/
void RTC_BkupWrite(uint32_t BackupRegister, uint32_t Data)
{
  assert_param(IS_RTC_BKP(BackupRegister));

  uint32_t tmp = (uint32_t)&(RTC->BKP0R);
  tmp += (BackupRegister * 4u);

  *(__IO uint32_t*)tmp = Data;

  LOG_Write(
    eLogger_Sys, eLogLevel_Low, _Module, false, "Writing RTC backup register %lu: 0x%08lX", BackupRegister, Data);
}

/* Private Implementation -----------------------------------------------------*/
/* Protected Implementation -----------------------------------------------------*/

#ifdef RTC_PROTECTED

#endif /* RTC_PROTECTED */
