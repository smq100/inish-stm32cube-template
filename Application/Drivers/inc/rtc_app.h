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

 @file    rtc_app.h
 @brief   Interface header file for RTC functions
 @author  Steve Quinlan (Design Solutions, Inc.)
 @date    2026-Apr

 ******************************************************************************/

#ifndef __RTC_APP_H
#define __RTC_APP_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

// Non volatile backup register to store RAM test result across resets
#define RTC_RAMCHECK_STATUS_REG RTC_BKP_DR0
#define RTC_WDGEXPECT_MARKER_REG RTC_BKP_DR1
#define RTC_WDGEXPECT_MARKERINV_REG RTC_BKP_DR2

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

uint32_t RTC_BkupRead(uint32_t BackupRegister);
void RTC_BkupWrite(uint32_t BackupRegister, uint32_t Data);

#ifdef RTC_PROTECTED

#endif /* RTC_PROTECTED */

#endif /* __RTC_APP_H */
