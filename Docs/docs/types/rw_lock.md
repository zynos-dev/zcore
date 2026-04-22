# `RwLock`

## Purpose

`RwLock` provides shared-reader and exclusive-writer synchronization for read-heavy shared state.

## Invariants and Contracts

- `RwLock` is non-copyable and non-movable.
- Exclusive API: `Lock()`, `TryLock()`, `Unlock()`.
- Shared API: `LockShared()`, `TryLockShared()`, `UnlockShared()`.
- Exclusive and shared adapters (`lock`, `try_lock`, `unlock`, `lock_shared`, `try_lock_shared`, `unlock_shared`) map directly to explicit methods.
- Reader/writer fairness is platform-implementation-defined.

## API Summary

- Exclusive operations: `Lock`, `TryLock`, `Unlock`.
- Shared operations: `LockShared`, `TryLockShared`, `UnlockShared`.
- Standard lock adapters for `std::unique_lock` and `std::shared_lock`.
- `NativeHandle()` for native interop.

Public include:

```cpp
#include <zcore/rw_lock.hpp>
```

## Usage Example

```cpp
zcore::RwLock lock;
std::shared_lock<zcore::RwLock> reader(lock);
```

## Warnings and Edge Cases

- Lock upgrade/downgrade operations are not provided.
- Writer starvation behavior is platform dependent.

## Thread-Safety and Ownership Notes

- `RwLock` synchronizes access and does not own protected data.
- Shared-mode reads are safe only when no writer mutates the same state concurrently.
