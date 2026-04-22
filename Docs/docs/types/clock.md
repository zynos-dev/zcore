# `Clock`

## Purpose

`Clock` defines an abstract instant source for monotonic or wall-clock time queries.

## Invariants and Contracts

- `Now()` returns the current `Instant` value for the implementation.
- `IsMonotonic()` reports whether `Now()` is guaranteed monotonic.
- `ElapsedSince(start)` returns `Now() - start` (inherits `Instant`/`Duration` saturation semantics).
- Interface is non-copyable and non-movable.

## API Summary

- Pure virtual: `Now()`.
- Virtual with default: `IsMonotonic()` (`false` default).
- Helper: `ElapsedSince(Instant start)`.

Public include:

```cpp
#include <zcore/clock.hpp>
```

## Usage Example

```cpp
const zcore::Instant begin = clock.Now();
const zcore::Duration elapsed = clock.ElapsedSince(begin);
```

## Warnings and Edge Cases

- Monotonicity is implementation-defined; check `IsMonotonic()` before assuming non-decreasing `Now()`.

## Thread-Safety and Ownership Notes

- Interface has no implicit synchronization.
- Ownership and thread-safety are implementation-defined.
