# Technician Interface for External Control, Data Acquisition, and Data Streaming

# 1. Overview

During the engineering development  it is necessary to provide control and data acquisition (DAQ) capabilities via an external communications port. A simple UART-based serial interface is available for this functionality.

An external serial terminal may be used to provide simple connectivity. If automated functionality is required, scripting, such as Python, may be used to provide a more complete interface. Other test and automation tools such as _LabView_ may also be used.

# 2. High-level System Architecture

The application does not include a real-time operating system (RTOS). It incorporates a "super loop" architecture that attempts to mimic the thread-based architecture of an RTOS using "tasks" that provide many of the features of RTOS threads. Tasks allow for prioritization, metrics measurement, initialization processing, and shutdown processing. Tasks provide a cooperative multi-tasking environment, but not the preemptive multitasking capabilities provided by an RTOS.

# 3. Technician Task

The Technician task provides:

- An external UART-based serial interface
- Command parsing
- Error processing
- Command execution
- Response generation

Three dedicated tasks are required for serial control and DAQ functionality: the Technician task (task_tech.c/h) and the Data Acquisition task (task_daq.c/h). The technician task uses a shared serial communication task (serial.c/h) to provide a buffered abstraction of the low-level serial UART communication capabilities.

## 3.1. Command Protocol

Commands are recognized by way of a simple protocol. The protocol used is: _\<start character\>\<command\>\<end character\>_. In this implementation the start character is an asterisk (\*) and the end character is a semicolon (;). An example of a command is:

```
*command;
```

Some commands may include optional parameters, which may be included after the command. Multiple parameters are separated by commas. _ **No spaces are allowed between parameters.** _ An example of a command with multiple parameters could be:

```
*command 1,2,3;
```
## 3.2. Response Protocol

The response uses the JSON protocol. This provides compatibility and easier implementations of external controllers and data consumers such as Python and other control and DAQ programs such as _LabView_ and the Mjölnir DAQ system.

All command responses contain the following tags:

- code: A numeric code that indicates success or errors
- status: A text description of the success or errors
- cmd: The command that was processed

Depending on the command, an additional _data_ tag may be included that provides an additional object with its own command-specific objects. The tags within the _data_ object are command specific.

An example response of a command is:

```
{"code":0,"status":"success","cmd":"sys"}
```

where _sys_ is the name of the command.

An example response to a command that returns data is:

```
{"code":0,"status":"success","cmd":"sys","data":{"sw":"0.2.1","hw":0,"temp":13.5}}
```

Here, the _data_ object contains the three tags, _sw_, _hw_, and _temp_, relevant to the _sys_ command.

The JSON responses do not include any whitespace characters.

## 3.3. Response Codes

The response codes returned are noted in the following table:

| **Code** | **Description** | " **status" text** |
| --- | --- | --- |
| 0 | Success: The command completed successfully. | "success" |
| 1 | Overflow: There was buffer overflow parsing the command. The command string was perhaps too long. This should never occur under normal circumstances. | "overflow" |
| 2 | Timeout: timeout occurred parsing the command most likely due to an incomplete command. | "timeout" |
| 3 | Unknown: An unknown command was received. | "unknown " |
| 4 | Disabled: A valid command was received and recognized but is disabled. | "disabled" |
| 5 | Processing: A valid command was accepted and processed but the command handler was not successful in completing. This could be the result of an invalid command parameter or a parameter that is out of range. | "process" |
| 6 | Streaming: A error occurred processing a stream command | "stream" |

## 3.4. Command Set

The technician commands that are currently implemented are:

| **Command** | **Description** | **Parameters** |
| --- | --- | --- |
| help | Prints a description of all available commands as raw text (not JSON) | None |
| sys | Provides a high-level description of the system including software and hardware versions | None |
| get | Reads DAQ data | n(,m): The DAQ item(s)No spaces allowed between items(See DAQ sections below for more information) |
| set | Writes DAQ data for non-read-only items | n: The DAQ itemvalue: The new value. Must be a float or an integer (See DAQ sections below for more information) |
| meta | Provides metadata for overall DAQ or for individual DAQ items | (n): Provides metadata for the specified item or, if no parameter is specified, overall metadata for the DAQ feature.(See DAQ sections below for more details) |
| sitems | Specified the DAQ streaming items | n(,m): The DAQ item(s)No spaces allowed between items(See Streaming sections below for more information) |
| sstart | Starts the streaming process | Specifies the type of streaming. Currently only _get_ is supported and "get" must be specified. |
| sstop | Stops the streaming process | None |

# 4. Data Acquisition (DAQ) Task

The data acquisition task supports the collection of internal data for the data acquisition commands of the TECH interface.

## 4.1. Metadata

Internal code modules that provide the data must register metadata with the DAQ task including the type of data and a callback function that will be called by the DAQ task to update the data.

The metadata for each DAQ item may be read with the _daqmeta_ command with the index of the DAQ item as the parameter. The metadata is contained within a meta tag in the data object in the JSON response.

An example response of the _daqmeta_ command is:

