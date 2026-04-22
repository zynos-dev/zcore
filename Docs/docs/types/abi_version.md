# `AbiVersion`

## Purpose

`AbiVersion` is a deterministic ABI epoch/revision value type for binary compatibility contracts.

## Invariants and Contracts

- Value representation: two unsigned 32-bit parts (`u32`): epoch, revision.
- Default/zero state is `0.0`.
- Ordering (`==`, `<=>`) is lexicographic by `(epoch, revision)`.
- Compatibility rule: provider is compatible with required when `epoch` matches and provider `revision >= required revision`.
- `BumpEpoch()` resets `revision` to zero.
- No allocation, no hidden synchronization.

## API Summary

- Construction: `AbiVersion()`, `AbiVersion(epoch, revision)`.
- Factories: `Zero()`, `FromRawUnchecked(epoch, revision)`.
- Accessors: `Epoch()`, `Revision()`.
- State helper: `IsZero()`.
- Compatibility helper: `IsCompatibleWith(required)`.
- Mutation helpers: `SetEpoch()`, `SetRevision()`, `BumpEpoch()`, `BumpRevision()`, `Reset()`.
- Hash adapters: `zcore::hash::Hash<AbiVersion>` and `std::hash<AbiVersion>`.

Public include:

```cpp
#include <zcore/abi_version.hpp>
```

## Usage Example

```cpp
const zcore::AbiVersion provider(2U, 3U);
const zcore::AbiVersion required(2U, 1U);
const bool compatible = provider.IsCompatibleWith(required);
```

## Warnings and Edge Cases

- This type models numeric ABI epoch/revision only; it does not encode source-level API SemVer metadata.
- `FromRawUnchecked` stores provided parts without additional validation.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.
