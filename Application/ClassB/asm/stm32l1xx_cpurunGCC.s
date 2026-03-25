/*!
 ******************************************************************************
 * Copyright (c) 2026 Inish Corporation
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
 * @file    stm32l1xx_cpurunGCC.s
 * @brief   CPU classB runtime tests
 * @author  Steve Quinlan
 *          Borrowed heavily from STMicroelectronics Class B library
 *          https://www.st.com/en/embedded-software/stm32-classb-spl.html
 * @date    2026-March
 *
 ******************************************************************************/

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

  /* reference to C variables for control flow monitoring
     eClassBVar_CtrlFlowCnt is index 0, so array base is the target word */
  .extern ClassBData
  .extern ClassBDataInv
  .extern CPURunFault

  .global  asm_RuntimeCPUTest

/**
 ******************************************************************************
 * @brief   Cortex-M4 CPU test during run-time
 *          If possible, branches are 16-bit only in dependence on BL instruction
 *          relative offset
 *          Test jumps directly to a Fail Safe routine in case of failure
 * @param   None
 * @retval : CPUTEST_SUCCESS (=1) if test is ok
*/

  .section  .text.__TEST_PATTERNS
  .align 4
  .type  __TEST_PATTERNS, %object
  .size  __TEST_PATTERNS, .-__TEST_PATTERNS

__TEST_PATTERNS:
conAA:      .long     conAA
con55:      .long     con55

  .section  .text.asm_RuntimeCPUTest
  .type  asm_RuntimeCPUTest, %function
  .size  asm_RuntimeCPUTest, .-asm_RuntimeCPUTest

