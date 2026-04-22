# `ThreadLocal<T, TagT>`

## Purpose

`ThreadLocal<T, TagT>` provides per-thread typed storage keyed by value type and optional tag type.

## Invariants and Contracts

- `T` must be an object type (no references, arrays, functions, or `void`).
- All `ThreadLocal<T, TagT>` instances within one thread share one slot.
- Different threads have isolated slots.
- Different `TagT` values isolate slots even with the same `T`.
- `Value()` requires slot to contain a value (`HasValue() == true`).
- `Set(...)` replaces slot content via in-place construction; assignability is not required.

## API Summary

- State: `HasValue()`, `IsEmpty()`, `TryValue()`.
- Mutation: `Set(...)`, `Emplace(...)`, `Reset()`.
- Access: `Value()`, `ValueOrEmplace(...)`.

Public include:

```cpp
#include <zcore/thread_local.hpp>
```

## Usage Example

```cpp
zcore::ThreadLocal<int> local;
local.Set(42);
const int value = local.Value();
```

## Warnings and Edge Cases

- Same `<T, TagT>` means shared per-thread slot across all instances, not per-object storage.
- Use a distinct `TagT` when multiple independent slots of the same `T` are required.

## Thread-Safety and Ownership Notes

- Storage is thread-local; cross-thread synchronization is not required for direct slot access.
- Slot values are owned by the current thread-local storage lifetime.
