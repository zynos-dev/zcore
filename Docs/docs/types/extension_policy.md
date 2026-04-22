# `ExtensionPolicy`

## Purpose

`ExtensionPolicy` defines deterministic acceptance rules for forward-compatible `ErrorCode` extensions.

## Invariants and Contracts

- Policy shape is valid only when base domain is a failure domain and `BuiltInMaxCode() > 0`.
- Known code: base domain + positive code in `1..BuiltInMaxCode()`.
- Extension code: any non-success failure code not classified as known.
- Modes:
  - `STRICT`: known base-domain codes only.
  - `SAME_DOMAIN_FORWARD_COMPATIBLE`: known + extension codes in base domain.
  - `ANY_DOMAIN_FORWARD_COMPATIBLE`: known + extension codes in any failure domain.
- `AllowOk` flag controls acceptance of canonical success code.

## API Summary

- Construction: `ExtensionPolicy()`, `ExtensionPolicy(baseDomain, builtInMaxCode, mode, allowOk)`.
- Factories: `Invalid()`, `Strict(...)`, `SameDomainForwardCompatible(...)`, `AnyDomainForwardCompatible(...)`, `FromRawUnchecked(...)`.
- Queries: `IsValid()`, `IsInvalid()`, `BaseDomain()`, `BuiltInMaxCode()`, `Mode()`, `AllowsOk()`, `IsKnownCode(...)`, `IsExtensionCode(...)`, `Allows(ErrorCode)`, `Allows(ErrorInfo)`.
- Mutation: `Reset()`.
- Error helpers: `ExtensionPolicyErrorCode`, `kExtensionPolicyErrorDomain`, `MakeExtensionPolicyError(...)`.

Public include:

```cpp
#include <zcore/extension_policy.hpp>
```

## Usage Example

```cpp
const zcore::ExtensionPolicy policy =
    zcore::ExtensionPolicy::SameDomainForwardCompatible(zcore::kZcoreErrorDomain, 100);
```

## Warnings and Edge Cases

- Invalid policy shape rejects all non-success codes.
- Success-code acceptance requires explicit `allowOk = true`.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.

