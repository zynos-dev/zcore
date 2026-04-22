# `InterfaceId`

## Purpose

`InterfaceId` is a deterministic ABI interface identifier token used to name stable binary contracts.

## Invariants and Contracts

- Value representation: unsigned 64-bit token (`u64`).
- Default state is invalid (`kInvalidValue == 0`).
- `FromLiteral("...")` produces deterministic compile-time ids from canonical interface names.
- `FromName(...)` uses deterministic FNV-1a hashing and returns invalid id for empty names.
- No allocation, no hidden synchronization.

## API Summary

- Construction: `InterfaceId()`, `InterfaceId(raw)`.
- Factories: `Invalid()`, `FromRawUnchecked(raw)`, `FromName(name)`, `FromLiteral("name")`.
- Access/state: `Value()`, `IsValid()`, `IsInvalid()`, `Reset()`.
- Operators: `==`, `<=>`, explicit `bool`.
- Hash adapters: `zcore::hash::Hash<InterfaceId>` and `std::hash<InterfaceId>`.

Public include:

```cpp
#include <zcore/interface_id.hpp>
```

## Usage Example

```cpp
constexpr zcore::InterfaceId kReader = zcore::InterfaceId::FromLiteral("zcore.io.reader.v1");
const zcore::InterfaceId runtimeId = zcore::InterfaceId::FromName("zcore.io.reader.v1");
const bool same = (kReader == runtimeId);
```

## Warnings and Edge Cases

- Canonical name strings must be versioned and stable across products/releases.
- Distinct names can theoretically collide under any hash scheme.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.
