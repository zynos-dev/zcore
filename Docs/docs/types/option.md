# `Option<T>`

## Purpose

`Option<T>` models normal value absence without using null or exceptions.

Use `Option<T>` when:
- a value may or may not be present
- absence is expected and not an error condition

Use `Result<T, E>` when failure information is required.

## Invariants and Contracts

- State is either:
  - empty (`None`)
  - engaged (contains exactly one `T`)
- No heap allocation is performed by `Option<T>` itself.
- `Value()` requires `HasValue() == true`.
- `Option<T>` does not support reference, `void`, array, function, or `NoneType` specializations.
- `Option<T>` is `[[nodiscard]]`; dropping it without handling is a contract smell.

## API Summary

- `Option<T>()` / `Option<T>(None)` creates empty state.
- `Option<T>(value)` creates engaged state.
- `Some(value)` creates engaged state with deduced type.
- `HasValue()` / `IsSome()` / `IsNone()` check engagement state.
- `TryValue()` returns pointer access (`nullptr` when empty).
- `Value()` returns contained value (contract-fails and terminates if empty).
- `ValueOr(default)` returns contained value or fallback.
- `Map`, `AndThen`, `OrElse` provide composable transformations/recovery.
- `MapOr`, `MapOrElse` provide mapped fallback values.
- `Inspect` runs side effects while preserving the current option.
- `Emplace(args...)` replaces current state with in-place value construction.
- `Take()` moves value out and clears the option.
- `Replace(value)` swaps value and returns previous contents.
- `Reset()` clears value and returns to empty state.

## Usage Examples

```cpp
#include <zcore/option.hpp>

zcore::Option<int> ParsePort(bool hasPort, int port) {
  if (!hasPort) {
    return zcore::None;
  }
  return port;
}
```

```cpp
zcore::Option<int> port = ParsePort(false, 0);
int effective = port.ValueOr(8080);  // 8080
```

```cpp
zcore::Option<std::string> name;
name.Emplace("zcore");
name.Reset();
```

## Warnings and Edge Cases

- Calling `Value()` on empty `Option<T>` violates the contract and terminates.
- `Map` callback must return a value type (not `void`); use `Inspect` for side effects.
- Prefer `HasValue()` checks in recovery-oriented paths before calling `Value()`.
- `ValueOr` returns by value; for heavy `T`, account for copy/move cost.
- For move-only `T`, access patterns should prefer move-aware usage.

## Thread-Safety and Ownership Notes

- `Option<T>` is not internally synchronized.
- Thread safety matches `T` and external synchronization policy.
- Ownership semantics of stored value are explicit in `T` (for example `Option<Owned<Foo>>`).
