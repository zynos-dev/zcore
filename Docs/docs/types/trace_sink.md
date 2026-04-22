# `TraceSink`

## Purpose

`TraceSink` defines a fallible sink interface for `TraceEvent` emission.

## Invariants and Contracts

- `Write(const TraceEvent&)` is required and returns `Status`.
- `Flush()` defaults to `OkStatus()` and may be overridden for buffered sinks.
- Interface is non-copyable and non-movable.
- No hidden synchronization or ownership transfer.

## API Summary

- `class TraceSink { virtual Status Write(const TraceEvent&) = 0; virtual Status Flush(); }`.

Public include:

```cpp
#include <zcore/trace_sink.hpp>
```

## Usage Example

```cpp
sink.Write(zcore::MakeTraceEvent(
    zcore::TraceEventType::COUNTER,
    "runtime",
    "jobs",
    1500U,
    3U,
    24));
sink.Flush();
```

## Warnings and Edge Cases

- Sink-specific failures must be propagated via `Status`.

## Thread-Safety and Ownership Notes

- Interface imposes no internal locking.
- Concrete sinks define synchronization policy.
