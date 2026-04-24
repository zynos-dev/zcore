# `BufferedWriter`

## Purpose

`BufferedWriter` is an allocator-backed `Writer` adapter that buffers writes before flushing to an upstream sink.

## Invariants and Contracts

- Dynamic storage is allocated only through the bound `Allocator`.
- `Write(ByteSpan)` returns bytes accepted from source or propagated upstream/allocator error.
- When upstream failure occurs after some bytes were accepted in the same call, `Write` returns the accepted byte count and defers error reporting to a later call.
- `Flush()` first drains buffered bytes to upstream, then delegates upstream `Flush()`.
- `capacity = 0` disables buffering and forwards writes directly to upstream `Writer`.
- Zero-byte upstream progress during flush is treated as `IoErrorCode::END_OF_STREAM`.

## API Summary

- Construction: `BufferedWriter(Writer&, Allocator&, usize capacity = 4096)`.
- Shape/state: `Capacity`, `BufferedSize`.
- I/O: `Write(ByteSpan)`, `Flush()` (overrides of `Writer`).

Public include:

```cpp
#include <zcore/buffered_writer.hpp>
```

## Usage Example

```cpp
zcore::BufferedWriter writer(sink, allocator, 4096U);
writer.Write(payloadBytes);
writer.Flush();
```

## Warnings and Edge Cases

- Data may remain buffered until `Flush()` or buffer-full auto-drain path.
- Upstream partial writes are retried during flush until complete or error.
- If a flush error occurs after partial upstream progress, already-flushed prefix bytes are consumed and not retried.

## Thread-Safety and Ownership Notes

- Adapter owns internal byte storage; it does not own upstream `Writer`.
- External synchronization is required for shared concurrent access.
