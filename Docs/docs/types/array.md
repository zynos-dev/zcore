# `Array<T, N>`

## Purpose

`Array<T, N>` is a deterministic fixed-extent owning contiguous container.

## Invariants and Contracts

- Extent is compile-time fixed at `N`; size never changes.
- Storage is inline-owned; no heap allocation.
- `operator[]` requires `index < N`.
- `Front`/`Back` require `N > 0`.
- `N == 0` is valid and behaves as always-empty.

## API Summary

- Shape/state: `Extent`, `Size/size`, `Empty/empty`.
- Access: `Data/data`, `begin/end`, `cbegin/cend`, `TryAt`, `Front`, `Back`, `operator[]`.
- Mutation: `Fill`.
- View adapters: `AsSlice`, `AsSliceMut`, implicit `Slice<const T>` conversion.

Public include:

```cpp
#include <zcore/array.hpp>
```

## Usage Example

```cpp
zcore::Array<int, 3> values;
values.Fill(1);
values[2] = 7;
zcore::Slice<const int> view = values;
```

## Warnings and Edge Cases

- Out-of-range indexing is a contract violation.
- `Front`/`Back` on `Array<T, 0>` are contract violations.
- `TryAt` is the non-terminating bounds-checked access path.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows element type and external synchronization policy.
- Container owns inline elements only.
