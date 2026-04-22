# `Vector<T>`

## Purpose

`Vector<T>` is an allocator-backed growable contiguous container with explicit fallible growth APIs.

## Invariants and Contracts

- Dynamic allocation uses the bound `Allocator` only.
- Growth and append failures are recoverable (`Status`/`Result`), not fatal.
- Reallocation preserves element order.
- `begin/end` support zero-distance arithmetic in empty state when storage is allocated (`Capacity() > 0`).
- `operator[]` requires `index < Size()`.
- `Front`/`Back` require non-empty state.
- `Vector<T>` is move-only.

## API Summary

- Construction: `Vector()`, `Vector(Allocator&)`, `TryWithCapacity(Allocator&, usize)`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Access: `Data/data`, `begin/end`, `cbegin/cend`, `TryAt`, `Front`, `Back`, `operator[]`.
- Growth/mutation: `TryReserve`, `TryPushBack`, `TryEmplaceBack`.
- Removal: `TryPopBack`, `TryPopBackValue`, `Clear`, `Reset`.
- View adapters: `AsSlice`, `AsSliceMut`, implicit `Slice<const T>` conversion.

Public include:

```cpp
#include <zcore/vector.hpp>
```

## Usage Example

```cpp
zcore::Vector<int> values(allocator);
if (values.TryPushBack(7).HasValue()) {
  values.TryPushBack(9);
}
```

## Warnings and Edge Cases

- Unbound vectors (`Vector()`) cannot allocate; reserve/append returns allocator `UNSUPPORTED_OPERATION`.
- Reallocation requires `T` to be nothrow-move-constructible.
- `Reset` deallocates owned storage through the bound allocator.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows element type, allocator implementation, and external synchronization policy.
- Container owns constructed elements and allocator-owned storage.
