# `TypeId`

## Purpose

`TypeId` provides a compact, deterministic identity token for exact C++ types.

## Invariants and Contracts

- Default state is invalid (`kInvalidValue`).
- `TypeId::Of<T>()` is deterministic for a given compiler/toolchain and type.
- `TypeId` is a value type with no allocation.
- Validity is sentinel-based (`0` is invalid).

## API Summary

- `TypeId()` creates invalid value.
- `TypeId(raw)` explicit raw construction.
- `Invalid()` returns canonical invalid value.
- `FromRawUnchecked(raw)` explicit unchecked raw factory.
- `Of<T>()` returns deterministic type identity.
- `Value()` returns raw token.
- `IsValid()` / `IsInvalid()` checks validity.
- `Reset()` restores invalid state.
- Full equality and ordering operators (`==`, `<=>`).
- `zcore::hash::Hash<TypeId>` specialization for zcore hashing.
- `std::hash<TypeId>` adapter for unordered containers.

Public include:

```cpp
#include <zcore/type_id.hpp>
```

## Usage Examples

```cpp
constexpr zcore::TypeId intType = zcore::TypeId::Of<int>();
constexpr zcore::TypeId floatType = zcore::TypeId::Of<float>();
```

```cpp
std::unordered_set<zcore::TypeId> registered;
registered.insert(zcore::TypeId::Of<int>());
```

## Warnings and Edge Cases

- `FromRawUnchecked` bypasses validity checks.
- `Of<T>()` identity is deterministic but not guaranteed stable across different compilers or signature formatting models.

## Thread-Safety and Ownership Notes

- `TypeId` is trivially copyable and has no ownership behavior.
- Thread-safety follows normal value-type semantics.
