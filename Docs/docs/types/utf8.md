# `utf8` Utilities

## Purpose

`zcore::utf8` provides validation, decoding, and code-point counting for UTF-8 byte sequences.

## Invariants and Contracts

- Inputs are byte-oriented (`const char*` + size).
- No allocation.
- Invalid UTF-8 returns `None`/`false` from fallible APIs.
- Utilities validate scalar-value constraints (no surrogates, no out-of-range values).

## API Summary

- `IsValid(data, size) -> bool`
- `CountCodePoints(data, size) -> Option<usize>`
- `DecodeAt(data, size, offset) -> Option<DecodedCodePoint>`
- `IsCodePointBoundary(data, size, offset) -> bool`
- `AdvanceCodePoints(data, size, offset, count) -> Option<usize>`
- `EncodeCodePoint(value, out, capacity) -> Option<usize>`
- `DecodedCodePoint { value, width }`

Public include:

```cpp
#include <zcore/utf8.hpp>
```

## Usage Examples

```cpp
bool ok = zcore::utf8::IsValid(bytes, byteCount);
auto count = zcore::utf8::CountCodePoints(bytes, byteCount);
```

```cpp
auto decoded = zcore::utf8::DecodeAt(bytes, byteCount, 0U);
if (decoded.HasValue()) {
  // decoded.Value().value, decoded.Value().width
}
```

```cpp
char out[4] = {};
auto width = zcore::utf8::EncodeCodePoint(0x1F600U, out, sizeof(out));
```

## Warnings and Edge Cases

- `offset >= size` in `DecodeAt` returns `None`.
- Null pointer is valid only for the empty range (`size == 0` and `offset == 0`).
- Counting/decoding are byte-sequence operations, not grapheme clustering.

## Thread-Safety and Ownership Notes

- Stateless functions.
- No ownership changes.
