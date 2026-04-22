# `StringView`

## Purpose

`StringView` is a non-owning bounds-aware view over validated UTF-8 bytes.

## Invariants and Contracts

- Never allocates and never owns backing memory.
- Range contract: `size > 0` requires non-null pointer.
- Checked construction requires valid UTF-8 (`StringView(...)`, `FromCString`, `TryFromCString`, `TryFromRaw`).
- `operator[]` is preconditioned (`index < Size()`).
- `TryFromRaw` and `TryFromCString` provide fallible construction.
- Window/mutation APIs (`First`, `Last`, `Substr`, `RemovePrefix`, `RemoveSuffix`) operate in code points.

## API Summary

- Construction: default, pointer+size, `std::string_view`, char array, C-string helpers.
- Fallible factories: `TryFromRaw`, `TryFromCString`.
- Unchecked factory: `FromRawUnchecked`.
- Access: `Data/data`, `Size/size`, `EmptyView/empty`, `begin/end`, `TryAt`, `operator[]`.
- Code-point subviews/mutation: `First`, `Last`, `Substr`, `RemovePrefix`, `RemoveSuffix`.
- Byte compatibility: `SubstrBytes`, `RemovePrefixBytes`, `RemoveSuffixBytes`.
- Reset: `Clear`.
- Adapters: `AsSlice`, `AsStdStringView`, implicit conversions.
- UTF-8 helpers: `IsValidUtf8`, `TryCodePointCount`, `CodePointCount`,
  `TryFromUtf8CString`, `FromUtf8CString`.

Public include:

```cpp
#include <zcore/string_view.hpp>
```

## Usage Examples

```cpp
const zcore::StringView value = zcore::StringView::FromCString("A\xC2\xA2\xE2\x82\xAC\xF0\x9F\x98\x80B");
auto middle = value.Substr(1U, 3U);  // code-point window: cent + euro + grinning-face
```

```cpp
zcore::StringView mutableWindow = zcore::StringView::FromCString("A\xC2\xA2\xE2\x82\xAC\xF0\x9F\x98\x80B");
mutableWindow.RemovePrefix(2U);  // euro + grinning-face + B (code points)
```

```cpp
zcore::StringView bytes = zcore::StringView::FromCString("A\xC2\xA2");
auto split = bytes.SubstrBytes(2U, 1U);  // byte-level compatibility API
```

## Warnings and Edge Cases

- Char-array constructor excludes trailing null terminator.
- `FromCString(nullptr)` is a contract violation; use `TryFromCString` for fallible behavior.
- `FromCString` also requires UTF-8 validity.
- `SubstrBytes`/`RemovePrefixBytes`/`RemoveSuffixBytes` may split code points.
- Lifetime and validity follow backing storage.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread safety follows backing storage and external synchronization policy.
- Ownership remains with source buffer/string.
