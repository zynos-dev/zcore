# `Synchronized<T>`

## Purpose

`Synchronized<T>` owns a value and a mutex, exposing access only through lock-scoped guards.

## Invariants and Contracts

- `T` must be an object type (no references, arrays, functions, or `void`).
- Type is non-copyable and non-movable.
- `Lock()` acquires exclusive access and returns a guard.
- `TryLock()` returns `None` when lock acquisition fails.
- `const Lock()`/`const TryLock()` provide read-only guard access while still taking the same mutex.

## API Summary

- Construction: `Synchronized()`, `Synchronized(const T&)`, `Synchronized(T&&)`.
- Mutable access: `Lock()`, `TryLock()`.
- Const access: `Lock() const`, `TryLock() const`.
- Interop: `NativeMutex()`.

Public include:

```cpp
#include <zcore/synchronized.hpp>
```

## Usage Example

```cpp
zcore::Synchronized<int> value(1);
auto guard = value.Lock();
guard.Value() += 1;
```

## Warnings and Edge Cases

- Guards should be kept short-lived to avoid lock contention.
- `Synchronized<T>` does not provide lock-free semantics.

## Thread-Safety and Ownership Notes

- One internal `Mutex` serializes all access.
- Returned guards own lock state via RAII and release on destruction.

