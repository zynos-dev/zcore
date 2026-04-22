# `StrongTypedef<TagT, ValueT, InvalidV>`

## Purpose

`StrongTypedef` provides a strongly typed value wrapper with an explicit invalid sentinel, preventing accidental cross-domain value mixing.

## Invariants and Contracts

- Tag-scoped type identity: `StrongTypedef<UserTag>` is distinct from `StrongTypedef<AssetTag>`.
- Default state is invalid (`kInvalidValue == InvalidV`).
- `IsValid()` is sentinel-based (`Raw() != InvalidV`).
- Value semantics only; no allocation, no hidden synchronization.

## API Summary

- Construction: `StrongTypedef()`, `StrongTypedef(raw)`.
- Factories: `Invalid()`, `FromRawUnchecked(raw)`.
- Access/state: `Raw()`, `IsValid()`, `IsInvalid()`, `Reset()`.
- Operators: `==`, `<=>`, explicit `bool`, explicit conversion to `ValueT`.
- Hash adapters: `zcore::hash::Hash<StrongTypedef<...>>` and `std::hash<StrongTypedef<...>>`.

Public include:

```cpp
#include <zcore/strong_typedef.hpp>
```

## Usage Example

```cpp
struct UserTag final {};
using UserToken = zcore::StrongTypedef<UserTag>;
const UserToken token(42ULL);
```

## Warnings and Edge Cases

- `FromRawUnchecked` stores the raw value directly; caller owns semantic validation beyond sentinel checks.
- Choose `InvalidV` to avoid colliding with valid runtime values in your domain.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.
