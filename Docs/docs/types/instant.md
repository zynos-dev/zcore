# `Instant`

## Purpose

`Instant` is a deterministic absolute time point stored as signed nanoseconds.

## Invariants and Contracts

- Value representation: signed 64-bit nanoseconds (`i64`).
- Point shift operations (`Instant ± Duration`) are saturating.
- Point difference (`Instant - Instant`) returns `Duration` with saturation.
- No allocation, no hidden synchronization.

## API Summary

- Construction: `Instant()`, `Instant::FromNanoseconds(...)`.
- Sentinels: `Zero()`, `Min()`, `Max()`.
- Accessors: `AsNanoseconds`, explicit conversion to `i64`.
- State helper: `IsZero`.
- Operators: `==`, `<=>`, `+/- Duration`, `+=`, `-=`, `Instant - Instant -> Duration`.

Public include:

```cpp
#include <zcore/instant.hpp>
```

## Usage Example

```cpp
const zcore::Instant start = zcore::Instant::FromNanoseconds(1000);
const zcore::Instant end = start + zcore::Duration::FromNanoseconds(250);
const zcore::Duration elapsed = end - start;
```

## Warnings and Edge Cases

- Overflow during point shifts and point-difference clamps to min/max representable range.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.
