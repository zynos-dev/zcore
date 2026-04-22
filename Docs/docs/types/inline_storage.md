# `InlineStorage<ValueT, CapacityV>`

## Purpose

`InlineStorage` is a fixed-capacity in-place slot storage primitive with explicit per-slot construction and destruction.

## Invariants and Contracts

- Capacity is fixed at compile time; no heap allocation.
- Type does not track which slots are live; caller owns slot lifetime protocol.
- `PtrAt`/`RefAt`/`ConstructAt`/`DestroyAt` require `index < Capacity()`.
- Copy/move is disabled to prevent accidental byte-wise ownership/lifetime duplication.

## API Summary

- Capacity/data: `Capacity()`, `Data()`, `data()`.
- Slot access: `TryPtrAt(index)`, `PtrAt(index)`, `RefAt(index)`.
- Lifetime control: `ConstructAt(index, args...)`, `DestroyAt(index)`.

Public include:

```cpp
#include <zcore/inline_storage.hpp>
```

## Usage Example

```cpp
zcore::InlineStorage<int, 4> storage;
storage.ConstructAt(0, 7);
const int value = storage.RefAt(0);
storage.DestroyAt(0);
```

## Warnings and Edge Cases

- Constructing an already-live slot or destroying an unconstructed slot is undefined behavior.
- Zero-capacity storage is valid; `Data()` and `TryPtrAt(0)` return `nullptr`.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Ownership is only for raw inline bytes; slot object lifetime is externally controlled.

