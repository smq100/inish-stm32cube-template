# STL_StartUpCPUTest Assembly Function - Detailed Summary

## Overview
`STL_StartUpCPUTest` is a **Cortex-M0+ CPU self-diagnostic routine** that executes during system startup to verify the integrity and functionality of the CPU's core components. This function is part of STMicroelectronics' Class B Safety Library for functional safety compliance.

## Function Metadata
- **File**: `stm32l0xx_STLcpustartGCC.s`
- **Architecture**: ARM Cortex-M0+
- **Assembly Syntax**: Unified ARM/Thumb
- **Return Value**: `0x1` (CPUTEST_SUCCESS) if all tests pass, otherwise branches to `FailSafePOR`
- **Side Effects**: Destroys all registers except R13 (Stack Pointer) and R14 (Link Register)

---

## Test Phases

### 1. CPU Flags Test (Status Register Verification)
**Purpose**: Verify that the CPU's status flags (NZCV) function correctly

**Test Sequence**:
- **Zero Flag (Z)**: Sets Z flag and verifies it can be detected and cleared
- **Negative Flag (N)**: Sets N flag and verifies it can be detected and cleared
- **Carry Flag (C)**: Sets C flag and verifies it can be detected and cleared
- **Overflow Flag (V)**: Sets V flag using overflow arithmetic (0x80000000 + 0x80000000) and verifies detection

**Operations**:
```
MOVS R0, #0x00      → Clear R0 and set Z flag
SUBS R0, #1         → Set N flag (R0 becomes 0xFFFFFFFF)
ADDS R0, #2         → Set C flag
ADDS R0, R0, R0     → Set V flag (0x80000000 + 0x80000000 = overflow)
```

**Failure Points**: Any flag not behaving as expected branches to `CPUTestFail`

---

### 2. General Purpose Register Test (R1-R12)
**Purpose**: Verify that each register can accurately store and retrieve data

**Test Pattern**: Uses complementary bit patterns for thorough testing:
- **0xAAAAAAAA** (alternating 1010... in binary)
- **0x55555555** (alternating 0101... in binary)

**Test Process for Each Register (R1-R12)**:
1. Load 0xAAAAAAAA into register and verify it matches
2. Load 0x55555555 into register and verify it matches
3. Set register to a ramp value (sequential: R1=0x1, R2=0x2, ... R12=0xC)

**Example (Register R1)**:
```
LDR R0, =conAA       → Load address of 0xAAAAAAAA
LDR R1, [R0]         → Load pattern into R1
LDR R0, [R0]         → Load pattern into R0
CMP R0, R1           → Compare; if different, fail
```

**Ramp Pattern Verification**: After loading all ramp values, each register is compared against its expected sequential value to detect data path corruption

---

### 3. Stack Pointer Test
**Purpose**: Verify that both stack pointer variants work correctly

#### Process Stack Pointer (PSP)
- Save original PSP value
- Load test pattern **0xAAAAAAA8** (note: 2 LSBs cleared as PSP is word-aligned)
- Verify stored value matches
- Load test pattern **0x55555554**
- Verify stored value matches
- Restore original PSP

#### Main Stack Pointer (MSP)
- Same procedure as PSP but with MSP register
- Test patterns: 0xAAAAAAA8 and 0x55555554
- Restore original MSP after testing

**Technical Note**: Stack pointers must maintain word alignment (2 LSBs = 0), which is automatically enforced by the hardware

---

### 4. Control Flow Monitoring
**Purpose**: Detect execution anomalies or instruction skipping

**Entry Point Logic**:
```
CtrlFlowCnt += 0x3   → Increment counter at function entry
```

**Exit Point Logic**:
```
CtrlFlowCntInv -= 0x3  → Decrement inverse counter at function exit
```

**Safety Mechanism**: External monitoring code verifies that:
- `CtrlFlowCnt` increased by expected amount
- `CtrlFlowCntInv` decreased by expected amount
- The two counters maintain inverse relationship

This catches scenarios where code execution jumps or is skipped due to:
- Memory bit flips
- Electromagnetic interference
- Timing anomalies