asm_RuntimeCPUTest:
/* M0+ cannot push high registers together; spill manually */
    PUSH {R4, R5, R6, R7}
    SUB SP, #16
    MOV R0, R8
    STR R0, [SP, #0]
    MOV R0, R9
    STR R0, [SP, #4]
    MOV R0, R10
    STR R0, [SP, #8]
    MOV R0, R11
    STR R0, [SP, #12]

/* This is for control flow test (ENTRY point) */
    LDR R0,=ClassBData
/* Assumes R1 OK; If not, error will be detected by R1 test and Ctrl flow test later on */
    LDR R1,[R0]
    ADDS R1,R1,#0x3	 /* ClassBData[0] += OxO3 */
    STR R1,[R0]
    MOVS R0, #0x0            /* For ramp test */

/* Register R1 */
    LDR R0, =conAA
    MOV R1, R0
    CMP R1, R0
    BEQ skip_fail_5
    B   fail_return
skip_fail_5:
    LDR R0, =con55
    MOV R1, R0
    CMP R1, R0
    BEQ skip_fail_6
    B   fail_return
skip_fail_6:
    MOVS R1, #0x01           /* For ramp test */

/* Register R2 */
    LDR R0, =conAA
    MOV R2, R0
    CMP R2, R0
    BEQ skip_fail_7
    B   fail_return
skip_fail_7:
    LDR R0, =con55
    MOV R2, R0
    CMP R2, R0
    BEQ skip_fail_8
    B   fail_return
skip_fail_8:
    MOVS R2, #0x02           /* For ramp test */

/* Register R3 */
    LDR R0, =conAA
    MOV R3, R0
    CMP R3, R0
    BEQ skip_fail_9
    B   fail_return
skip_fail_9:
    LDR R0, =con55
    MOV R3, R0
    CMP R3, R0
    BEQ skip_fail_10
    B   fail_return
skip_fail_10:
    MOVS R3, #0x03           /* For ramp test */

/* Register R4 */
    LDR R0, =conAA
    MOV R4, R0
    CMP R4, R0
    BEQ skip_fail_11
    B   fail_return
skip_fail_11:
    LDR R0, =con55
    MOV R4, R0
    CMP R4, R0
    BEQ skip_fail_12
    B   fail_return
skip_fail_12:
    MOVS R4, #0x04           /* For ramp test */

/* Register R5 */
    LDR R0, =conAA
    MOV R5, R0
    CMP R5, R0
    BEQ skip_fail_13
    B   fail_return
skip_fail_13:
    LDR R0, =con55
    MOV R5, R0
    CMP R5, R0
    BEQ skip_fail_14
    B   fail_return
skip_fail_14:
    MOVS R5, #0x05           /* For ramp test */

/* Register R6 */
    LDR R0, =conAA
    MOV R6, R0
    CMP R6, R0
    BEQ skip_fail_15
    B   fail_return
skip_fail_15:
    LDR R0, =con55
    MOV R6, R0
    CMP R6, R0
    BEQ skip_fail_16
    B   fail_return
skip_fail_16:
    MOVS R6, #0x06           /* For ramp test */

/* Register R7 */
    LDR R0, =conAA
    MOV R7, R0
    CMP R7, R0
    BEQ skip_fail_17
    B   fail_return
skip_fail_17:
    LDR R0, =con55
    MOV R7, R0
    CMP R7, R0
    BEQ skip_fail_18
    B   fail_return
skip_fail_18:
    MOVS R7, #0x07           /* For ramp test */

/* Register R8 */
    LDR R0, =conAA
    MOV R8, R0
    CMP R8, R0
    BEQ skip_fail_19
    B   fail_return
skip_fail_19:
    LDR R0, =con55
    MOV R8, R0
    CMP R8, R0
    BEQ skip_fail_20
    B   fail_return
skip_fail_20:
    MOVS R0, #0x08           /* For ramp test */
    MOV  R8, R0

/* Register R9 */
    LDR R0, =conAA
    MOV R9, R0
    CMP R9, R0
    BEQ skip_fail_21
    B   fail_return
skip_fail_21:
    LDR R0, =con55
    MOV R9, R0
    CMP R9, R0
    BEQ skip_fail_22
    B   fail_return
skip_fail_22:
    MOVS R0, #0x09           /* For ramp test */
    MOV  R9, R0

/* Register R10 */
    LDR R0, =conAA
    MOV R10, R0
    CMP R10, R0
    BEQ skip_fail_23
    B   fail_return
skip_fail_23:
    LDR R0, =con55
    MOV R10, R0
    CMP R10, R0
    BEQ skip_fail_24
    B   fail_return
skip_fail_24:
    MOVS R0, #0x0A           /* For ramp test */
    MOV  R10, R0

/* Register R11 */
    LDR R0, =conAA
    MOV R11, R0
    CMP R11, R0
    BEQ skip_fail_25
    B   fail_return
skip_fail_25:
    LDR R0, =con55
    MOV R11, R0
    CMP R11, R0
    BEQ skip_fail_26
    B   fail_return
skip_fail_26:
    MOVS R0, #0x0B           /* For ramp test */
    MOV  R11, R0

/* Register R12 */
    LDR R0, =conAA
    MOV R12, R0
    CMP R12, R0
    BEQ skip_fail_27
    B   fail_return
skip_fail_27:
    LDR R0, =con55
    MOV R12, R0
    CMP R12, R0
    BEQ skip_fail_28
    B   fail_return
skip_fail_28:
    MOVS R0, #0x0C           /* For ramp test */
    MOV  R12, R0

    MOVS R0, #0x0           /* re-load R0 */

/* Ramp pattern verification */
    B   ramp_checks
ramp_checks:
    CMP R0, #0x00
    BEQ ramp_1
    B   fail_return
ramp_1:
    CMP R1, #0x01
    BEQ ramp_2
    B   fail_return
ramp_2:
    CMP R2, #0x02
    BEQ ramp_3
    B   fail_return
ramp_3:
    CMP R3, #0x03
    BEQ ramp_4
    B   fail_return
ramp_4:
    CMP R4, #0x04
    BEQ ramp_5
    B   fail_return
ramp_5:
    CMP R5, #0x05
    BEQ ramp_6
    B   fail_return
ramp_6:
    CMP R6, #0x06
    BEQ ramp_7
    B   fail_return
ramp_7:
    CMP R7, #0x07
    BEQ ramp_8
    B   fail_return
ramp_8:
    MOVS R0, #0x08
    CMP R8, R0
    BEQ ramp_9
    B   fail_return
ramp_9:
    MOVS R0, #0x09
    CMP R9, R0
    BEQ ramp_10
    B   fail_return
ramp_10:
    MOVS R0, #0x0A
    CMP R10, R0
    BEQ ramp_11
    B   fail_return
ramp_11:
    MOVS R0, #0x0B
    CMP R11, R0
    BEQ ramp_12
    B   fail_return
ramp_12:
    MOVS R0, #0x0C
    CMP R12, R0
    BEQ inject_error
    B   fail_return

inject_error:
    LDR R0, =CPURunFault
    LDR R0, [R0]             /* Load the value of CPURunFault */
    CMP R0, #0
    BNE fail_return          /* If CPURunFault > 0, fail the test */

success_return:
    LDR R0, [SP, #0]
    MOV R8, R0
    LDR R0, [SP, #4]
    MOV R9, R0
    LDR R0, [SP, #8]
    MOV R10, R0
    LDR R0, [SP, #12]
    MOV R11, R0
    ADD SP, #16
    POP {R4, R5, R6, R7}

    LDR R0,=ClassBDataInv
    LDR R1,[R0]
    SUBS R1,R1,#0x3	/* ClassBDataInv[0] -= OxO3 */
    STR R1,[R0]

    MOVS R0, #0x1       /* CPUTEST_SUCCESS */
    BX LR               /* return to the caller */

fail_return:
    LDR R0, [SP, #0]
    MOV R8, R0
    LDR R0, [SP, #4]
    MOV R9, R0
    LDR R0, [SP, #8]
    MOV R10, R0
    LDR R0, [SP, #12]
    MOV R11, R0
    ADD SP, #16
    POP {R4, R5, R6, R7}

    LDR R0,=ClassBDataInv
    LDR R1,[R0]
    SUBS R1,R1,#0x3	/* ClassBDataInv[0] -= OxO3 */
    STR R1,[R0]

    MOVS R0, #0x0       /* CPUTEST_FAIL */
    BX LR               /* return to the caller */
