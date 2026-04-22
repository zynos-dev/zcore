# `TypeInfo`

## Purpose

`TypeInfo` provides deterministic metadata for an exact C++ type (`TypeId`, name token, layout, triviality traits).

## Invariants and Contracts

- Default state is invalid.
- Valid metadata requires valid `TypeId`, non-empty name token, non-zero size, non-zero alignment.
- `TypeInfo::Of<T>()` is deterministic for a given compiler/toolchain and exact type.
- `TypeInfo` is a value type with no allocation.

## API Summary

- `TypeInfo()` creates invalid metadata.
- `Invalid()` returns canonical invalid metadata.
- `FromRawUnchecked(...)` builds metadata from raw fields without validation.
- `Of<T>()` returns deterministic metadata for exact type `T`.
- `Id()` returns `TypeId`.
- `Name()` returns deterministic type-name/view token.
- `Size()` / `Alignment()` return object layout metadata.
- `IsTriviallyCopyable()` / `IsTriviallyDestructible()` / `IsTriviallyDefaultConstructible()` expose trait flags.
- `TraitBits()` returns stable compact trait mask.
- `IsValid()` / `IsInvalid()` checks validity.
- `Reset()` restores invalid state.
- Full equality and ordering operators (`==`, `<=>`).
- `zcore::hash::Hash<TypeInfo>` and `std::hash<TypeInfo>` are provided.

Public include:

```cpp
#include <zcore/type_info.hpp>
```

## Usage Examples

```cpp
constexpr zcore::TypeInfo info = zcore::TypeInfo::Of<int>();
if (info.IsValid()) {
  auto size = info.Size();
}
```

```cpp
std::unordered_set<zcore::TypeInfo> types;
types.insert(zcore::TypeInfo::Of<int>());
```

## Warnings and Edge Cases

- `FromRawUnchecked` bypasses validity checks.
- `Name()` token is compiler-signature-derived and not guaranteed stable across different compilers/signature formats.

## Thread-Safety and Ownership Notes

- `TypeInfo` is trivially copyable and has no ownership behavior.
- Thread-safety follows normal value-type semantics.