```
{"code":0,"status":"success","cmd":"daqmeta","data":{"meta":{"item":2,"enabled":1,"ro":1,"interval":2000,"name":"vref","desc":"processor vref","units":"volts"}}}
```

Issuing the _daqmeta_ without the index will return metadata regarding the interface itself including the version of the interface and the number of DAQ items available. An external program may use the number of DAQ items to iterate over each item to retrieve its index number and other metadata.

An example output of the _daqmeta_ command without an index could be:

```
{"code":0,"status":"success","cmd":"daqmeta","data":{"meta":{"items":20,"version":1}}}
```

## 4.2. Variable Registration

Modules registering with the DAQ task provide the following metadata:

- Type – Data type. The data type is a union data structure supporting most C standard types
- Enabled – Enabled if true. This is "enabled" in JSON output
- Read-Only – Read-only if true (for future use). This is "ro" in JSON output
- Modified – True if the data has been modified via the _daqset_ command. This is "mod" in JSON output. The value specified during variable registration is ignored and is set to false
- Interval – Update interval in milliseconds. The DAQ task will fetch data at this interval. This is "int" in JSON output
- Read Callback – Callback function of the module to retrieve data
- Write Callback – Callback function of the module to write data
- Units – Const pointer to string of unit description (ex: "RPM"). This is "units" in JSON output
- Description – Const pointer to string of the data description. This is "desc" in JSON output
- Name – Const pointer to name. Must be unique to all items. Attempt to keep lowercase and below 8 chars. The name is used as the JSON tag in the DAQ output
- Scale – Provides optional scaling of numeric output values. This is "scale" in JSON output
- Min – Specifies a minimum value for data modification (for future use). This is "min" in JSON output
- Max – Specifies a maximum value for data modification (for future use). This is "max" in JSON output

## 4.3. Implementation

Each exposed DAQ item must be included in the _tDAQ\_Entry_ enumeration in daq.h.

Once added to the enumeration, each module wishing to expose the item for data acquisition must register with the DAQ module using the function:

```
bool DAQ\_RegisterItem(tDAQ\_Entry Entry, tDAQ\_Config\* Config);
```

_tDAQ\_Entry_ is the index of the DAQ item and _tDAQ\_Config_ is a pointer to a structure containing the metadata described above.

Once registered, the DAQ module will call the specified read callback function at the specified interval, which will then update the registered variable.

## 4.4. Modifying Data

Variables specified as not read-only may be modified by using the _daqset_ command. The item and the new value. Only numeric entries are allowed. Values may be specified as an integer or a float.

For modules allowing variable modification a write callback must be specified during variable registered.

# 5. Streaming

The technician interface supports DAQ data streaming where data may be continuously streamed without issuing commands past the two initialization commands, _sitems_ and _sstart_.

Three commands are used to stream data:

| **Command** | **Description** | **Parameters** |
| --- | --- | --- |
| sitems | Specifies the DAQ items to be streamed | n(,m): The DAQ item(s).No spaces allowed between items |
| sstart | Starts the streaming process | Type: The type of streaming. The interface currently supports only one type of streaming: _get_. "get" must be specified as the parameter |
| sstop | Stops the streaming process | None |

## 5.1. Initialization and Starting

To start a streaming process, both the _sitems_ and _sstart_ initialization commands must be issued. Once started, the streaming will continue until the _sstop_ command is issued or an error is encountered.

The _sitems_ command provides the DAQ items to be streamed. An example use of _sitems_ could be:

```
sitems 6,7,8;
```

The response to the command is the same as any other command with the exception that a _data_ object is also provided that contains the indexes and names of the streaming items:

```
{"code":0,"status":"success","cmd":"sitems","data":{"6":"vref", "7":"vrtc","8":"vtemp"}}
```

The streaming is started by issuing the _sstart_ command with the "_get"_ parameter.

```
sstart get;
```

The order of issuing the _sitems_ and _sstart_ commands does not matter. The _sstart_ command may also be issued prior to the _sitems_ command and the streaming will start after acknowledging the _sitems_ command.

The format of the streaming output is different from the normal acknowledgment output of commands in that it does not contain _code_ or _status_ tags and it contains additional tags. An example streaming output is:

```
{"cmd":"stream","stype":"get","ts":500,"data":{"6":3.092,"7":2.858,"8":0.538}}
```

The _cmd_ tag is always "_stream_".

The _stype_ tag specifies the type of streaming, which is always "_get"_.

The _ts_ (timestamp) tag contains the number of milliseconds since the start of the stream. The time stamp always starts at 0 for each streaming operation.

The _data_ object contains the data with the item indexes used as keys.

# 6. External Interfaces

Any standard serial terminal such as _Putty_ may be used to attach to the technician interface. Interface scripting using languages such as Python may also be used to provide a more feature-rich interface. Other commercial test and data acquisition programs such as _LabView_ may also be used.

The project contains PTP files that may be used with the button based _Docklight_ serial terminal program to issue serial commands. ([https://docklight.de](https://docklight.de/))
