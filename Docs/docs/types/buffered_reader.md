# `BufferedReader`

## Purpose

`BufferedReader` is an allocator-backed `Reader` adapter that batches upstream reads through an internal byte buffer.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- `Read(ByteSpanMut)` returns bytes read (`0` allowed), or propagated upstream/allocator error.
- When upstream failure occurs after some bytes were produced in the same call, `Read` returns the produced byte count and defers error reporting to a later call.
- `capacity = 0` disables buffering and forwards reads directly to upstream `Reader`.
- Buffered unread bytes are observable via `BufferedSize()` and discardable with `ClearBuffer()`.
- No implicit synchronization.

## API Summary

- Construction: `BufferedReader(Reader&, Allocator&, usize capacity = 4096)`.
- Shape/state: `Capacity`, `BufferedSize`, `ClearBuffer`.
- I/O: `Read(ByteSpanMut)` (override of `Reader`).

Public include:

```cpp
#include <zcore/buffered_reader.hpp>
```

## Usage Example

```cpp
zcore::BufferedReader reader(source, allocator, 4096U);
auto result = reader.Read(destinationBytes);
```

## Warnings and Edge Cases

- `ClearBuffer` drops prefetched unread bytes.
- Large destination reads may bypass internal buffering when buffer is empty.

## Thread-Safety and Ownership Notes

- Adapter owns internal byte storage; it does not own upstream `Reader`.
- External synchronization is required for shared concurrent access.
