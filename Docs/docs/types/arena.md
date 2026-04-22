# `Arena`

## Purpose

`Arena` is a fixed-capacity bump-pointer allocator over caller-owned backing storage.

## Invariants and Contracts

- Arena never performs system allocation.
- Arena is non-copyable and move-only.
- Allocation succeeds only when enough aligned capacity remains.
- `Reset()` releases all arena allocations in bulk by rewinding the cursor.
- `Deallocate(...)` is shape/ownership validation + no-op for owned blocks.
- Foreign or malformed allocations return allocator-domain `INVALID_ALLOCATION`.
- Error reporting is delegated to callers; use `HandleIfError(...)` to forward arena failures to handlers.

## API Summary

- Construction: `Arena()`, `Arena(ByteSpanMut)`, `Arena(std::array<Byte, N>&)`.
- Ownership: move-constructible and move-assignable; copy disabled.
- Capacity/state: `Capacity()`, `Used()`, `Remaining()`, `IsEmpty()`.
- Ownership: `Owns(const Byte*)`.
- Lifecycle: `Reset()`.
- Allocator interface:
  - `Allocate(AllocationRequest)`
  - `Deallocate(Allocation)`
  - inherited `AllocateBytes(size, alignment)`.

Public include:

```cpp
#include <zcore/arena.hpp>
```

## Usage Examples

```cpp
std::array<zcore::Byte, 1024> storage{};
zcore::Arena arena(storage);

auto block = arena.AllocateBytes(128U, 16U);
if (block.HasValue()) {
  arena.Reset();
}
```

## Warnings and Edge Cases

- Per-allocation free is not supported; `Deallocate` does not reclaim cursor space.
- Zero-size allocation returns `Allocation::Empty()`.
- Alignment padding consumes arena capacity.
- Moved-from arenas become empty (zero-capacity) handles.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Backing storage ownership remains with caller.
- Thread-safety follows external synchronization policy.