---

## Failure Handling

### CPUTestFail Label
If any single test fails, the code branches to `CPUTestFail`:

```
CPUTestFail:
    BLAL FailSafePOR   → Branch with link to FailSafe routine
```

**FailSafePOR** is an external routine that:
- Stops normal execution
- Puts the system into a safe state
- May trigger watchdog reset or enter safe mode

This ensures no corrupted CPU can continue execution after detecting a fault.

---

## Register Usage

| Register | Purpose | Preserved? |
|----------|---------|-----------|
| R0 | Temporary scratch, loop index | No |
| R1-R7 | Test targets, ramp pattern values | No |
| R8-R12 | Test targets, ramp pattern values | No |
| R13 (SP/MSP) | Stack pointer | **Yes** |
| R14 (LR) | Return address/Link register | Tested only by control flow monitoring |

**Preserved Registers on Entry**: R4-R6 are pushed at start and popped at end via `PUSH`/`POP`

---

## Test Patterns Explained

### Pattern Constants
Located in `.text.__TEST_PATTERNS` section:

| Constant | Value | Purpose |
|----------|-------|---------|
| `conAA` | 0xAAAAAAAA | Alternating 1010... pattern |
| `con55` | 0x55555555 | Alternating 0101... pattern (complement) |
| `con80` | 0x80000000 | Sign bit set (for overflow test) |
| `conA8` | 0xAAAAAAA8 | conAA with 2 LSBs cleared (word-aligned) |
| `con54` | 0x55555554 | con55 with 2 LSBs cleared (word-aligned) |

**Why Complementary Patterns?**
- Ensures stuck-at-0 and stuck-at-1 faults are detected
- Tests both rise and fall transitions
- More comprehensive than single pattern testing

---

## Success Criteria

Function returns successfully when:
1. ✓ All CPU status flags function correctly
2. ✓ All general-purpose registers (R1-R12) pass load/store verification
3. ✓ Both stack pointers accept and maintain test patterns
4. ✓ Control flow counters increment/decrement as expected
5. ✓ No branch to `CPUTestFail` is taken

**Return Value**: `0x1` loaded into R0 and execution returns via `BX LR`

---

## Functional Safety Compliance

This test is designed for **IEC 61508** and **ISO 26262** functional safety standards, providing:

- **Periodic CPU Testing**: Can be called during startup or periodically during runtime
- **Self-Diagnostics**: Requires no external test equipment
- **Fail-Safe Design**: Any detected fault immediately invokes safe shutdown
- **Comprehensive Coverage**: Tests registers, flags, memory access, and control flow
- **Deterministic Execution**: Fixed sequence ensures reproducible results

---

## Execution Flow Diagram

```
Entry (STL_StartUpCPUTest)
    ↓
[1] Test CPU Flags (Z, N, C, V)
    ↓ (all pass?)
[2] Increment CtrlFlowCnt (+0x3)
    ↓
[3] Test Registers R1-R7 (with direct LDR)
    ↓ (all pass?)
[4] Test Registers R8-R12 (with MOV operations)
    ↓ (all pass?)
[5] Verify Ramp Pattern (0x1-0xC)
    ↓ (all pass?)
[6] Test Process Stack Pointer (PSP)
    ↓ (pass?)
[7] Test Main Stack Pointer (MSP)
    ↓ (pass?)
[8] Decrement CtrlFlowCntInv (-0x3)
    ↓
[9] Return R0 = 0x1 (SUCCESS)
    ↓
Exit (BX LR to caller)

Any failure → CPUTestFail → FailSafePOR → Safe Shutdown
```

---

## Notes

- **Inline Branches**: Uses 16-bit Thumb instructions where possible for compact code
- **No Memory Corruption**: Stack pointers are restored before exit
- **Atomic-like Behavior**: Once started, test runs to completion without interruption
- **Link Register (R14)**: Not explicitly tested; anomalies detected via control flow monitoring
- **Performance**: Deterministic execution time, suitable for startup code

