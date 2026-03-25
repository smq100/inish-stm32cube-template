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
 * @file    stm32l1xx_cpustartGCC.s
 * @brief   CPU classB startup tests
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

  .global  asm_StartUpCPUTest
  .global  conAA
  .global  con55

/**
 ******************************************************************************
 * @brief   Cortex-M0 CPU test during start-up
 *          If possible, branches are 16-bit only in dependence on BL instruction
 *          relative offset
 *          Test jumps directly to a Fail Safe routine in case of failure
 *          All registers are destroyed when exiting this function (including
 *          preserved registers R4 to R11) and excluding stack point R13
 * @param   None
 * @retval : CPUTEST_SUCCESS (=1) if test is ok
*/

  .section  .text.__TEST_PATTERNS
  .align 4
  .type  __TEST_PATTERNS, %object
  .size  __TEST_PATTERNS, .-__TEST_PATTERNS

__TEST_PATTERNS:
conAA:      .long     0xAAAAAAAA
con55:      .long     0x55555555
con80:      .long     0x80000000
conA8:      .long     0xAAAAAAA8
con54:      .long     0x55555554

  .section  .text.asm_StartUpCPUTest
  .type  asm_StartUpCPUTest, %function
  .size  asm_StartUpCPUTest, .-asm_StartUpCPUTest

asm_StartUpCPUTest:
    PUSH {R4-R6}               /* Safe critical registers */

    MOVS R0, #0
    UXTB R0,R0
    ADDS R0,#0                 /* Set Z(ero) Flag */
    BNE CPUTestFail            /* Fails if Z clear */
    BMI CPUTestFail            /* Fails if N is set */
    SUBS R0,#1                 /* Set N(egative) Flag */
    BPL CPUTestFail            /* Fails if N clear */
    ADDS R0,#2                 /* Set C(arry) Flag and do not set Z */
    BCC CPUTestFail            /* Fails if C clear */
    BEQ CPUTestFail            /* Fails if Z is set */
    BMI CPUTestFail            /* Fails if N is set */

    LDR R0,=con80              /* Prepares Overflow test */
    LDR R0,[R0]
    ADDS R0, R0, R0            /* Set V(overflow) Flag */
    BVC CPUTestFail            /* Fails if V clear */

    /*  This is for control flow test (ENTRY point) */
    LDR R0,=ClassBData
    /* Assumes R1 OK If not, error will be detected by R1 test and Ctrl flow test later on */
    LDR R1,[R0]
    ADDS R1,R1,#3	           /* ClassBData[0] += 3 (FLOW_START_CPU2) */
    STR R1,[R0]

    /* Register R1 */
    LDR R0, =conAA
    LDR R1,[R0]
    LDR R0,[R0]
    CMP R0,R1
    BNE CPUTestFail
    LDR R0, =con55
    LDR R1,[R0]
    LDR R0,[R0]
    CMP R0,R1
    BNE CPUTestFail
    MOVS R1, #1              /* For ramp test */

    /* Register R2 */
    LDR R0, =conAA
    LDR R2,[R0]
    LDR R0,[R0]
    CMP R0,R2
    BNE CPUTestFail
    LDR R0, =con55
    LDR R2,[R0]
    LDR R0,[R0]
    CMP R0,R2
    BNE CPUTestFail
    MOVS R2, #2              /* For ramp test */

    /* Register R3 */
    LDR R0, =conAA
    LDR R3,[R0]
    LDR R0,[R0]
    CMP R0,R3
    BNE CPUTestFail
    LDR R0, =con55
    LDR R3,[R0]
    LDR R0,[R0]
    CMP R0,R3
    BNE CPUTestFail
    MOVS R3, #3              /* For ramp test */

    /* Register R4 */
    LDR R0, =conAA
    LDR R4,[R0]
    LDR R0,[R0]
    CMP R0,R4
    BNE CPUTestFail
    LDR R0, =con55
    LDR R4,[R0]
    LDR R0,[R0]
    CMP R0,R4
    BNE CPUTestFail
    MOVS R4, #4              /* For ramp test */

    /* Register R5 */
    LDR R0, =conAA
    LDR R5,[R0]
    LDR R0,[R0]
    CMP R0,R5
    BNE CPUTestFail
    LDR R0, =con55
    LDR R5,[R0]
    LDR R0,[R0]
    CMP R0,R5
    BNE CPUTestFail
    MOVS R5, #5              /* For ramp test */

    /* Register R6 */
    LDR R0, =conAA
    LDR R6,[R0]
    LDR R0,[R0]
    CMP R0,R6
    BNE CPUTestFail
    LDR R0, =con55
    LDR R6,[R0]
    LDR R0,[R0]
    CMP R0,R6
    BNE CPUTestFail
    MOVS R6, #6              /* For ramp test */

    /* Register R7 */
    LDR R0, =conAA
    LDR R7,[R0]
    LDR R0,[R0]
    CMP R0,R7
    BNE CPUTestFail
    LDR R0, =con55
    LDR R7,[R0]
    LDR R0,[R0]
    CMP R0,R7
    BNE CPUTestFail
    MOVS R7, #7              /* For ramp test */

    /* Register R8 */
    LDR R0, =conAA
    LDR R0,[R0]
    MOV R8,R0
    CMP R0,R8
    BNE CPUTestFail
    LDR R0, =con55
    LDR R0,[R0]
    MOV R8,R0
    CMP R0,R8
    BNE CPUTestFail
    MOVS R0, #8             /* For ramp test */
    MOV	R8,R0
    BAL CPUTstCont

