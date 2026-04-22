# `LogSink`

## Purpose

`LogSink` defines a fallible diagnostics output interface for structured `LogRecord` emission.

## Invariants and Contracts

- `Write(const LogRecord&)` is required and returns `Status`.
- `Flush()` defaults to `OkStatus()` and may be overridden for buffered sinks.
- `LogRecord` contains explicit severity/category/message/source-location fields.
- Interface is non-copyable and non-movable.
- No hidden synchronization or ownership transfer.

## API Summary

- `struct LogRecord { level, category, message, file, line }`.
- `MakeLogRecord(...)`.
- `class LogSink { virtual Status Write(const LogRecord&) = 0; virtual Status Flush(); }`.

Public include:

```cpp
#include <zcore/log_sink.hpp>
```

## Usage Example

```cpp
const zcore::LogRecord record =
    zcore::MakeLogRecord(zcore::LogLevel::ERROR, "asset", "load failed", __FILE__, __LINE__);
sink.Write(record);
sink.Flush();
```

## Warnings and Edge Cases

- Sink-specific failures must be propagated via `Status`.
- `category/message/file` are non-owning `const char*` references; caller controls lifetime.

## Thread-Safety and Ownership Notes

- Interface imposes no internal locking.
- Concrete sinks define synchronization policy.
