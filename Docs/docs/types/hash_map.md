# `HashMap<KeyT, ValueT>`

## Purpose

`HashMap` is an allocator-backed associative container using deterministic open addressing and linear probing.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- Buckets use power-of-two capacity with explicit load-factor growth policy.
- Rehash preserves key/value associations.
- `Get(key)` requires `Contains(key)`.
- `HashMap<KeyT, ValueT>` is move-only.

## API Summary

- Construction: `HashMap()`, `HashMap(Allocator&)`, `TryWithCapacity(Allocator&, usize)`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Lookup: `Contains`, `TryGet`, `Get`.
- Growth/mutation: `TryReserve`, `TryInsert`, `TryInsertOrAssign`.
- Removal: `TryRemove`, `TryRemoveValue`, `Clear`, `Reset`.

Public include:

```cpp
#include <zcore/hash_map.hpp>
```

## Usage Example

```cpp
zcore::HashMap<int, int> values(allocator);
values.TryInsert(7, 11);
if (int* found = values.TryGet(7)) {
  *found = 13;
}
```

## Warnings and Edge Cases

- Unbound maps (`HashMap()`) cannot allocate; reserve/insert returns allocator `UNSUPPORTED_OPERATION`.
- Rehash requires nothrow move construction for key and mapped types.
- `Reset` deallocates owned bucket storage through the bound allocator.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows key/value types, allocator behavior, and external synchronization policy.
- Container owns constructed entries and allocator-owned bucket storage.

