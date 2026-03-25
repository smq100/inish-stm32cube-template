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
 * @file    stm32l1xx_ramMcMxGCC.s
 * @brief   CPU classB tests
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

  .global  asm_FullRamMarchC
  .global  asm_TranspRamMarchCXStep

  /* reference to C variables for control flow monitoring
     eClassBVar_CtrlFlowCnt is index 0, so array base is the target word */
  .extern ClassBData
  .extern ClassBDataInv
  .extern RAMRunFault

/* tables with offsets of physical order of address in RAM */
  .section  .text.__STANDARD_RAM_ORDER
  .align 4
  .type  __STANDARD_RAM_ORDER, %object
  .size  __STANDARD_RAM_ORDER, .-__STANDARD_RAM_ORDER
__STANDARD_RAM_ORDER:
        .word  -4
        .word   0
        .word   4
        .word   8
        .word   12
        .word   16
        .word   20
        .word   24
        .word   28

  .section  .text.__ARTISAN_RAM_ORDER
  .align 4
  .type  __ARTISAN_RAM_ORDER, %object
  .size  __ARTISAN_RAM_ORDER, .-__ARTISAN_RAM_ORDER
__ARTISAN_RAM_ORDER:
        .word  -8
        .word   0
        .word   4
        .word   12
        .word   8
        .word   16
        .word   20
        .word   28
        .word   24

/**
 ******************************************************************************
 * @brief   Full RAM MarchC test for start-up
 *          WARNING > All the RAM area including stack is destroyed during this test
 * Compilation paramater : ARTISAN - changes order of the sequence of tested
 *                                   addresses to respect their physical order
 * @param   R0 .. RAM begin (first address to check),
 *          R1 .. RAM end (last address to check)
 *          R2 .. Background pattern
 *          > R3 .. Inverted background pattern (local)
 *          > R4 .. result status (local)
 *          > R5  .. pointer to RAM (local)
 *          > R6  .. content RAM to compare (local)
 * @retval : TEST_SUCCESSFULL(=1) TEST_ERROR(=0)
*/

  .section  .text.asm_FullRamMarchC
  .type  asm_FullRamMarchC, %function
  .size  asm_FullRamMarchC, .-asm_FullRamMarchC

asm_FullRamMarchC:
  MOVS  R4, #0x1       /* Test success status by default */

  MOVS  R3,R2          /* setup inverted background pattern */
  RSBS  R3, R3, #0
  SUBS  R3,R3, #1

/* *** Step 1 *** */
/* Write background pattern with addresses increasing */
  MOVS  R5,R0
