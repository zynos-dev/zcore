# `LogLevel`

## Purpose

`LogLevel` defines canonical ordered log severities for diagnostics pipelines.

## Invariants and Contracts

- Enum ordering is stable and monotonic by severity (`TRACE`..`FATAL`).
- `LogLevelName(level)` returns canonical uppercase names.
- Invalid enum values map to `"UNKNOWN"` and fail `IsValidLogLevel(level)`.
- `LogLevelAtLeast(candidate, minimum)` uses ordinal severity ordering.

## API Summary

- Enum: `LogLevel::{TRACE, DEBUG, INFO, WARN, ERROR, FATAL}`.
- Helpers: `LogLevelName`, `IsValidLogLevel`, `LogLevelAtLeast`.

Public include:

```cpp
#include <zcore/log_level.hpp>
```

## Usage Example

```cpp
if (zcore::LogLevelAtLeast(eventLevel, zcore::LogLevel::WARN)) {
  // emit high-priority path
}
```

## Warnings and Edge Cases

- Casted out-of-range values are invalid and intentionally reported as `"UNKNOWN"`.

## Thread-Safety and Ownership Notes

- Pure value enum + constexpr helpers; no internal state or ownership.
