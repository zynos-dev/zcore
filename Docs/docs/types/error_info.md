# `ErrorInfo`

## Purpose

`ErrorInfo` is the structured recoverable failure payload consumed by `Result<T, E>` and `Status`.

## Invariants and Contracts

- `code` is domain-qualified (`ErrorDomain` + numeric value).
- Success requires canonical code shape (`ErrorDomain::Success()` + `value == 0`).
- `context` stores lightweight diagnostics (`subsystem`, `operation`, `message`, source location).
- Value type only; no owning allocations.
- `Error` remains a compatibility alias of `ErrorInfo`.

## API Summary

- Construction: aggregate initialization.
- Factory: `ErrorInfo::Ok()`.
- Helpers: `MakeErrorInfo(...)`, `MakeErrorContext(...)`.
- Compatibility: `using Error = ErrorInfo`, `MakeError(...)`.
- Query: `IsOk()`.

Public include:

```cpp
#include <zcore/error_info.hpp>
```

## Usage Example

```cpp
const zcore::ErrorInfo info = zcore::MakeErrorInfo(
    zcore::kZcoreErrorDomain,
    11,
    zcore::MakeErrorContext("io", "read", "short read", __FILE__, __LINE__));
```

## Warnings and Edge Cases

- Context string pointers are non-owning, keep backing storage alive.
- Error code values should remain stable once published.

## Thread-Safety and Ownership Notes

- Thread-safe by value-copy semantics when referenced string storage lifetime is valid.

