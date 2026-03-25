# STM32L073RZTX Linker Script Modifications for Class B Library

## Overview
This document lists the **Class B required linker modifications** and the corresponding implementation in `STM32L073RZTX_FLASH_ClassB.ld`.
Explanations are intentionally brief.

---

## Required Changes Summary

| Change | Location in Linker Script | Brief Class B Reason |
|--------|----------------------------|----------------------|
| Add ROM boundary symbol (`_rom_start`) | Start of `SECTIONS`, before `.isr_vector` | Defines flash CRC start address for Class B ROM test. |
| Keep `.check_sum` in FLASH with 64-byte alignment | After `.fini_array` and before `.data` | Holds reference CRC word used by startup/runtime flash integrity checks. |
| Add RAM boundary symbols (`_ram_start`, `_ram_end`) | After `.data` section | Defines March RAM test range boundaries. |
| Add dedicated Class B RAM sections | Before `.bss` | Isolates safety variables/buffers from normal application RAM usage. |
| Add stack pattern section (`.stack_bottom`) | Before `.bss` | Supports stack overflow monitoring for Class B diagnostics. |
| Ensure Class B symbols are exported with `PROVIDE` | In `.class_b_ram` / `.class_b_ram_rev` | Gives Class B code deterministic linker symbols for bounds checking. |

---

## Current Implementation Mapping (from `STM32L073RZTX_FLASH_ClassB.ld`)

| Required Mod | Current Script Status | Current Value / Section |
|--------------|-----------------------|-------------------------|
| ROM start symbol | Implemented | `PROVIDE( _rom_start = ORIGIN(FLASH) );` |
| CRC reference symbol | Implemented | `PROVIDE( _Check_Sum = . );` in `.check_sum` |
| Checksum placeholder | Implemented | `LONG(0xFFFFFFFF)` |
| RAM bounds symbols | Implemented | `_ram_start`, `_ram_end` |
| Runtime RAM buffer section | Implemented | `.runtime_ram_buf` |
| Class B normal vars section | Implemented | `.class_b_ram` + `__class_b_ram_start__/__class_b_ram_end__` |
| Class B inverted vars section | Implemented | `.class_b_ram_rev` + `__class_b_ram_rev_start__/__class_b_ram_rev_end__` |
| Stack pattern section | Implemented | `.stack_bottom` |
| Pointer section `.run_time_ram_pnt` | Not present in current linker script | N/A |

---

## Section Order (Current)

| Order | Section / Symbol | Memory |
|------:|------------------|--------|
| 1 | `_rom_start` | FLASH symbol |
| 2 | `.isr_vector` | FLASH |
| 3 | `.text` | FLASH |
| 4 | `.rodata` | FLASH |
| 5 | `.ARM.extab`, `.ARM`, `.preinit_array`, `.init_array`, `.fini_array` | FLASH |
| 6 | `_sidata = LOADADDR(.data)` | linker symbol |
| 7 | `.check_sum` | FLASH |
| 8 | `.data` | RAM (`AT> FLASH`) |
| 9 | `_ram_start`, `_ram_end` | RAM symbols |
| 10 | `.runtime_ram_buf`, `.class_b_ram`, `.class_b_ram_rev`, `.stack_bottom` | RAM |
| 11 | `.bss` | RAM |
| 12 | `._user_heap_stack` | RAM |

---

## Notes

| Item | Current Value | Comment |
|------|---------------|---------|
| Heap size | `_Min_Heap_Size = 0x200` | Standard project setting. |
| Stack size | `_Min_Stack_Size = 0x400` | Current script value (not `0x800`). |
| Runtime buffer naming | `.runtime_ram_buf` | Uses `runtime` naming (not `.run_time_ram_buf`). |

---

## Quick Verification Checklist

| Check | Expected |
|-------|----------|
| `_rom_start` exists | Present in map/symbols |
| `_Check_Sum` exists | Present in `.check_sum` |
| `.check_sum` alignment | 64-byte aligned |
| Class B RAM sections | Located before `.bss` |
| `_ram_start`/`_ram_end` | Present and valid for RAM range |

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.1 | 2026-02-25 | Reworked to concise tabular format with brief Class B explanations and current linker-script mapping |

