# `ThreadBound<T>`

## Purpose

`ThreadBound<T>` wraps one value and binds access to the constructing thread.

## Invariants and Contracts

- `T` must be an object type (no references, arrays, functions, or `void`).
- Construction captures owner thread identity and owner `ThreadId`.
- `Value()` requires current thread to equal owner thread.
- `TryValue()` returns `nullptr` on non-owner threads.
- Type is non-copyable and non-movable.

## API Summary

- Construction: `ThreadBound()`, `ThreadBound(const T&)`, `ThreadBound(T&&)`.
- Ownership checks: `OwnerThreadId()`, `IsOwnerThread()`.
- Access: `Value()`, `TryValue()`.

Public include:

```cpp
#include <zcore/thread_bound.hpp>
```

## Usage Example

```cpp
zcore::ThreadBound<int> value(42);
value.Value() = 99;
```

## Warnings and Edge Cases

- Access from non-owner thread is a contract violation for `Value()`.
- `ThreadBound<T>` does not provide synchronization; it enforces affinity only.

## Thread-Safety and Ownership Notes

- Owner-thread access is explicit and checked.
- Cross-thread handoff requires external synchronization and wrapper replacement.
