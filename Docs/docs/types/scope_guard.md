# `ScopeGuard`

## Purpose

`ScopeGuard` runs a noexcept cleanup callable on scope exit unless explicitly dismissed.

## Invariants and Contracts

- Cleanup callable must be `noexcept` and nothrow move-constructible.
- Guard is non-copyable and move-constructible.
- Active guard runs cleanup exactly once (destructor or `ExecuteNow()`).
- `Dismiss()` disables cleanup; `Arm()` re-enables it.
- `ExecuteNow()` runs cleanup immediately and disables further execution.

## API Summary

- `template <typename CleanupFnT> class ScopeGuard`.
- `MakeScopeGuard(callable)`.
- State/mutation: `Active()`, `Dismiss()`, `Arm()`, `ExecuteNow()`.

Public include:

```cpp
#include <zcore/scope_guard.hpp>
```

## Usage Example

```cpp
auto guard = zcore::MakeScopeGuard([&]() noexcept { rollback(); });
// guard.Dismiss(); // keep commit path
```

## Warnings and Edge Cases

- Dismissing then arming changes final cleanup behavior; call sites should keep this explicit.

## Thread-Safety and Ownership Notes

- Guard owns its cleanup callable by value.
- No internal synchronization.
