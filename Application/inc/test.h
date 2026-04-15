/*!
 ******************************************************************************
 * @copyright Copyright (c) 2026 Phillips And Termo. All rights reserved.
 *
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

 @file    test.h
 @brief   Interface header file template
 @author  Steve Quinlan (Design Solutions, Inc.)
 @date    2026-April

******************************************************************************

 @file    test.h
 @brief   Header file for test code
 @author  Steve Quinlan
 @date    2026-March

 ******************************************************************************/

#ifndef __TEST_H
#define __TEST_H

//! *** Define special testing modes (should all be undefined for production builds)
// Enable logging via LOG module
// #define TEST__DISABLE_LOGGING

// Enable debugger mode, which disabled all watchdogs and classb CLK runtime tests
// This is needed if debugging via the IDE, which typically halts the CPU without resetting,
// which would cause watchdogs to trigger and interfere with debugging
// #define TEST__ENABLE_DEBUG

///< Disable IWDG functionality (refresh, init, etc). Use with caution. Only for debugging purposes.
// #define TEST__DISABLE_IWDG

#endif
