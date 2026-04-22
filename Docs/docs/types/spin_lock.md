# `SpinLock`

## Purpose

`SpinLock` provides a lightweight busy-wait exclusive lock for very short critical sections.

## Invariants and Contracts

- `SpinLock` is non-copyable and non-movable.
- `Lock()` busy-waits until ownership is acquired.
- `TryLock()` is non-blocking and returns acquisition success.
- `Unlock()` releases ownership with release semantics.
- `IsLocked()` reports current flag state.
- Standard `BasicLockable` adapters (`lock`, `try_lock`, `unlock`) map directly to explicit methods.

## API Summary

- `Lock()`, `TryLock()`, `Unlock()`, `IsLocked()`.
- `lock()`, `try_lock()`, `unlock()`.

Public include:

```cpp
#include <zcore/spin_lock.hpp>
```

## Usage Example

```cpp
zcore::SpinLock lock;
std::lock_guard<zcore::SpinLock> guard(lock);
```

## Warnings and Edge Cases

- Avoid long critical sections; busy-waiting can waste CPU and increase contention.
- Not recursive; relocking by the owning thread is invalid.

## Thread-Safety and Ownership Notes

- `SpinLock` is a synchronization primitive and does not own protected data.
- Best used for short, low-contention paths where mutex sleep/wakeup overhead dominates.
