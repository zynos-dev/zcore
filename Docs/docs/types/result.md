# `Result<T, E>`

## Purpose

`Result<T, E>` models recoverable operations that either succeed with `T` or fail with `E`.

`Result` is generic over error payloads; it does not require `zcore::Error`.
Use `Status` (alias of `Result<void, Error>`) when you want the default project error payload.

## Invariants and Contracts

- Exactly one state is active:
  - success value `T`
  - failure value `E`
- No exceptions are required for recoverable control flow.
- `Result<T, E>` is `[[nodiscard]]`; callers are expected to handle it explicitly.
- `Value()` requires success state.
- `Error()` requires failure state.
- `TryValue()` and `TryError()` provide non-fatal branch-based access.
- `Ok()` and `Err()` project branches into `Option`.
- Combinators provide explicit transformation/propagation:
  - `Map`
  - `MapOr`
  - `MapOrElse`
  - `MapError`
  - `AndThen`
  - `OrElse`
  - `UnwrapOr`
  - `UnwrapOrElse`
  - `Inspect`
  - `InspectError`
- `Transpose()` supports `Result<Option<U>, E> -> Option<Result<U, E>>`.

## Usage Example

```cpp
#include <zcore/result.hpp>
#include <zcore/error.hpp>

zcore::Result<int, zcore::Error> ParsePort(bool ok) {
  if (!ok) {
    return zcore::Result<int, zcore::Error>::Failure(
        zcore::MakeError(
            zcore::kZcoreErrorDomain,
            12,
            zcore::MakeErrorContext("cfg", "parse_port", "invalid", __FILE__, __LINE__)));
  }
  return zcore::Result<int, zcore::Error>::Success(8080);
}
```

```cpp
auto out = ParsePort(true)
    .Map([](int port) { return port + 1; })
    .AndThen([](int port) { return zcore::Result<int, zcore::Error>::Success(port * 2); });
```

## Warnings and Edge Cases

- `Value()`/`Error()` are preconditioned accessors.
- Use `TryValue()`/`TryError()` when branch-based handling is preferred.
- `Map` callback may return `void` and produces `Result<void, E>`.
- Keep `E` lightweight for hot paths.

## Thread-Safety and Ownership Notes

- `Result<T, E>` has no internal synchronization.
- Thread safety follows `T`/`E` and external synchronization policy.
