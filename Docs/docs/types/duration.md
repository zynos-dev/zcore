# `Duration`

## Purpose

`Duration` is a deterministic signed time-span value stored as nanoseconds.

## Invariants and Contracts

- Value representation: signed 64-bit nanoseconds (`i64`).
- Unit factories (`FromMicroseconds`/`FromMilliseconds`/`FromSeconds`/`FromMinutes`/`FromHours`) are saturating.
- Arithmetic (`+`, `-`, unary `-`, `+=`, `-=`) is saturating.
- Unit accessors (`AsMicroseconds`/`AsMilliseconds`/...) truncate toward zero.
- No allocation, no hidden synchronization.

## API Summary

- Construction: `Duration()`, `Duration::FromNanoseconds(...)`.
- Unit factories: `FromMicroseconds`, `FromMilliseconds`, `FromSeconds`, `FromMinutes`, `FromHours`.
- Sentinels: `Zero()`, `Min()`, `Max()`.
- Accessors: `AsNanoseconds`, `AsMicroseconds`, `AsMilliseconds`, `AsSeconds`, `AsMinutes`, `AsHours`.
- State helpers: `IsZero`, `IsPositive`, `IsNegative`, `Abs`.
- Operators: `==`, `<=>`, unary `-`, `+`, `-`, `+=`, `-=`.

Public include:

```cpp
#include <zcore/duration.hpp>
```

## Usage Example

```cpp
const zcore::Duration frame = zcore::Duration::FromMilliseconds(16);
const zcore::Duration budget = frame + zcore::Duration::FromMilliseconds(1);
```

## Warnings and Edge Cases

- Saturation clamps to `Duration::Min()`/`Duration::Max()` on overflow.
- Conversion accessors are integer-truncating; they do not round.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.
