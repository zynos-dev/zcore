# `Mutex`

## Purpose

`Mutex` provides non-recursive exclusive mutual exclusion for shared state.

## Invariants and Contracts

- `Mutex` is non-copyable and non-movable.
- `Lock()` blocks until exclusive ownership is acquired.
- `TryLock()` is non-blocking and returns acquisition success.
- `Unlock()` releases exclusive ownership.
- Standard `BasicLockable` adapters (`lock`, `try_lock`, `unlock`) map directly to the explicit methods.

## API Summary

- `Lock()`, `TryLock()`, `Unlock()`.
- `lock()`, `try_lock()`, `unlock()`.
- `NativeHandle()` for native interop.

Public include:

```cpp
#include <zcore/mutex.hpp>
```

## Usage Example

```cpp
zcore::Mutex mutex;
std::lock_guard<zcore::Mutex> guard(mutex);
```

## Warnings and Edge Cases

- `Mutex` is non-recursive; locking the same mutex twice from one thread is invalid.
- Fairness and scheduling behavior follow the platform `std::mutex` implementation.

## Thread-Safety and Ownership Notes

- `Mutex` is a synchronization primitive and does not own protected data.
- Correctness requires all shared mutable state accesses to follow one consistent locking policy.
