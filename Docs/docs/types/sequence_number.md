# `SequenceNumber`

## Purpose

`SequenceNumber` is a wrap-aware unsigned sequence token for ordered modular counters.

## Invariants and Contracts

- Value representation: unsigned 64-bit token (`u64`).
- Increment/decrement/advance use modulo (`u64`) wrap semantics.
- `IsNewerThan`/`IsOlderThan` use half-range modular ordering.
- Half-range distance (`2^63`) is intentionally ambiguous (`newer=false`, `older=false`).
- No allocation, no hidden synchronization.

## API Summary

- Construction: `SequenceNumber()`, `SequenceNumber::FromRawUnchecked(...)`.
- Sentinels: `Zero()`, `Max()`.
- Accessors: `Raw`, explicit conversion to `u64`, `IsZero`.
- Movement: `Next`, `Previous`, `Increment`, `Decrement`, `Advance`, `AdvancedBy`.
- Ordering helpers: `IsNewerThan`, `IsOlderThan`.
- Distance helpers: `ForwardDistanceTo`, `BackwardDistanceTo`.

Public include:

```cpp
#include <zcore/sequence_number.hpp>
```

## Usage Example

```cpp
zcore::SequenceNumber seq = zcore::SequenceNumber::Zero();
seq.Increment();
const bool newer = seq.IsNewerThan(zcore::SequenceNumber::Zero());
```

## Warnings and Edge Cases

- `operator<=>` is raw-value ordering, not wrap-aware recency ordering.
- Use `IsNewerThan`/`IsOlderThan` for protocol-style modular ordering.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.

