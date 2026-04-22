# `TraceEvent`

## Purpose

`TraceEvent` defines a stable structured diagnostics payload for tracing timelines and counters.

## Invariants and Contracts

- `TraceEventType` enum ordering is stable: `INSTANT`, `DURATION_BEGIN`, `DURATION_END`, `COUNTER`.
- `TraceEventTypeName(type)` returns canonical uppercase names.
- Invalid enum values map to `"UNKNOWN"` and fail `IsValidTraceEventType(type)`.
- `timestampNs` is caller-defined nanosecond timestamp under caller-selected clock.
- String fields are non-owning pointers; caller owns lifetime.

## API Summary

- Enum: `TraceEventType`.
- Payload: `struct TraceEvent`.
- Helpers: `MakeTraceEvent`, `TraceEventTypeName`, `IsValidTraceEventType`.

Public include:

```cpp
#include <zcore/trace_event.hpp>
```

## Usage Example

```cpp
const zcore::TraceEvent event = zcore::MakeTraceEvent(
    zcore::TraceEventType::INSTANT,
    "runtime",
    "tick",
    1234U);
```

## Warnings and Edge Cases

- Out-of-range cast values are invalid and intentionally reported as `"UNKNOWN"`.

## Thread-Safety and Ownership Notes

- Trivially copyable value type; thread behavior follows normal value semantics.
