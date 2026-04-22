# `Any`

## Purpose

`Any` is a move-only allocator-aware type-erased value container with inline storage + explicit fallible heap spill.

## Invariants and Contracts

- Stored value type must be object, non-`void`, non-reference, and nothrow move-constructible.
- `TryMake`/`TryEmplace` require nothrow construction and use allocator only when value does not fit inline storage.
- `Get<T>()` requires exact stored type match; mismatches are contract violations.
- `Reset()` destroys stored value and deallocates heap storage through the original allocator.

## API Summary

- `class Any`
- `TryMake<T>(Allocator&, args...) -> Result<Any, Error>`
- `TryEmplace<T>(Allocator&, args...) -> Status`
- `HasValue()`, `IsEmpty()`, `Type()`
- `Contains<T>()`, `TryGet<T>()`, `Get<T>()`, `Reset()`

Public include:

```cpp
#include <zcore/any.hpp>
```

## Thread-Safety and Ownership Notes

- `Any` owns stored value lifetime.
- No internal synchronization.
