# `ErrorHandler`

## Purpose

`ErrorHandler` defines explicit caller-owned reporting for recoverable `Error` values.

## Invariants and Contracts

- Allocation APIs still propagate failures through `Result`/`Status`.
- Reporting is opt-in and callback-based; no hidden global state.
- `ErrorHandlerRef` is non-owning (`handler`, `userData`).
- `HandleIfError(...)` reports only failure branches.
- `StderrErrorSink` is explicit opt-in and writes formatted diagnostics to `stderr`.

## API Summary

- `ErrorHandler` callback signature: `void(const Error&, void*) noexcept`.
- `ErrorHandlerRef`.
- `MakeErrorHandler(handler, userData)`.
- `ReportError(error, ErrorHandlerRef)` and callback overload.
- `HandleIfError(Result<T, Error>, ...)`.
- `HandleIfError(Status, ...)`.
- `StderrErrorSink`.

Public include:

```cpp
#include <zcore/error_handler.hpp>
```

## Usage Example

```cpp
struct SinkState {
  int count = 0;
};

void CountErrors(const zcore::Error&, void* userData) noexcept {
  auto* state = static_cast<SinkState*>(userData);
  if (state != nullptr) {
    state->count += 1;
  }
}

SinkState state{};
auto handler = zcore::MakeErrorHandler(&CountErrors, &state);
zcore::HandleIfError(status, handler);
```

## Warnings and Edge Cases

- Passing a null handler disables reporting.
- Handler/user-data lifetime is caller-managed.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread safety follows callback implementation and external synchronization.

