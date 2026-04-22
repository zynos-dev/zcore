# `FixedVector<T, N>`

## Purpose

`FixedVector<T, N>` is a deterministic contiguous container with compile-time capacity and no heap allocation.

## Invariants and Contracts

- Capacity is fixed at `N`; growth beyond `N` is forbidden.
- Element storage is in-place; ownership and destruction are explicit.
- `PushBack`/`EmplaceBack` on full vector are contract violations.
- `Front`/`Back`/`operator[]` require valid non-empty/in-range preconditions.
- `Try*` APIs provide fallible non-terminating alternatives.

## API Summary

- Capacity/state: `Capacity`, `Size`, `RemainingCapacity`, `Empty`, `Full`.
- Access: `Data/data`, `begin/end`, `TryAt`, `Front`, `Back`, `operator[]`.
- Mutation: `TryPushBack`, `PushBack`, `TryEmplaceBack`, `EmplaceBack`.
- Removal: `TryPopBack`, `PopBack`, `TryPopBackValue`, `Clear`.
- View adapters: `AsSlice`, `AsSliceMut`, implicit `Slice<const T>` conversion.

Public include:

```cpp
#include <zcore/fixed_vector.hpp>
```

## Usage Examples

```cpp
zcore::FixedVector<int, 4> values;
values.PushBack(1);
values.EmplaceBack(2);

for (int value : values) {
  // deterministic contiguous iteration
}
```

```cpp
zcore::FixedVector<int, 2> values;
if (!values.TryPushBack(9)) {
  // at capacity
}
```

## Warnings and Edge Cases

- `N == 0` is valid; vector is always full and always empty.
- `TryPopBackValue` requires move-constructible element type.
- `AsSliceMut` follows normal mutable aliasing/lifetime rules of backing container.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread safety follows element type and external synchronization policy.
- Container owns its in-place elements only; no external resource ownership is implied.

