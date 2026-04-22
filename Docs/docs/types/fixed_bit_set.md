# `FixedBitSet<BitCountV>`

## Purpose

`FixedBitSet` is a deterministic fixed-size inline bitset for allocation-free bit state tracking.

## Invariants and Contracts

- Bit capacity is fixed at compile time; no heap allocation.
- `Set`/`Clear`/`Toggle`/`Assign`/`Test` require `bitIndex < BitCount()`.
- `Try*` APIs provide non-terminating fallible alternatives for out-of-range indices.
- `SetAll`/`FlipAll` mask unused tail bits in the final storage word.

## API Summary

- Shape/state: `BitCount`, `WordCount`, `Empty`, `Any`, `NoneSet`, `All`, `CountSet`.
- Bit access: `Test`, `TryTest`, `Set`, `Clear`, `Toggle`, `Assign`.
- Fallible mutation: `TrySet`, `TryClear`, `TryToggle`, `TryAssign`.
- Whole-set mutation: `SetAll`, `ClearAll`, `FlipAll`.
- Raw storage view: `RawWord`, `TryRawWord`.
- Operators: `==`, `~`, `|`, `&`, `^`, `|=`, `&=`, `^=`.

Public include:

```cpp
#include <zcore/fixed_bit_set.hpp>
```

## Usage Example

```cpp
zcore::FixedBitSet<128> visible;
visible.Set(5U);
visible.Assign(9U, true);
```

## Warnings and Edge Cases

- `BitCountV == 0` is valid; set is always empty and `All()` is true.
- `TryRawWord` returns `None` for out-of-range word indices.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Ownership is inline word storage only; synchronization policy is external.

