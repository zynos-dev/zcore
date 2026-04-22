# `Endian` Utilities

## Purpose

`zcore::endian` provides explicit byte-order conversion helpers for integer/enum values at serialization and I/O boundaries.

## Invariants and Contracts

- Supported value types: integral and enum (excluding `bool`).
- `ByteSwap` is identity for 1-byte values.
- `ToLittleEndian` / `FromLittleEndian` and `ToBigEndian` / `FromBigEndian` form deterministic round-trip pairs.
- Helpers are `constexpr` and allocation-free.

## API Summary

- Native endian checks:
  - `IsNativeLittleEndian()`
  - `IsNativeBigEndian()`
- Conversion primitives:
  - `ByteSwap(value)`
  - `ToLittleEndian(value)`, `FromLittleEndian(value)`
  - `ToBigEndian(value)`, `FromBigEndian(value)`
- Short aliases:
  - `to_le(value)`, `from_le(value)`
  - `to_be(value)`, `from_be(value)`

Public include:

```cpp
#include <zcore/endian.hpp>
```

## Usage Example

```cpp
const zcore::u32 networkOrder = zcore::endian::to_be(0x11223344U);
const zcore::u32 hostOrder = zcore::endian::from_be(networkOrder);
```

## Warnings and Edge Cases

- Conversions are value-level helpers, not buffer parsers.
- Use explicit fixed-width integer types for stable binary protocol semantics.

## Thread-Safety and Ownership Notes

- Pure value transforms; no shared state or ownership implications.
