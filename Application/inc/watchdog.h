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

 @file    watchdog.h
 @brief   Interface header file for the watchdog module
 @author  Steve Quinlan (Design Solutions, Inc.)
 @date    2026-Apr

 ******************************************************************************/

#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

#define CLASSB_WDG_HB_TASK_LOOP ((uint32_t)0x00000001)
#define CLASSB_WDG_HB_CLASSB ((uint32_t)0x00000002)

// TODO - enable CLASSB_WDG_HB_CLASSB heartbeat source
#define CLASSB_WDG_HB_REQUIRED (CLASSB_WDG_HB_TASK_LOOP)  //| CLASSB_WDG_HB_CLASSB)

/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */

/* Exported functions ------------------------------------------------------- */

void WDG_Disable(void);
bool WDG_IsTesting(void);
void WDG_MarkExpectedReset(void);
void WDG_ClearExpectedReset(void);
bool WDG_IsExpectedReset(void);
void WDG_ResetDiagnosticsInit(void);
void WDG_RequireHeartbeat(bool Require);
void WDG_SetHeartbeat(uint32_t SourceMask);

#ifdef WATCHDOG_PROTECTED

void WDG__Refresh(void);

#endif /* WATCHDOG_PROTECTED */

#endif /* __WATCHDOG_H */
