# `Foundation` Aliases

## Purpose

`zcore` foundation aliases provide stable primitive naming across repositories.

## Invariants and Contracts

- Aliases map directly to standard fixed-width and size-related C++ types.
- These aliases do not introduce runtime overhead.
- `Byte` is `std::byte` and is intended for binary/byte-oriented APIs.

## API Summary

- Unsigned fixed-width: `u8`, `u16`, `u32`, `u64`
- Signed fixed-width: `i8`, `i16`, `i32`, `i64`
- Size-related: `usize`, `isize`
- Byte-oriented: `Byte`

Public include:

```cpp
#include <zcore/foundation.hpp>
```

## Usage Examples

```cpp
zcore::u32 frameCount = 0;
zcore::usize itemCount = 128;
zcore::Byte first = std::byte{0xFF};
```

## Warnings and Edge Cases

- Use fixed-width aliases for serialized/networked data where exact size matters.
- Prefer `usize`/`isize` for memory sizes and indexing arithmetic.

## Thread-Safety and Ownership Notes

- These aliases are pure type aliases and have no synchronization or ownership behavior by themselves.
