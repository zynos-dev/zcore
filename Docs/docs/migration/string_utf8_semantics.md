# String UTF-8 Migration

## Scope

`StringView` and `FixedString<N>` now default to UTF-8 validated construction and code-point window/mutation semantics.

## Behavior Changes

- Construction:
  - `StringView(pointer, size)`, `StringView(std::string_view)`, `StringView(char[N])`, `FromCString`, `TryFromCString`, `TryFromRaw` now validate UTF-8.
  - `FixedString<N>::FromCString` / `TryFromCString` now reject invalid UTF-8.
- Window/mutation:
  - `StringView::First`, `Last`, `Substr`, `RemovePrefix`, `RemoveSuffix` now use code-point counts.
  - `FixedString<N>::RemovePrefix`, `RemoveSuffix`, `TryPopBack` now use code-point counts.
- Single-byte append:
  - `FixedString<N>::PushBack` / `TryPushBack` are ASCII-only.
  - `FixedString<N>::AppendCodePoint` / `TryAppendCodePoint` added for non-ASCII scalar appends.

## Compatibility APIs

- `StringView`: `SubstrBytes`, `RemovePrefixBytes`, `RemoveSuffixBytes`.
- `FixedString<N>`: `RemovePrefixBytes`, `RemoveSuffixBytes`.

Compatibility byte APIs keep legacy byte behavior and can split code points (`IsValidUtf8()` may become `false` after use).

## Migration Mapping

- Byte slicing (`Substr(offsetBytes, countBytes)`) -> `SubstrBytes(offsetBytes, countBytes)`.
- Byte trimming (`RemovePrefix(bytes)`, `RemoveSuffix(bytes)`) -> `RemovePrefixBytes(bytes)`, `RemoveSuffixBytes(bytes)`.
- Character append (`PushBack(nonAscii)`) -> `AppendCodePoint(scalar)`.

## Invariant Reminder

- `Size()` = bytes.
- `CodePointCount()` = Unicode scalar count.
- Use code-point APIs for UTF-8-safe text operations.

## Performance Check

Build and run:

```sh
cmake -S . -B cmake-build-debug -DZCORE_BUILD_BENCHMARKS=ON
cmake --build cmake-build-debug --target zcore_string_utf8_bench
./cmake-build-debug/benchmarks/zcore_string_utf8_bench
```

Current debug baseline (April 11, 2026, Windows/Clang):
- `StringView::Substr (code point)`: ~317 ns/op
- `StringView::SubstrBytes`: ~31 ns/op
- `StringView remove (code point)`: ~1315 ns/op
- `StringView remove bytes`: ~8 ns/op
- `FixedString::TryAppendCodePoint`: ~88 ns/op
