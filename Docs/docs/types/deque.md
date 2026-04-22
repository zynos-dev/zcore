# `Deque<T>`

## Purpose

`Deque<T>` is an allocator-backed growable ring-buffer container with explicit front/back operations.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- Growth failures are recoverable (`Status`/`Result`), not fatal.
- Reallocation preserves logical element order.
- `operator[]` requires `index < Size()`.
- `Front`/`Back` require non-empty state.
- `Deque<T>` is move-only.

## API Summary

- Construction: `Deque()`, `Deque(Allocator&)`, `TryWithCapacity(Allocator&, usize)`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Access: `TryAt`, `Front`, `Back`, `operator[]`, `begin/end`, `cbegin/cend`.
- Growth/mutation: `TryReserve`, `TryPushBack`, `TryPushFront`, `TryEmplaceBack`, `TryEmplaceFront`.
- Removal: `TryPopBack`, `TryPopFront`, `TryPopBackValue`, `TryPopFrontValue`, `Clear`, `Reset`.

Public include:

```cpp
#include <zcore/deque.hpp>
```

## Usage Example

```cpp
zcore::Deque<int> values(allocator);
values.TryPushBack(2);
values.TryPushFront(1);
```

## Warnings and Edge Cases

- Unbound deques (`Deque()`) cannot allocate; reserve/push operations return allocator `UNSUPPORTED_OPERATION`.
- Reallocation requires `T` to be nothrow-move-constructible.
- `Reset` deallocates owned storage through the bound allocator.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows element type, allocator behavior, and external synchronization policy.
- Container owns constructed elements and allocator-owned storage.

