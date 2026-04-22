# `ThreadId`

## Purpose

`ThreadId` is a lightweight runtime thread identity token for thread-affinity and ownership checks.

## Invariants and Contracts

- Value representation: 64-bit unsigned token (`u64`).
- Default state is invalid (`kInvalidValue == 0`).
- `Current()` returns a valid non-zero token for the calling thread.
- `Current()` token derives from `std::thread::id` hash and is stable per thread during process lifetime.
- Type is trivially copyable and allocation-free.

## API Summary

- Construction: `ThreadId()`, `ThreadId(raw)`, `FromRawUnchecked(raw)`.
- Factories: `Invalid()`, `Current()`.
- Access/state: `Raw()`, `IsValid()`, `IsInvalid()`, `Reset()`.
- Operators: `==`, `<=>`, explicit conversion to `u64`.
- Hash adapters: `zcore::hash::Hash<ThreadId>` and `std::hash<ThreadId>`.

Public include:

```cpp
#include <zcore/thread_id.hpp>
```

## Usage Example

```cpp
const zcore::ThreadId owner = zcore::ThreadId::Current();
if (owner == zcore::ThreadId::Current()) {
  // thread-affine path
}
```

## Warnings and Edge Cases

- Raw token values are process/runtime artifacts; do not persist them as stable cross-run identifiers.
- `Current()` derives from hashed native thread identity; collisions are possible across distinct threads.
- `FromRawUnchecked` does not validate semantic origin of the token.

## Thread-Safety and Ownership Notes

- Value type only; no implicit synchronization.
- `Current()` is thread-safe as it queries calling-thread identity only.
