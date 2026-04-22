# `ByteBuffer`

## Purpose

`ByteBuffer` is an allocator-backed owning growable byte container for binary I/O and serialization paths.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- Unbound buffers (`ByteBuffer()`) cannot grow; fallible growth returns allocator `UNSUPPORTED_OPERATION`.
- Growth and append failures are recoverable (`Status`/`Result`), not exception-based.
- `operator[]` requires `index < Size()`.
- `Front`/`Back` require non-empty state.
- Type is move-only.

## API Summary

- Construction:
  - `ByteBuffer()`
  - `ByteBuffer(Allocator&)`
  - `TryWithCapacity(Allocator&, usize)`
- Shape/state:
  - `Size/size`, `Capacity`, `RemainingCapacity`, `Empty/empty`
  - `HasAllocator`, `AllocatorRef`
- Access:
  - `Data/data`, `begin/end`, `cbegin/cend`
  - `TryAt`, `Front`, `Back`, `operator[]`
- Mutation:
  - `TryReserve`, `TryResize`
  - `TryAssign(ByteSpan)`, `TryAppend(ByteSpan)`
  - `TryPushBack`, `TryPopBack`, `TryPopBackValue`
  - `Clear`, `Reset`
- View adapters:
  - `AsBytes`, `AsBytesMut`, `operator ByteSpan`

Public include:

```cpp
#include <zcore/byte_buffer.hpp>
```

## Usage Example

```cpp
zcore::ByteBuffer buffer(allocator);
buffer.TryAppend(zcore::ByteSpan(packetBytes));
```

## Warnings and Edge Cases

- `TryAssign` on unbound non-empty input fails with allocator-domain error.
- `Reset` releases owned storage through the bound allocator.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Buffer owns its storage; callers own synchronization policy.
