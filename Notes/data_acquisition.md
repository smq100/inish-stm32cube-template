# Data Acquisition (DAQ) Summary

This note summarizes the DAQ feature implemented in `Application/Tasks/inc/task_daq.h` and `Application/Tasks/src/task_daq.c`, and how it is used by Technician Mode (`task_tech.c`).

## 1. What DAQ is in this codebase

The DAQ task is a centralized broker for internal telemetry and controlled write access to selected values.

- It defines a fixed DAQ index list (`tDAQ_Entry`) for all exposed channels.
- Each channel has metadata (`tDAQ_Config`) plus runtime storage (`tRuntime` value array + timestamp).
- Producer modules register callbacks with DAQ.
- The DAQ task periodically calls registered read callbacks and caches the latest values.
- Consumer modules (mainly Technician Mode) read the cached values and metadata through DAQ APIs.

In short: DAQ decouples data producers from the external serial protocol.

## 2. Core DAQ features in `task_daq.c/h`

### 2.1 Static channel catalog and metadata defaults

`task_daq.h` defines the public DAQ entries:

- `eDAQ_NOP`
- `eDAQ_Test`
- `eDAQ_TempPCB`
- `eDAQ_Vref`
- `eDAQ_Uptime`
- `eDAQ_Sleep`

`task_daq.c` provides one default config per entry (`_Defaults[]`) with:

- data type (`Type`)
- update cadence (`Interval`)
- read-only flag (`ReadOnly`)
- units, name, description
- subitem count (`Items`)
- scaling/min/max placeholders

All entries start disabled until a module registers callbacks.

### 2.2 Registration model

`DAQ_RegisterItem()` enables an entry and installs callbacks.

- `All=false` (used throughout this project): keep default metadata, override callbacks only.
- `All=true`: replace full metadata with caller-provided config.

This lets DAQ own consistent item descriptions while feature modules only provide data hooks.

### 2.3 Periodic sampling scheduler

`DAQ_Exec()` is the polling engine:

- Iterate every DAQ entry.
- Skip disabled entries or those missing `CallbackRead`.
- If elapsed time >= configured interval, call read callback for each subitem.
- Callback writes value into DAQ runtime cache via `DAQ_UpdateItem()`.

DAQ itself does not compute the measurements; it schedules module callbacks and stores results.

### 2.4 Runtime read/write APIs

- `DAQ_GetMetadata(entry, &config)`: returns current metadata/state for one entry.
- `DAQ_GetItem(entry, item, &value)`: returns cached value (enabled entries only).
- `DAQ_UpdateItem(entry, item, value)`: producer callback path to update cache.
- `DAQ_WriteItem(entry, item, value)`: controlled external write path:
	- rejects invalid/disabled/read-only entries
	- requires `CallbackWrite`
	- sets `Modified=true` when successful
	- logs write attempt/result

### 2.5 Built-in DAQ channels inside DAQ task

DAQ owns two internal channels:

- `eDAQ_NOP`: no-op/stub
- `eDAQ_Test`: 1 Hz sawtooth (0.0 to 1.0) for client validation

Both are wired in `DAQ_Init()`.

### 2.6 Safety and limits

- Pointer sanity checks with `IsRAM()` and assertions.
- Entry bounds checks against `eDAQ_Entry_NUM`.
- Per-entry subitem bounds checks.
- Global subitem cap: `DAQ_MAX_SUBITEMS` (25).
- Stream-rate-related max constant exported: `DAQ_MAX_UPDATE_FREQ` (20 Hz), used by Technician Mode.

## 3. Where DAQ data comes from

Other modules register callbacks for their DAQ ownership:

- `tgen_adc.c`: Temp1/Temp2/TempPCB/Vfixed/Vref (Temp1/Temp2 writable for emulation).
- `task_system.c`: Uptime.
- `power.c`: accumulated sleep time.
- `task_classb.c`: ClassB runtime error counters (multi-subitem).

This is why DAQ can expose heterogeneous values with one common API.

## 4. How DAQ works closely with Technician Mode

Technician Mode is the primary DAQ client.

### 4.1 Read path (`meta` / `get`)

- `meta` command calls `DAQ_GetMetadata()`:
	- no parameter: DAQ interface summary (item count, protocol version)
	- with item index: detailed per-item metadata (enabled, read-only, interval, name, units, etc.)
- `get` command calls:
	- `DAQ_GetItem()` to fetch current cached data
	- `DAQ_GetMetadata()` to decorate output (`note` text, formatting context)

Technician Mode does not sample hardware directly; it reads DAQ cache.

### 4.2 Write path (`set`)

- `set` parses numeric `entry,value`.
- It checks metadata first (`DAQ_GetMetadata()` / `ReadOnly`).
- Then it calls `DAQ_WriteItem()`.

Practical effect in this firmware:

- writable DAQ items are mainly Temp1/Temp2 through ADC module write callback.
- this is used by emulation/test workflows, including technician-driven simulation.

### 4.3 Stream path (`sitems` / `sstart` / `sstop`)

Technician streaming repeatedly invokes its `get` handler at user-selected rate, but bounded by DAQ max frequency (`DAQ_MAX_UPDATE_FREQ`).

- `sitems` stores requested DAQ entry/subitem list and builds name header via `DAQ_GetMetadata()`.
- `sstart` validates streamable command and frequency, then starts periodic output.
- Runtime stream loop calls `get` per requested item and emits JSON frames with timestamp.

This makes Technician Mode a transport layer around DAQ data, not a second acquisition implementation.

### 4.4 Shared design contract

There is an explicit code-level coupling rule in `task_daq.h`:

- when new DAQ entries are added, `task_tech.c::_Handler_Get()` must also be updated for output formatting.

So DAQ and Technician Mode evolve together by design: DAQ defines what exists, Technician Mode defines how it is serialized to the external interface.

## 5. Behavior summary

Operationally, the sequence is:

1. DAQ initializes default entry table and runtime storage.
2. Feature modules register callbacks for their DAQ entries.
3. DAQ scheduler refreshes enabled items at their individual intervals.
4. Technician commands consume DAQ cache (`meta`/`get`), optionally write permitted items (`set`), and stream selected channels.

This architecture keeps hardware/data ownership in feature modules, centralizes acquisition timing and caching in DAQ, and keeps external protocol details in Technician Mode.
