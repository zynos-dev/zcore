# `BTreeSet<ValueT, CompareT>`

## Purpose

`BTreeSet` is an allocator-backed ordered unique-value set built on `BTreeMap`.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- Values are unique under comparator-equivalence.
- Ordering follows `CompareT`.
- Growth and insertion failures are recoverable (`Result`/`Status`), not fatal.
- `BTreeSet<ValueT, CompareT>` is move-only.

## API Summary

- Construction: `BTreeSet()`, `BTreeSet(Allocator&)`, `TryWithCapacity(Allocator&, usize)`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Lookup: `Contains`.
- Mutation: `TryReserve`, `TryInsert`, `TryRemove`, `Clear`, `Reset`.

Public include:

```cpp
#include <zcore/btree_set.hpp>
```

## Usage Example

```cpp
zcore::BTreeSet<int> values(allocator);
values.TryInsert(42);
values.TryInsert(7);
```

## Warnings and Edge Cases

- Unbound sets (`BTreeSet()`) cannot allocate; reserve/insert returns allocator `UNSUPPORTED_OPERATION`.
- Comparator must satisfy strict-weak ordering and equivalence consistency.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows value type, allocator behavior, and external synchronization policy.
- Container owns allocator-backed ordered storage.
