# `BTreeMap<KeyT, ValueT, CompareT>`

## Purpose

`BTreeMap` is an allocator-backed ordered associative container with deterministic sorted contiguous storage.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- Entries are maintained in strict key order under `CompareT`.
- Key uniqueness is enforced by comparator-equivalence (`!comp(a,b) && !comp(b,a)`).
- `Get(key)` requires `Contains(key)`.
- `BTreeMap<KeyT, ValueT, CompareT>` is move-only.

## API Summary

- Construction: `BTreeMap()`, `BTreeMap(Allocator&)`, `TryWithCapacity(Allocator&, usize)`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Iteration: `begin/end`, `cbegin/cend` (entries in sorted key order).
- Lookup: `Contains`, `TryGet`, `Get`.
- Growth/mutation: `TryReserve`, `TryInsert`, `TryInsertOrAssign`.
- Removal: `TryRemove`, `TryRemoveValue`, `Clear`, `Reset`.

Public include:

```cpp
#include <zcore/btree_map.hpp>
```

## Usage Example

```cpp
zcore::BTreeMap<int, int> values(allocator);
values.TryInsert(7, 11);
values.TryInsert(3, 5);
for (const auto& entry : values) {
  // entry.key visits 3, then 7
}
```

## Warnings and Edge Cases

- Unbound maps (`BTreeMap()`) cannot allocate; reserve/insert returns allocator `UNSUPPORTED_OPERATION`.
- Mid-sequence insertion/removal shifts contiguous entries to preserve key order.
- Comparator must satisfy strict-weak ordering for deterministic behavior.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows key/value types, allocator behavior, and external synchronization policy.
- Container owns constructed entries and allocator-owned contiguous storage.
