# `Thread`

## Purpose

`Thread` is a move-only owning handle for runtime threads with explicit `Join()`/`Detach()` ownership completion.

## Invariants and Contracts

- `Thread` is non-copyable and move-only.
- Default-constructed `Thread` is non-joinable with invalid id.
- Destruction requires non-joinable state; callers must explicitly `Join()` or `Detach()`.
- `Join()` requires `Joinable()` and must not target the current thread.
- `Detach()` requires `Joinable()`.
- `GetId()` returns invalid id when non-joinable.

## API Summary

- Construction: `Thread()`, `Thread(std::thread&&)`.
- Ownership/state: `Joinable()`, `GetId()`, `CurrentId()`.
- Lifecycle: `Join()`, `Detach()`, `Swap(Thread&)`.
- Interop: `NativeHandle()`.

Public include:

```cpp
#include <zcore/thread.hpp>
```

## Usage Example

```cpp
zcore::Thread worker(std::thread([]() {
  // work
}));
worker.Join();
```

## Warnings and Edge Cases

- Destroying a joinable `Thread` is a contract violation.
- Move-assigning into a joinable target is a contract violation.

## Thread-Safety and Ownership Notes

- `Thread` owns one native thread handle when joinable.
- Synchronization of shared data with worker threads remains caller responsibility.