CPUTestFail:
    LDR R0,=ClassBDataInv
    LDR R1,[R0]
    SUBS R1,R1,#3	             /* ClassBDataInv -= 3 (FLOW_START_CPU2) */
    STR R1,[R0]

    POP {R4-R6}                /* Restore critical registers */
    MOVS R0, #0                /* false */
    BX LR                      /* return 'false' to the caller */

CPUTstCont:
    /* Register R9 */
    LDR R0, =conAA
    LDR R0,[R0]
    MOV R9,R0
    CMP R0,R9
    BNE CPUTestFail
    LDR R0, =con55
    LDR R0,[R0]
    MOV R9,R0
    CMP R0,R9
    BNE CPUTestFail
    MOVS R0, #9             /* For ramp test */
    MOV	R9,R0

    /* Register R10 */
    LDR R0, =conAA
    LDR R0,[R0]
    MOV R10,R0
    CMP R0,R10
    BNE CPUTestFail
    LDR R0, =con55
    LDR R0,[R0]
    MOV R10,R0
    CMP R0,R10
    BNE CPUTestFail
    MOVS R0, #10            /* For ramp test */
    MOV	R10,R0

    /* Register R11 */
    LDR R0, =conAA
    LDR R0,[R0]
    MOV R11,R0
    CMP R0,R11
    BNE CPUTestFail
    LDR R0, =con55
    LDR R0,[R0]
    MOV R11,R0
    CMP R0,R11
    BNE CPUTestFail
    MOVS R0, #11             /* For ramp test */
    MOV	R11,R0

    /* Register R12 */
    LDR R0, =conAA
    LDR R0,[R0]
    MOV R12,R0
    CMP R0,R12
    BNE CPUTestFail
    LDR R0, =con55
    LDR R0,[R0]
    MOV R12,R0
    CMP R0,R12
    BNE CPUTestFail
    MOVS R0, #12             /* For ramp test */
    MOV	R12,R0
    LDR R0, =CPUTstCont

    /* Ramp pattern verification (R0 is not tested) */
    CMP R1, #1
    BNE CPUTestFail
    CMP R2, #2
    BNE CPUTestFail
    CMP R3, #3
    BNE CPUTestFail
    CMP R4, #4
    BNE CPUTestFail
    CMP R5, #5
    BNE CPUTestFail
    CMP R6, #6
    BNE CPUTestFail
    CMP R7, #7
    BNE CPUTestFail
    MOVS R0, #8
    CMP R0,R8
    BNE CPUTestFail
    MOVS R0, #9
    CMP R0,R9
    BNE CPUTestFail
    MOVS R0, #10
    CMP R0,R10
    BNE CPUTestFail
    MOVS R0, #11
    CMP R0,R11
    BNE CPUTestFail
    MOVS R0, #12
    CMP R0,R12
    BNE CPUTestFail

    /* Process Stack pointer (banked Register R13) */
    MRS R0,PSP                 /* Save process stack value */
    LDR R1, =conA8             /* Test is different (PSP is word aligned, 2 LSB cleared) */
    LDR R1,[R1]
    MSR PSP,R1                 /* load process stack value */
    MRS R2,PSP                 /* Get back process stack value */
    CMP R2,R1                  /* Verify value */
    BNE CPUTestFail
    LDR R1, =con54             /* Test is different (PSP is word aligned, 2 LSB cleared) */
    LDR R1,[R1]
    MSR PSP,R1                 /* load process stack value */
    MRS R2,PSP                 /* Get back process stack value */
    CMP R2,R1                  /* Verify value */
    BNE CPUTestFail
    MSR PSP, R0                /* Restore process stack value */

    /* Stack pointer (Register R13) */
    MRS R0,MSP                 /* Save stack pointer value */
    LDR R1, =conA8             /* Test is different (SP is word aligned, 2 LSB cleared) */
    LDR R1,[R1]
    MSR MSP,R1                 /* load SP value */
    MRS R2,MSP                 /* Get back SP value */
    CMP R2,R1                  /* Verify value */
    BNE CPUTestFail
    LDR R1, =con54
    LDR R1,[R1]                /* load SP value */
    MSR MSP,R1                 /* Get back SP value */
    MRS R2,MSP                 /* Verify value */
    CMP R2,R1
    BNE CPUTestFail
    MSR MSP,R0                 /* Restore stack pointer value */

    /* Link register R14 cannot be tested an error should be detected by Ctrl flow test later */

    /* Control flow test (EXIT point) */
    LDR R0,=ClassBDataInv
    LDR R1,[R0]
    SUBS R1,R1,#3	             /* ClassBDataInv -= 3 (FLOW_START_CPU2) */
    STR R1,[R0]

    POP {R4-R6}                /* Restore critical registers */
    MOVS R0, #1                /* true */
    BX LR                      /* return 'true' to the caller */
