# `Error`

## Purpose

`Error` is the structured recoverable failure payload used by `Result<T, E>` and `Status`.

## Invariants and Contracts

- `ErrorCode::value == 0` means success.
- `ErrorDomain` identifies subsystem-specific error spaces.
- `ErrorContext` carries lightweight diagnostic metadata (subsystem/operation/message/source location).
- `Error` is a value type and does not allocate memory by itself.

## Usage Example

```cpp
#include <zcore/error.hpp>

zcore::Error err = zcore::MakeError(
    zcore::kZcoreErrorDomain,
    1,
    zcore::MakeErrorContext("io", "read", "short read", __FILE__, __LINE__));
```

## Warnings and Edge Cases

- Keep error code values stable once published.
- Do not rely on message pointers owning storage; use static or externally-managed strings.

## Thread-Safety and Ownership Notes

- `Error` is trivially shareable as a value object when underlying pointer targets remain valid.
