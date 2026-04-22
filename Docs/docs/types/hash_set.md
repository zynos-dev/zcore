# `HashSet<ValueT>`

## Purpose

`HashSet` is an allocator-backed unique-value set built on `HashMap` open-addressing storage.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- Value uniqueness is enforced by key-equivalence/hash contracts.
- Growth and insertion failures are recoverable (`Result`/`Status`), not fatal.
- `HashSet<ValueT>` is move-only.

## API Summary

- Construction: `HashSet()`, `HashSet(Allocator&)`, `TryWithCapacity(Allocator&, usize)`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Lookup: `Contains`.
- Mutation: `TryReserve`, `TryInsert`, `TryRemove`, `Clear`, `Reset`.

Public include:

```cpp
#include <zcore/hash_set.hpp>
```

## Usage Example

```cpp
zcore::HashSet<int> ids(allocator);
ids.TryInsert(42);
```

## Warnings and Edge Cases

- Unbound sets (`HashSet()`) cannot allocate; reserve/insert returns allocator `UNSUPPORTED_OPERATION`.
- Value type must satisfy `zcore::hash::Hashable` and equality comparability contracts.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows value type, allocator behavior, and external synchronization policy.
- Container owns allocator-backed bucket storage.

