# `Owned<T>`

## Purpose

`Owned<T>` provides unique allocator-backed ownership for a single object with deterministic destroy+deallocate behavior.

## Invariants and Contracts

- `Owned<T>` is move-only (copy disabled).
- `TryMake(...)` allocates via `Allocator`, constructs `T` in-place, returns `Result<Owned<T>, Error>`.
- Empty state has no allocator/allocation.
- `Reset()` destroys `T`, then deallocates through the originating allocator.
- `Value()`/`operator*`/`operator->` require non-empty state and enforce contract checks.
- `TryMake(...)` requires `T` to be nothrow-constructible for the provided arguments.

## API Summary

- `Owned()`.
- `TryMake(Allocator&, Args...)`.
- `HasValue()`, `IsEmpty()`, `Get()`.
- `Value()`, `operator*`, `operator->`.
- `AllocatorRef()`.
- `Reset()`.
- move constructor / move assignment.

Public include:

```cpp
#include <zcore/owned.hpp>
```

## Usage Example

```cpp
auto created = zcore::Owned<MyType>::TryMake(allocator, 7);
if (created.HasValue()) {
  zcore::Owned<MyType> owned = std::move(created.Value());
  owned->DoWork();
}
```

## Warnings and Edge Cases

- `Owned<T>` currently does not provide raw release/transfer APIs; ownership remains RAII-bound.
- Deallocate failure on internally-owned allocation is treated as contract violation.

## Thread-Safety and Ownership Notes

- `Owned<T>` has no internal synchronization.
- Thread-safety follows `T`, allocator implementation, and external synchronization policy.

