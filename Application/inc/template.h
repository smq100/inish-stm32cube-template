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

 @file    template.h
 @brief   Interface header file template
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __TEMPLATE_H  // TODO Rename
#define __TEMPLATE_H

#include "common.h"

/* Exported types ------------------------------------------------------------*/

// Example of a public enum type. Rename and modify as needed
typedef enum
{
  eTemplate_ID1,
  eTemplate_ID2,
  eTemplate_ID_NUM
} tTemplate_ID;

/* Exported constants --------------------------------------------------------*/
/* Exported macros ------------------------------------------------------------*/
/* Exported vars ------------------------------------------------------------ */
/* Exported functions ------------------------------------------------------- */

uint32_t TEMPLATE_Function(tTemplate_ID Param);

#ifdef TEMPLATE_PROTECTED

// Protected functions are not intended for general public consumption but used in a limited scope
uint32_t TEMPLATE__Protected(uint32_t Param);

#endif /* TEMPLATE_PROTECTED */

#endif /* __TEMPLATE_H */
