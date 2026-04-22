# `Timestamp`

## Purpose

`Timestamp` is a deterministic unsigned nanosecond time point for absolute stamped events.

## Invariants and Contracts

- Value representation: unsigned 64-bit nanoseconds (`u64`).
- Point shifts with `Duration` (`Timestamp +/- Duration`) are saturating to `[0, Timestamp::Max()]`.
- Point difference (`Timestamp - Timestamp`) returns `Duration` with saturation.
- No allocation, no hidden synchronization.

## API Summary

- Construction: `Timestamp()`, `Timestamp::FromNanoseconds(...)`.
- Sentinels: `Zero()`, `Max()`.
- Accessors: `AsNanoseconds`, explicit conversion to `u64`.
- State helper: `IsZero`.
- Operators: `==`, `<=>`, `+/- Duration`, `+=`, `-=`, `Timestamp - Timestamp -> Duration`.

Public include:

```cpp
#include <zcore/timestamp.hpp>
```

## Usage Example

```cpp
const zcore::Timestamp t0 = zcore::Timestamp::FromNanoseconds(1000);
const zcore::Timestamp t1 = t0 + zcore::Duration::FromNanoseconds(250);
const zcore::Duration dt = t1 - t0;
```

## Warnings and Edge Cases

- Negative duration shifts at zero clamp to zero.
- Difference magnitudes above `i64` range clamp to `Duration::Min()`/`Duration::Max()`.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.

