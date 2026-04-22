# `FixedString<N>`

## Purpose

`FixedString<N>` provides deterministic inline-owned UTF-8 text storage with fixed maximum byte length `N`.

## Invariants and Contracts

- Capacity is fixed at `N` bytes (plus internal null terminator slot).
- Storage is always null-terminated (`c_str()` valid for current contents).
- Heap allocation never occurs.
- Stored bytes are validated UTF-8 for checked constructors and mutating APIs.
- `Assign`/`Append`/`PushBack` are contract-enforced and terminate on overflow.
- `Try*` APIs return failure without mutating state on overflow.
- `RemovePrefix`, `RemoveSuffix`, and `TryPopBack` operate on UTF-8 code points.

## API Summary

- Construction/factories: default, `FromStringView`, `FromCString`, `TryFrom*`.
- Capacity/state: `Capacity`, `Size`, `RemainingCapacity`, `Empty`, `Full`.
- Access: `Data/data`, `CStr/c_str`, `begin/end`, `TryAt`, `operator[]`.
- Mutation: `TryAssign`/`Assign`, `TryAppend`/`Append`, `TryPushBack`/`PushBack`, `TryAppendCodePoint`/`AppendCodePoint`.
- Removal (code point): `TryPopBack`/`PopBack`, `RemovePrefix`, `RemoveSuffix`.
- Removal (byte compatibility): `RemovePrefixBytes`, `RemoveSuffixBytes`.
- Reset: `Clear`.
- View adapters: `AsStringView`, `AsStdStringView`.
- UTF-8 helpers: `IsValidUtf8`, `TryCodePointCount`, `CodePointCount`,
  `TryAssignUtf8`/`AssignUtf8`, `TryAppendUtf8`/`AppendUtf8`,
  `TryFromUtf8CString`/`FromUtf8CString`.

Public include:

```cpp
#include <zcore/fixed_string.hpp>
```

## Usage Examples

```cpp
zcore::FixedString<16> name = zcore::FixedString<16>::FromCString("zcore");
name.Append(zcore::StringView("-core"));
```

```cpp
zcore::FixedString<12> code = zcore::FixedString<12>::FromCString("A\xC2\xA2\xE2\x82\xAC");
if (!code.TryAppend(zcore::StringView("\xF0\x9F\x98\x80"))) {
  // overflow rejected, previous content preserved
}
```

```cpp
zcore::FixedString<8> value;
value.AppendCodePoint(0x00A2U);  // cent sign
value.AppendCodePoint(0x20ACU);  // euro sign
```

## Warnings and Edge Cases

- `FromCString(nullptr)` is invalid; use `TryFromCString` for fallible behavior.
- `FromCString` and `TryFromCString` reject invalid UTF-8.
- `PushBack`/`TryPushBack` accept ASCII single-byte input only.
- `RemovePrefixBytes`/`RemoveSuffixBytes` may split code points.
- `operator[]` is bounds-preconditioned.
- `N == 0` is valid and only represents empty string.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Owns only inline character storage.
- Thread safety follows normal value-type + external synchronization policy.
