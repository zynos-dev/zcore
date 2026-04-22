# `Id<TagT>`

## Purpose

`Id<TagT>` provides strongly typed, lightweight identifiers to prevent accidental cross-domain ID mixing.

## Invariants and Contracts

- IDs are tag-scoped: `Id<UserTag>` is a distinct type from `Id<AssetTag>`.
- Default state is invalid (sentinel `kInvalidValue`).
- `Id` is a value type with deterministic behavior and no allocation.
- Underlying value type is unsigned integral.

## API Summary

- `Id<TagT>()` creates invalid ID.
- `Id<TagT>(raw)` explicit raw construction.
- `Invalid()` returns sentinel invalid value.
- `FromRawUnchecked(raw)` explicit unchecked raw factory.
- `Raw()` returns underlying value.
- `IsValid()` / `IsInvalid()` checks validity.
- `Reset()` restores invalid state.
- Full equality and ordering operators (`==`, `<=>`).
- `zcore::hash::Hash<Id<...>>` specialization is provided for zcore hashing.
- `std::hash` specialization is provided as an adapter for unordered containers.

Public include:

```cpp
#include <zcore/id.hpp>
```

## Usage Examples

```cpp
struct UserTag final {};
using UserId = zcore::Id<UserTag>;

UserId id(42);
if (id.IsValid()) {
  auto raw = id.Raw();
}
```

```cpp
std::unordered_set<UserId> users;
users.insert(UserId(10));
```

## Warnings and Edge Cases

- `FromRawUnchecked` does not enforce validity checks.
- Validity is sentinel-based; choose custom `InvalidV` carefully if customizing `ValueT`.

## Thread-Safety and Ownership Notes

- `Id` is trivially copyable and has no ownership behavior.
- Thread-safety follows normal value-type semantics.
