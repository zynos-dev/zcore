# `Flags<EnumT>`

## Purpose

`Flags<EnumT>` provides a type-safe enum-backed bitmask for explicit multi-flag state.

Use it when an API needs composable flag sets without exposing raw integers.

## Invariants and Contracts

- `EnumT` must be an enum type.
- Storage is deterministic and value-type only (no allocation, no ownership behavior).
- `Raw()` returns the exact current bit pattern.
- `Has`, `HasAny`, and `HasAll` are pure bitmask checks.
- `Flags` is `[[nodiscard]]`; dropped values should be intentional.

## API Summary

- `Flags<EnumT>()` creates empty flags.
- `Flags<EnumT>(EnumT)` enables one mask value.
- `None()` returns empty flags.
- `FromRawUnchecked(bits)` explicitly wraps raw mask bits.
- `Raw()`, `IsEmpty()`, `Any()`, and explicit `operator bool()`.
- `Has(flag)`, `HasAny(other)`, `HasAll(other)`.
- `Set`, `Clear`, `Toggle`, `Assign`, `ClearAll`.
- Bitwise ops for `Flags` and `EnumT`: `|`, `&`, `^`, `~` (+ assignment variants).
- `zcore::hash::Hash<Flags<...>>` and `std::hash<Flags<...>>` adapters are provided.

Public include:

```cpp
#include <zcore/flags.hpp>
```

## Usage Examples

```cpp
enum class Permission : zcore::u8 {
  READ = 1U << 0U,
  WRITE = 1U << 1U,
  EXECUTE = 1U << 2U,
};

zcore::Flags<Permission> permissions = zcore::Flags<Permission>(Permission::READ) | Permission::WRITE;
if (permissions.Has(Permission::READ)) {
  permissions.Clear(Permission::READ);
}
```

```cpp
std::unordered_set<zcore::Flags<Permission>> cache;
cache.insert(zcore::Flags<Permission>(Permission::READ) | Permission::EXECUTE);
```

## Warnings and Edge Cases

- `FromRawUnchecked` does not validate domain-specific mask policies.
- `~flags` flips all storage bits; mask it if your domain only allows a subset.
- `Has(flag)` is meaningful only for non-zero enum mask values.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread safety follows normal value-type usage and external synchronization policy.
- `Flags<EnumT>` never owns external resources.
