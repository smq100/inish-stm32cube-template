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

 @file    watchdog.c
 @brief   Source file for the watchdog module
 @author  Steve Quinlan (Design Solutions, Inc.)
 @date    2026-Apr

 ******************************************************************************/

#define WATCHDOG_PROTECTED

#include "main.h"
#include "task.h"
#include "classb.h"
#include "classb_params.h"
#include "eeprom_mcu.h"
#include "watchdog.h"
#include "log.h"
#include "rtc_app.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

// static const char* _Module = "WDG";

static const uint32_t _ExpectedMagic = 0x57444754u;  // 'WDGT'
static bool _Disabled = false;
static uint32_t _HeartbeatMask = 0;
static bool _Starve = false;  ///< Force IWDG reset

/* Private function prototypes -----------------------------------------------*/
/* Public Implementation -----------------------------------------------------*/
/* Private Implementation -----------------------------------------------------*/
/* Protected Implementation -----------------------------------------------------*/

/*******************************************************************/
/*!
 @brief   Disables IWDG for testing purposes
 @param   None
 @return  None
*******************************************************************/
void WDG_Disable(void)
{
  printf("Disabled IWDG. Waiting for reset...\r\n");
  _Disabled = true;
}

/*******************************************************************/
/*!
 @brief   Checks if IWDG test is in progress
 @param   None
 @return  true if IWDG test is in progress, false otherwise
*******************************************************************/
bool WDG_IsTesting(void)
{
  return _Disabled;
}

/*******************************************************************/
/*!
 @brief   Marks that the next IWDG reset is expected from ClassB startup test
 @return  None
*******************************************************************/
void WDG_MarkExpectedReset(void)
{
  RTC_BkupWrite(RTC_WDGEXPECT_MARKER_REG, _ExpectedMagic);
  RTC_BkupWrite(RTC_WDGEXPECT_MARKERINV_REG, ~_ExpectedMagic);

  // Make sure marker writes complete before intentionally expiring IWDG.
  __DSB();
  __ISB();
}

/*******************************************************************/
/*!
 @brief   Clears expected-IWDG-reset marker
 @return  None
*******************************************************************/
void WDG_ClearExpectedReset(void)
{
  RTC_BkupWrite(RTC_WDGEXPECT_MARKER_REG, 0u);
  RTC_BkupWrite(RTC_WDGEXPECT_MARKERINV_REG, 0u);
}

/*******************************************************************/
/*!
 @brief   Returns true if expected-IWDG-reset marker is valid
 @return  Marker validity
*******************************************************************/
bool WDG_IsExpectedReset(void)
{
  uint32_t marker = RTC_BkupRead(RTC_WDGEXPECT_MARKER_REG);
  uint32_t marker_inv = RTC_BkupRead(RTC_WDGEXPECT_MARKERINV_REG);

  return (marker == _ExpectedMagic) && (marker_inv == ~_ExpectedMagic);
}

/*******************************************************************/
/*!
 @brief   Classifies IWDG reset cause as expected (startup test) or unexpected
 @return  None
*******************************************************************/
void WDG_ResetDiagnosticsInit(void)
{
  uint32_t reset_cause = GetResetCause();
  bool iwdg_was_reset = ((reset_cause & RCC_CSR_IWDGRSTF) != 0u);
  bool iwdg_expected = WDG_IsExpectedReset();
  bool iwdg_test_inprogress = (ClassB_GetStartupStatus(eClassBStartItem_IWDG) == TEST_CLASSB_INPROGRESS);
  tTask last_task = eTask_NUM;
  const char* last_task_name = NULLPTR;
  bool has_task_breadcrumb = TASK_GetLastRunningBeforeReset(&last_task, &last_task_name);

  if (iwdg_was_reset)
  {
    if (has_task_breadcrumb)
    {
      printf("IWDG breadcrumb: last task=%s (%u)\r\n", last_task_name, (unsigned)last_task);
    }
    else
    {
      printf("IWDG breadcrumb: last task=unknown\r\n");
    }
  }

  if (iwdg_was_reset && iwdg_expected && iwdg_test_inprogress)
  {
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_WdgResetExpected);
    printf("ClassB WDG reset classified as EXPECTED (startup test)\r\n");
  }
  else if (iwdg_was_reset)
  {
    EEPROM_MCU_IncrementReg(eEEPROM_Reg_WdgResetUnexpected);
    printf("*** Unexpected IWDG reset (marker=%u, startup_iwdg=%u)\r\n",
           iwdg_expected ? 1u : 0u,
           iwdg_test_inprogress ? 1u : 0u);
  }
  else if (iwdg_expected && !iwdg_test_inprogress)
  {
    // Marker present without a matching startup-IWDG in-progress state indicates stale data.
    printf("*** Clearing stale expected-IWDG marker\r\n");
  }

  if ((iwdg_was_reset || iwdg_expected) && !iwdg_test_inprogress)
  {
    // Keep marker in place across the intentional startup reset path and clear otherwise.
    WDG_ClearExpectedReset();
  }

  if (iwdg_was_reset)
  {
    TASK_ClearLastRunningBeforeReset();
  }
}

/*******************************************************************/
/*!
 @brief   Records a watchdog heartbeat from a system source
 @param   SourceMask: OR-mask of CLASSB_WDG_HB_* sources
 @return  None
*******************************************************************/
void WDG_Heartbeat(uint32_t SourceMask)
{
  if (_Starve)
  {
    _HeartbeatMask = 0;
  }
  else
  {
    _HeartbeatMask |= SourceMask;
  }
}

/*******************************************************************/
/*!
 @brief   Returns true when all required watchdog heartbeat sources are present
 @return  Feed permission
*******************************************************************/
bool WDG_CanRefresh(void)
{
  bool can_refresh = ((_HeartbeatMask & CLASSB_WDG_HB_REQUIRED) == CLASSB_WDG_HB_REQUIRED);

  if (can_refresh)
  {
    // Clear only required heartbeat bits after one successful refresh opportunity.
    _HeartbeatMask &= ~CLASSB_WDG_HB_REQUIRED;
  }

  return can_refresh;
}

/* Private Implementation ----------------------------------------------------*/

#ifdef WATCHDOG_PROTECTED

#endif /* WATCHDOG_PROTECTED */
