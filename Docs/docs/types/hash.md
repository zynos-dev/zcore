# `hash.hpp`

## Purpose

`zcore::hash` provides deterministic non-cryptographic hashing over explicit byte views.

## Invariants and Contracts

- Hash results are deterministic for `(algorithm, seed/key, input bytes)`.
- Public API is byte-view based (`ByteSpan`) and allocation-free.
- This module is not cryptographic.

## API Summary

- `Hash64`
- `Hash<T>` customization trait
  - default specialization for integral/enum values
  - specialize for project-specific types
- `HashObject(value, seed)` type-level hash entry point
- `DigestToSizeT(digest)` adapter for `std::size_t`
- `HashAlgorithm`
  - `XXH3_64` (default)
  - `FNV1A_64` (portable simple fallback/compatibility option)
  - `SIPHASH_2_4` (seed-derived or keyed mode)
- `SipHashKey` (`Default()`, `FromSeed(...)`)
- `HashBytes(...)`
- `HashBytesKeyed(...)`
- `HashString(...)`
- `HashValue(...)` for integral/enum values (delegates to canonical `HashObject` / `Hash<T>` path)

Public include:

```cpp
#include <zcore/hash.hpp>
```

## Usage Examples

```cpp
zcore::ByteSpan payload = ...;
zcore::u64 digest = zcore::hash::HashBytes(payload);
```

```cpp
struct EntityTag final {};
using EntityId = zcore::Id<EntityTag>;
zcore::hash::Hash64 digest = zcore::hash::HashObject(EntityId(42));
```

```cpp
auto digest = zcore::hash::HashBytes(
    payload,
    zcore::hash::HashAlgorithm::SIPHASH_2_4,
    12345);
```

## Warnings and Edge Cases

- `XXH3_64` is non-cryptographic and not suitable for integrity/authentication uses.
- `FNV1A_64` is portable/simple but weaker than XXH3 for distribution/performance on most workloads.
- For attacker-controlled keys, prefer keyed hashing (`HashBytesKeyed` with `SipHashKey`).
- Hash values are not stable across algorithm changes; include algorithm/version in persisted formats.
- `std::hash` should be treated as an adapter surface for standard containers; prefer `zcore::hash::Hash<T>` for zcore/container internals.

## Thread-Safety and Ownership Notes

- Functions are pure and thread-safe.
