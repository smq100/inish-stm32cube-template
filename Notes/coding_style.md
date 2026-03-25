# APP C Coding Style Guide (Derived from template.c/h)

Use this guide when creating new C modules or refactoring legacy modules to match project style.

## Scope

- Applies to paired module files in `Application/inc/*.h` and `Application/src/*.c`.
- Primary source of truth is the structure and naming shown in `template.c` and `template.h`.

## File Naming and Module Identity

- Use lowercase snake_case file names: `module_name.c` and `module_name.h`.
- Public symbol prefix should be uppercase module token + underscore, matching the module identity from the template:
  - Example pattern: `TEMPLATE_Function(...)`
  - Real modules should follow `MODULE_Function(...)` (replace `MODULE` with actual module prefix).
- Keep one logical module per `.c/.h` pair.

## Required File Header Block

Every `.c` and `.h` file starts with the same multi-line Doxygen-style header block:

- Copyright paragraph.
- Confidentiality paragraph.
- `@file` with exact filename.
- `@brief` single-line description.
- `@author`.
- `@date`.

Use the same banner style and alignment as the template.

## Header Guard and Include Order

- Header guards use uppercase token form:
  - `#ifndef __MODULE_H`
  - `#define __MODULE_H`
  - `#endif /* __MODULE_H */`
- In headers, include only required dependencies (template uses `common.h`).
- In source files, include order is:
  1. `main.h`
  2. matching module header

## Section Layout in .c Files

Keep these section separators and order, even when some sections are empty:

1. `/* Private typedef -----------------------------------------------------------*/`
2. `/* Private define ------------------------------------------------------------*/`
3. `/* Private macro -------------------------------------------------------------*/`
4. `/* Public variables ----------------------------------------------------------*/`
5. `/* Private variables ---------------------------------------------------------*/`
6. `/* Private function prototypes -----------------------------------------------*/`
7. `/* Public Implementation -----------------------------------------------------*/`
8. `/* Private Implementation -----------------------------------------------------*/`
9. `/* Protected Implementation -----------------------------------------------------*/` (if used)

## Section Layout in .h Files

Keep these section separators and order:

1. `/* Exported types ------------------------------------------------------------*/`
2. `/* Exported constants --------------------------------------------------------*/`
3. `/* Exported macros ------------------------------------------------------------*/`
4. `/* Exported vars ------------------------------------------------------------ */`
5. `/* Exported functions ------------------------------------------------------- */`

Place public enums in the `Exported types` section.

Example pattern from `template.h`:

```c
typedef enum
{
  eTemplate_ID1,
  eTemplate_ID2,
  eTemplate_ID_Num
} tTemplate_ID;
```

## Naming Rules

- Public functions: `MODULE_FunctionName` (PascalCase after module prefix).
- Public enum types: `tModule_Name`.
- Public enum values: `eModule_Name`.
- Use a trailing `_Num` enumerator for the count/size sentinel when the enum represents a bounded list, matching the template pattern:
  - Example: `eTemplate_ID_Num`
- Private file-scope helpers: leading underscore + PascalCase, and declared `static`:
  - Example: `_PrivateFunction`
- Private file-scope variables: leading underscore + PascalCase, and declared `static`:
  - Example: `_Private`
- Function parameters: PascalCase in prototypes/definitions:
  - Example: `uint32_t Param`
- Local variables: lowercase snake_case:
  - Example: `uint32_t local_var`
- Separate engineering units from variable and function names using an underscore unit suffix:
  - Variable example: `_SleepTime_ms`
  - Function example: `POWER_GetSleepTime_ms`
  - Additional examples: `_Delay_us`, `temperature_c`, `period_ticks`
- Macro symbols and guard tokens: uppercase with underscores.
- typedef enum and struct type begin with a lower-case `t` type name
- enum elements begin with a lower-case `e`

## Scope and Visibility

- Public API:
  - Declared in module `.h`.
  - Defined in module `.c` without `static`.
- Private (file-local):
  - Prototypes and definitions in `.c`.
  - Must use `static`.
- Protected API (optional):
  - Controlled with module-level macro guard, following template pattern:
    - `#define MODULE_PROTECTED` in `.c` when enabled.
    - `#ifdef MODULE_PROTECTED` around protected declarations and definitions.
  - Comment protected functions as limited-scope, not general API.

## Function Comment Blocks

Before each non-trivial function, use the template-style Doxygen comment block:

- Include the full upper and lower `*` borders, matching `template.c`.
- `@brief`
- `@param`
- `@retval`

Keep the decorative separator lines as shown in template files.

Example pattern:

```c
/*******************************************************************/
/*!
 @brief Template public function
 @param Param Description of the parameter
 @retval Description of the return value
********************************************************************/
```

## Formatting

- Use two-space indentation inside function bodies.
- Place opening brace on its own line for function definitions.
- Keep a blank line between logical steps in functions.
- Use end-of-`#endif` comments:
  - `#endif /* MODULE_PROTECTED */`
  - `#endif /* __MODULE_H */`

## Refactor Checklist (Copilot)

When refactoring a non-conforming module, apply this order:

1. Rename files to lowercase snake_case where needed.
2. Normalize header banner and metadata tags.
3. Enforce section comment layout/order in `.c` and `.h`.
4. Normalize symbol naming by scope:
   - Public: `MODULE_*`
   - Private: `static` + leading underscore names
   - Locals: lowercase snake_case
5. Move declarations to correct visibility section (public vs private vs protected).
6. Add or normalize Doxygen blocks for public and private functions.
7. Verify header guards and `#endif` comments.

## Notes

- Prefer minimal diffs when restyling to avoid behavioral regressions.
- Do not change hardware behavior while performing style-only refactors.