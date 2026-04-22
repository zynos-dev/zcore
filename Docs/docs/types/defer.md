# `Defer`

## Purpose

`Defer` provides defer-style scope-exit cleanup and is a naming alias over `ScopeGuard`.

## Invariants and Contracts

- Cleanup callable must be `noexcept` and nothrow move-constructible.
- `Defer<CleanupFnT>` is non-copyable and move-constructible.
- Active defer runs cleanup exactly once (destructor or `ExecuteNow()`).
- `Dismiss()` disables cleanup; `Arm()` re-enables it.

## API Summary

- Alias: `template <typename CleanupFnT> using Defer = ScopeGuard<CleanupFnT>`.
- Factory: `MakeDefer(callable)`.
- State/mutation: `Active()`, `Dismiss()`, `Arm()`, `ExecuteNow()`.

Public include:

```cpp
#include <zcore/defer.hpp>
```

## Thread-Safety and Ownership Notes

- `Defer` owns its cleanup callable by value.
- No internal synchronization.
