# `String`

## Purpose

`String` is an allocator-backed owning UTF-8 string built on dynamic contiguous storage.

## Invariants and Contracts

- Dynamic memory uses the bound `Allocator` only.
- Recoverable failures return `Status`/`Result` (allocator + string-domain errors).
- Non-empty storage keeps a trailing null terminator for `c_str()` interoperability.
- UTF-8-aware APIs (`TryAssign`, `TryAppend`, `TryAppendCodePoint`, code-point windowing) require valid UTF-8 inputs.
- Byte compatibility APIs (`RemovePrefixBytes`, `RemoveSuffixBytes`) can split code points and invalidate UTF-8.
- Type is move-only.

## API Summary

- Construction: `String()`, `String(Allocator&)`, `TryWithCapacity`, `TryFromStringView`, `TryFromCString`.
- Shape/state: `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`, `HasAllocator`, `AllocatorRef`.
- Access: `Data/data`, `CStr/c_str`, `begin/end`, `TryAt`, `Front`, `Back`, `operator[]`.
- Mutation: `TryReserve`, `TryAssign*`, `TryAppend*`, `TryPushBack` (ASCII), `TryPopBack`.
- Window/removal: `RemovePrefix`, `RemoveSuffix`, `RemovePrefixBytes`, `RemoveSuffixBytes`.
- Validation/helpers: `IsValidUtf8`, `TryCodePointCount`, `CodePointCount`, `AsStringView`, `AsStdStringView`.
- Lifecycle: `Clear`, `Reset`.

Public include:

```cpp
#include <zcore/string.hpp>
```

## Usage Example

```cpp
zcore::String value(allocator);
if (value.TryAssignCString("zcore").HasValue()) {
  value.TryAppendCodePoint(0x1F642U);
}
```

## Warnings and Edge Cases

- Unbound strings (`String()`) cannot allocate; non-empty assign/append/reserve returns error.
- `TryPushBack` accepts ASCII only; use `TryAppendCodePoint` for non-ASCII scalar values.
- `CodePointCount` is contract-checked; use `TryCodePointCount` after byte-level mutations.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows allocator implementation, string instance ownership, and external synchronization policy.
- String owns dynamic byte storage only.
