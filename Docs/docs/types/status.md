# `Status`

## Purpose

`Status` is an alias for `Result<void, Error>` and represents recoverable success/failure without a value payload.

## Invariants and Contracts

- Success has no payload.
- Failure contains `Error`.
- Use `OkStatus()` for success and `ErrorStatus(...)` for failures.
- `Status` is `[[nodiscard]]` through `Result<void, Error>`.
- `ErrorStatus(Error&&)` preserves move semantics for low-overhead failure construction.

## Usage Example

```cpp
#include <zcore/status.hpp>

zcore::Status Validate(bool ok) {
  if (ok) {
    return zcore::OkStatus();
  }
  return zcore::ErrorStatus(
      zcore::MakeError(
          zcore::kZcoreErrorDomain,
          3,
          zcore::MakeErrorContext("validate", "check", "failed", __FILE__, __LINE__)));
}
```

## Warnings and Edge Cases

- Keep code paths explicit: use status for recoverable flow, panic only for invariants/unrecoverable states.

## Thread-Safety and Ownership Notes

- Same semantics as `Result<void, Error>`.
