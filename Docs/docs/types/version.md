# `Version`

## Purpose

`Version` is a deterministic semantic version value type (`major.minor.patch`) for stable API/version contracts.

## Invariants and Contracts

- Value representation: three unsigned 32-bit parts (`u32`): major, minor, patch.
- Default/zero state is `0.0.0`.
- Ordering (`==`, `<=>`) is lexicographic by `(major, minor, patch)`.
- `BumpMajor()` resets `minor`/`patch` to zero.
- `BumpMinor()` resets `patch` to zero.
- No allocation, no hidden synchronization.

## API Summary

- Construction: `Version()`, `Version(major, minor, patch)`.
- Factories: `Zero()`, `FromRawUnchecked(major, minor, patch)`.
- Accessors: `Major()`, `Minor()`, `Patch()`.
- State helpers: `IsZero()`, `IsPreStable()`.
- Mutation helpers: `SetMajor()`, `SetMinor()`, `SetPatch()`, `BumpMajor()`, `BumpMinor()`, `BumpPatch()`, `Reset()`.
- Hash adapters: `zcore::hash::Hash<Version>` and `std::hash<Version>`.

Public include:

```cpp
#include <zcore/version.hpp>
```

## Usage Example

```cpp
zcore::Version api(1U, 4U, 2U);
api.BumpPatch();   // 1.4.3
api.BumpMinor();   // 1.5.0
```

## Warnings and Edge Cases

- This type models numeric SemVer core fields only; prerelease/build metadata is intentionally out of scope.
- `FromRawUnchecked` performs no semantic validation beyond storing the provided parts.

## Thread-Safety and Ownership Notes

- Pure value type; thread behavior follows normal copy-by-value semantics.
