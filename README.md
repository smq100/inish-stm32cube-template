# inish-stm32cube-template

STM32CubeIDE template project for STM32Cube targets, with a reusable application structure, task framework, hardware drivers, and Class B safety support.

The code provides a core set of functionality including:
- Cooperative multitasking with task priorities, testing, shutdown pricessing, and metrics
- UL 60730 ClassB testing and runtime support
- Multi-level logging
- Data acquisition support
- Technician serial support
- Extensive LED support (GPIO)
- Extensive button support (GPIO)

This repository is intended to be copied and adapted for new applications while keeping a consistent baseline architecture and coding style.

## What This Template Includes

- STM32Cube-generated core startup and HAL integration.
- Application framework split into modules, tasks, and drivers.
- Class B support files and linker/startup assets.
- Debug build artifacts and makefile output (under `Debug/`).
- Project notes for architecture and implementation details (under `Notes/`).

## Project Layout

- `Core/`
	- Cube-generated MCU setup and interrupt handlers.
	- Device startup assembly in `Core/Startup/`.
- `Application/`
	- `src/` and `inc/`: shared app utilities (queue, timer, power, log, etc.).
	- `Tasks/`: task-level behavior modules (app, daq, serial, system, tech, etc.).
	- `Drivers/`: board/application peripheral wrappers (UART, EEPROM, ADC, etc.).
	- `ClassB/`: runtime/startup safety test support.
	- `App/`: product-specific app layer area.
- `Drivers/`
	- STM32 HAL and CMSIS vendor stacks.
- `Notes/`
	- Internal design notes and implementation references.
- Root project files
	- `.ioc`, linker scripts, launch config, helper scripts, and workspace file.

## Prerequisites

- STM32CubeIDE compatible with STM32L1 series.
- ST-Link (or equivalent programmer/debugger) for flashing and debugging.
- GNU Arm toolchain (normally bundled in STM32CubeIDE).

## Build and Run Basics

1. Open the project in STM32CubeIDE.
2. If prompted, trust and import as an existing project.
3. Build with the IDE build button (or use the generated make system).
4. Flash and debug using the provided launch configuration.

## Reusing This Template for a New Application

Use this checklist when starting a new product from this template.

1. Copy or clone this repository into a new folder/repository.
2. Rename project-visible identifiers from `inish-stm32cube-template` to your new project name:
	 - `.ioc` project name
	 - `.project` / `.cproject` (if present)
	 - launch configuration file
	 - workspace file (optional)
3. Keep `Core/` and vendor `Drivers/` as generated baselines unless hardware changes require regeneration.
4. Create/update app behavior in `Application/App/`, `Application/Tasks/`, and `Application/Drivers/`.
5. Review linker scripts (`*.ld`) if memory layout, bootloader offset, or Class B requirements differ.
6. Confirm interrupt/peripheral setup in the `.ioc`, then regenerate code carefully.
7. Verify boot, scheduler/task startup, comms, and watchdog behavior on hardware.

## Porting to Other STM32 MCUs

This template can be adapted to other STM32 families and part numbers.

Recommended approach:

1. Create a new STM32CubeMX/STM32CubeIDE project for the target MCU and board.
2. Configure clocks, pins, interrupts, DMA, and peripherals in CubeMX for the new hardware.
3. Generate code and confirm the fresh baseline builds and runs before migrating app code.
4. Copy application-level code from this repository into the new project, primarily:
   - `Application/src`, `Application/inc`
   - `Application/Tasks`
   - `Application/Drivers`
   - Optional: `Application/ClassB` (only if your target device/safety flow supports it)
5. Rework board- and MCU-specific drivers, pin mappings, and peripheral assumptions.
6. Update linker/startup configuration for the new memory map and startup sequence.
7. Re-test all startup, timing, comms, watchdog, and safety-critical behavior on target hardware.

Tip: avoid copying old `Core/` and HAL/CMSIS vendor files into a new MCU project. Keep the newly generated Cube baseline for the target device, then layer this template's application code on top.

## Onboarding Another Developer

For another user to work from this template:

1. Install STM32CubeIDE and required ST device support.
2. Clone the repository.
3. Open/import the project in STM32CubeIDE.
4. Build once to confirm toolchain setup.
5. Connect target hardware and run a debug session.
6. Review `Notes/` and this README for architecture context.

## Recommended First Edits in a New Product

- Update module names from `template` placeholders to product-specific names.
- Define product versioning behavior (`make_fw_version.py`, `Application/inc/version.h`).
- Configure logging/UART defaults for your board.
- Review tech/service mode behavior in task modules.

## Notes

- Avoid editing vendor HAL/CMSIS files unless absolutely necessary.
- Prefer putting custom logic under `Application/` to simplify future Cube regeneration.
- Keep project notes in `Notes/` when you add architecture decisions.
