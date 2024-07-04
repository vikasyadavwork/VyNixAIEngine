# Logging System

## Goal

Provide a simple, thread-safe logging system for the entire VyNix AI Engine.

---

## Design Goals

- Easy to use
- Thread-safe
- Cross-platform
- Extensible
- Multiple output destinations
- Zero memory leaks
- Minimal API for engine users

---

## Public API

```cpp
VX_LOG_TRACE(...)

VX_LOG_DEBUG(...)

VX_LOG_INFO(...)

VX_LOG_WARN(...)

VX_LOG_ERROR(...)

VX_LOG_FATAL(...)
```

The user should never need to create a Logger manually.

---

## Architecture

Application

↓

Logger

↓

LogMessage

↓

ILogSink

↓

ConsoleSink

↓

Terminal

Future:

↓

FileSink

↓

EditorSink

↓

NetworkSink

---

## Components

### Logger

Responsible for:

- Creating log messages
- Sending messages to sinks
- Managing sinks
- Thread safety

Logger does NOT print to the console directly.

---

### LogMessage

Contains logging information.

Initially:

- Log Level
- Message

Future:

- Timestamp
- Thread ID
- File
- Function
- Line Number

---

### LogLevel

Strongly typed enum describing the severity.

Trace

Debug

Info

Warn

Error

Fatal

---

### ILogSink

Interface implemented by every output destination.

Current implementation:

ConsoleSink

Future:

FileSink

EditorSink

DebuggerSink

RemoteSink

---

## Ownership

Logger owns all sinks.

Storage:

std::vector<std::unique_ptr<ILogSink>>

Reason:

- Automatic cleanup
- No memory leaks
- Single owner

---

## Future Improvements

- Colored console output
- File logging
- Async logging
- Log formatting
- Filters
- Categories
- Engine/Application separation