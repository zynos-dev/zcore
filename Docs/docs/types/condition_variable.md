# `ConditionVariable`

## Purpose

`ConditionVariable` provides blocking wait/notify coordination for state guarded by `Mutex`.

## Invariants and Contracts

- `ConditionVariable` is non-copyable and non-movable.
- Works with `std::unique_lock<zcore::Mutex>`.
- `Wait(lock)` can wake spuriously; use predicate overload for state-based waiting.
- `WaitFor(lock, timeout, predicate)` returns `true` when predicate becomes true before timeout.
- Non-positive timeout in `WaitFor` is treated as immediate timeout check (`0ns`).

## API Summary

- `NotifyOne()`, `NotifyAll()`.
- `Wait(std::unique_lock<Mutex>&)`.
- `Wait(std::unique_lock<Mutex>&, PredicateT)`.
- `WaitFor(std::unique_lock<Mutex>&, Duration, PredicateT) -> bool`.
- `NativeHandle()` for native interop.

Public include:

```cpp
#include <zcore/condition_variable.hpp>
```

## Usage Example

```cpp
zcore::ConditionVariable cv;
zcore::Mutex mutex;
bool ready = false;

std::unique_lock<zcore::Mutex> lock(mutex);
cv.Wait(lock, [&ready]() { return ready; });
```

## Warnings and Edge Cases

- Always guard shared predicate state with the same mutex used for waiting.
- Notification does not transfer ownership of shared data; lock discipline remains required.

## Thread-Safety and Ownership Notes

- `ConditionVariable` is a synchronization primitive and owns no protected state.
- Correctness depends on mutex + predicate protocol, not notification alone.