__FULL1_LOOP:
  CMP   R5,R1
  BHI   __FULLSTEP_2
  STR   R2,[R5, #+0]
  ADDS  R5,R5,#+4
  B     __FULL1_LOOP

/* *** Step 2 *** */
/* Verify background and write inverted background with addresses increasing */
__FULLSTEP_2:
  MOVS  R5,R0
__FULL2_LOOP:
  CMP   R5,R1
  BHI   __FULLSTEP_3
  LDR   R6,[R5,#+0]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+0]
  LDR   R6,[R5,#+4]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+4]
.ifdef ARTISAN
  LDR   R6,[R5,#+12]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+12]
  LDR   R6,[R5,#+8]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+8]
.else
  LDR   R6,[R5,#+8]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+8]
  LDR   R6,[R5,#+12]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+12]
.endif /* ARTISAN */
  ADDS  R5,R5,#+16
  B     __FULL2_LOOP

/* *** Step 3 *** */
/* Verify inverted background and write background with addresses increasing   */
__FULLSTEP_3:
  MOVS  R5,R0
__FULL3_LOOP:
  CMP   R5,R1
  BHI   __FULLSTEP_4
  LDR   R6,[R5,#+0]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+0]
  LDR   R6,[R5,#+4]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+4]
.ifdef ARTISAN
  LDR   R6,[R5,#+12]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+12]
  LDR   R6,[R5,#+8]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+8]
.else
  LDR   R6,[R5,#+8]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+8]
  LDR   R6,[R5,#+12]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+12]
.endif /* ARTISAN */
  ADDS  R5,R5,#+16
  B     __FULL3_LOOP

/* *** Step 4 *** */
/* Verify background and write inverted background with addresses decreasing */
__FULLSTEP_4:
  MOVS  R5,R1
  SUBS  R5,R5,#+15
__FULL4_LOOP:
  CMP   R5,R0
  BLO   __FULLSTEP_5
.ifdef ARTISAN
  LDR   R6,[R5,#+8]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+8]
  LDR   R6,[R5,#+12]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+12]
.else
  LDR   R6,[R5,#+12]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+12]
  LDR   R6,[R5,#+8]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+8]
.endif /* ARTISAN */
  LDR   R6,[R5,#+4]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+4]
  LDR   R6,[R5,#+0]
  CMP   R6,R2
  BNE   __FULL_ERR
  STR   R3,[R5,#+0]
  SUBS  R5,R5,#+16
  B     __FULL4_LOOP

/* *** Step 5 *** */
/* Verify inverted background and write background with addresses decreasing */
__FULLSTEP_5:
  MOVS  R5,R1
  SUBS  R5,R5,#+15
__FULL5_LOOP:
  CMP   R5,R0
  BLO   __FULLSTEP_6
.ifdef ARTISAN
  LDR   R6,[R5,#+8]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+8]
  LDR   R6,[R5,#+12]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+12]
.else
  LDR   R6,[R5,#+12]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+12]
  LDR   R6,[R5,#+8]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+8]
.endif /* ARTISAN */
  LDR   R6,[R5,#+4]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+4]
  LDR   R6,[R5,#+0]
  CMP   R6,R3
  BNE   __FULL_ERR
  STR   R2,[R5,#+0]
  SUBS  R5,R5,#+16
  B     __FULL5_LOOP

/* *** Step 6 *** */
/* Verify background with addresses increasing */
__FULLSTEP_6:
  MOVS  R5,R0
__FULL6_LOOP:
  CMP   R5,R1
  BHI   __FULL_RET
  LDR   R6,[R5,#+0]
  CMP   R6,R2
  BNE   __FULL_ERR
  ADDS  R5,R5,#+4
  B     __FULL6_LOOP

__FULL_ERR:
  MOVS  R4,#0       /* error result */

__FULL_RET:
  MOVS  R0,R4
  BX    LR          /* return to the caller */

/**
 ******************************************************************************
 * @brief   Transparent RAM MarchC-/March X test for run time
 *          WARNING - original content of the area under test is not valid during the test!
 *          Neighbour addresses (first-1 or -2 and last+1) are tested, too.
 * Compilation paramaters : ARTISAN - changes order of the sequence of tested
 *                                    addresses to respect their physical order
 *                  USE_MARCHX_TEST - Skip step 3 and 4 of March C- to make the test
 *                                    shorter and faster but less efficient overall
 * @param   R0 .. RAM begin (first address to check),
 *          R1 .. Buffer begin (First address of backup buffer)
 *          R2 .. Background pattern
 *          > R4 .. pointer to physical address order (local)
 * @retval : TEST_SUCCESSFULL(=1) TEST_ERROR(=0)
*/

  .section  .text.asm_TranspRamMarchCXStep
  .type  asm_TranspRamMarchCXStep, %function
  .size  asm_TranspRamMarchCXStep, .-asm_TranspRamMarchCXStep

asm_TranspRamMarchCXStep:
  PUSH  {R4-R7}

  LDR   R5,=ClassBData  /* Control flow control */
  LDR   R6,[R5]
  ADDS  R6,R6,#7 // Must match FLOW_RUNTIME_RAM2 in STL_params.h
  STR   R6,[R5]

  MOVS  R3,R2               /* setup inverted background pattern (R3) */
  RSBS  R3, R3, #0
  SUBS  R3,R3, #1

.ifdef ARTISAN
  LDR   R4, =__ARTISAN_RAM_ORDER /* setup pointer to physical order of the addresses (R4) */
.else
  LDR   R4, =__STANDARD_RAM_ORDER
.endif /* ARTISAN */

  MOVS  R5,R0       /* backup buffer to be tested? */
  CMP   R5,R1
  BEQ   __BUFF_TEST

/* ***************** test of the RAM slice ********************* */
  MOVS  R5, #0       /* NO - save content of the RAM slice into the backup buffer */
__SAVE_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* load data from RAM */
  ADDS  R5,R5,#4     /* original data are stored starting from second item of the buffer */
  STR   R7,[R1, R5]  /* (first and last items are used for testing purpose exclusively) */
  CMP   R5, #20
  BLE   __SAVE_LOOP

/* *** Step 1 *** */
/* Write background pattern with addresses increasing */
  MOVS  R5, #0
__STEP1_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  STR   R2,[R0, R6]  /* store background pattern */
  ADDS  R5,R5,#4
  CMP   R5, #20
  BLE   __STEP1_LOOP

/* *** Step 2 *** */
/* Verify background and write inverted background with addresses increasing */
  MOVS  R5, #0
__STEP2_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify background pattern */
  CMP   R7, R2
  BNE   __STEP_ERR
  STR   R3,[R0, R6]  /* store inverted background pattern */
  ADDS  R5,R5,#4
  CMP   R5, #20
  BLE   __STEP2_LOOP

.ifndef USE_MARCHX_TEST
/* *** Step 3 *** (not used at March-X test)  */
/* Verify inverted background and write background with addresses increasing */
  MOVS  R5, #0
__STEP3_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify inverted background pattern */
  CMP   R7, R3
  BNE   __STEP_ERR
  STR   R2,[R0, R6]  /* store background pattrern */
  ADDS  R5,R5,#4
  CMP   R5, #20
  BLE   __STEP3_LOOP

/* *** Step 4 *** (not used at March-X test) */
/* Verify background and write inverted background with addresses decreasing */
  MOVS  R5, #24
__STEP4_LOOP:
  SUBS  R5,R5,#4
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify background pattern */
  CMP   R7, R2
  BNE   __STEP_ERR
  STR   R3,[R0, R6]  /* store inverted background pattrern */
  CMP   R5, #0
  BHI   __STEP4_LOOP
.endif /* March-X  */

/* *** Step 5 *** */
/* Verify inverted background and write background with addresses decreasing  */
  MOVS  R5, #24
__STEP5_LOOP:
  SUBS  R5,R5,#4
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify inverted background pattern */
  CMP   R7, R3
  BNE   __STEP_ERR
  STR   R2,[R0, R6]  /* store background pattrern */
  CMP   R5, #0
  BHI   __STEP5_LOOP

/* *** Step 6 *** */
/* Verify background with addresses increasing */
  MOVS  R5, #0
__STEP6_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify background pattern */
  CMP   R7, R2
  BNE   __STEP_ERR
  ADDS  R5,R5,#4
  CMP   R5, #20
  BLE   __STEP6_LOOP

  MOVS  R5, #24      /* restore content of the RAM slice back from the backup buffer */
__RESTORE_LOOP:
  LDR   R7,[R1, R5]  /* (first and last items are used for testing purpose exclusively) */
  SUBS  R5,R5,#4     /* original data are stored starting from second item of the buffer */
  LDR   R6,[R4, R5]  /* load data offset */
  STR   R7,[R0, R6]  /* load data from RAM */
  CMP   R5, #0
  BHI   __RESTORE_LOOP

  B     __MARCH_RET

/* ************** test of the buffer itself ******************** */
__BUFF_TEST:
/* *** Step 1 ***  */
/* Write background pattern with addresses increasing */
  MOVS  R5, #4
__BUFF1_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  STR   R2,[R0, R6]  /* store background pattern */
  ADDS   R5,R5,#4
  CMP   R5, #32
  BLE   __BUFF1_LOOP

/* *** Step 2 *** */
/* Verify background and write inverted background with addresses increasing */
  MOVS  R5, #4
__BUFF2_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify background pattern */
  CMP   R7, R2
  BNE   __STEP_ERR
  STR   R3,[R0, R6]  /* store inverted background pattern */
  ADDS  R5,R5,#4
  CMP   R5, #32
  BLE   __BUFF2_LOOP

.ifndef USE_MARCHX_TEST
/* *** Step 3 *** (not used at March-X test) */
/* Verify inverted background and write background with addresses increasing */
  MOVS  R5, #4
__BUFF3_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify inverted background pattern */
  CMP   R7, R3
  BNE   __STEP_ERR
  STR   R2,[R0, R6]  /* store  background pattern */
  ADDS  R5,R5,#4
  CMP   R5, #32
  BLE   __BUFF3_LOOP

/* *** Step 4 *** (not used at March-X test) */
/* Verify background and write inverted background with addresses decreasing */
  MOVS  R5, #36
__BUFF4_LOOP:
  SUBS  R5,R5,#4
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify background pattern */
  CMP   R7, R2
  BNE   __STEP_ERR
  STR   R3,[R0, R6]  /* store inverted background pattrern */
  CMP   R5, #4
  BHI   __BUFF4_LOOP
.endif /* March-X  */

/* *** Step 5 *** */
/* Verify inverted background and write background with addresses decreasing */
  MOVS  R5, #36
__BUFF5_LOOP:
  SUBS  R5,R5,#4
  LDR   R6,[R4, R5]  /* load data offset  */
  LDR   R7,[R0, R6]  /* verify inverted background pattern */
  CMP   R7, R3
  BNE   __STEP_ERR
  STR   R2,[R0, R6]  /* store background pattrern */
  CMP   R5, #4
  BHI   __BUFF5_LOOP

/* *** Step 6 *** */
/* Verify background with addresses increasing */
  MOVS  R5, #4
__BUFF6_LOOP:
  LDR   R6,[R4, R5]  /* load data offset */
  LDR   R7,[R0, R6]  /* verify background pattern */
  CMP   R7, R2
  BNE   __STEP_ERR
  ADDS  R5,R5,#4
  CMP   R5, #32
  BLE   __BUFF6_LOOP

__MARCH_RET:
  LDR   R4,=ClassBDataInv  /* Control flow control */
  LDR   R5,[R4]
  SUBS  R5,R5,#7 // Must match FLOW_RUNTIME_RAM2 in STL_params.h
  STR   R5,[R4]

  MOVS  R0, #1       /* Correct return */
  B     __STEP_RET

__STEP_ERR:
  MOVS  R0, #0       /* error result */

__STEP_RET:
  LDR R1, =RAMRunFault
  LDR R1, [R1]             /* Load the value of CPURunFault */
  CMP R1, #0
  BEQ __RET          /* If CPURunFault > 0, fail the test */
  MOVS  R0, #0       /* error result */

__RET:
  POP   {R4-R7}
  BX    LR           /* return to the caller */
