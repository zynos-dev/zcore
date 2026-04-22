# `Shared<T>`

## Purpose

`Shared<T>` provides allocator-backed shared ownership for a single object with deterministic last-owner destroy+deallocate behavior.

## Invariants and Contracts

- `Shared<T>` is copyable and moveable; copies share one control block.
- `TryMake(...)` allocates one control block via `Allocator`, constructs `T` in-place, returns `Result<Shared<T>, Error>`.
- Empty state has no control block/allocation.
- `Reset()` releases one reference; final release destroys `T` and deallocates control block through the originating allocator.
- `UseCount()` reports live owner count, `IsUnique()` is `UseCount() == 1`.
- `Value()`/`operator*`/`operator->` require non-empty state and enforce contract checks.
- `TryMake(...)` requires `T` to be nothrow-constructible for the provided arguments.
- Reference counting is non-atomic.

## API Summary

- `Shared()`.
- `TryMake(Allocator&, Args...)`.
- `HasValue()`, `IsEmpty()`, `Get()`.
- `Value()`, `operator*`, `operator->`.
- `UseCount()`, `IsUnique()`.
- `AllocatorRef()`.
- `Reset()`.
- copy/move constructors and assignments.

Public include:

```cpp
#include <zcore/shared.hpp>
```

## Usage Example

```cpp
auto created = zcore::Shared<MyType>::TryMake(allocator, 7);
if (created.HasValue()) {
  zcore::Shared<MyType> a = std::move(created.Value());
  zcore::Shared<MyType> b = a;
  b->DoWork();
}
```

## Warnings and Edge Cases

- `Shared<T>` currently does not provide weak references.
- Reference counting is not thread-safe; synchronize externally when sharing across threads.
- Deallocate failure on internally-owned allocation is treated as contract violation.

## Thread-Safety and Ownership Notes

- `Shared<T>` has no internal synchronization.
- Thread-safety follows `T`, allocator implementation, and external synchronization policy.

