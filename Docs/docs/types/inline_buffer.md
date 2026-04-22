# `InlineBuffer<CapacityV>`

## Purpose

`InlineBuffer` is a deterministic owning fixed-capacity byte buffer with no allocator dependency.

## Invariants and Contracts

- Capacity is fixed at compile time; no heap allocation.
- `TryAssign`/`TryAppend`/`TryResize` fail instead of growing past capacity.
- `Assign`/`Append`/`Resize` are contract-checked fixed-capacity operations.
- `Front`/`Back`/`operator[]` require valid non-empty/in-range state.
- `Try*` APIs provide non-terminating fallible alternatives.

## API Summary

- Capacity/state: `Capacity`, `Size/size`, `RemainingCapacity`, `Empty/empty`, `Full`.
- Access: `Data/data`, `begin/end`, `cbegin/cend`, `TryAt`, `Front`, `Back`, `operator[]`.
- Mutation: `TryAssign`/`Assign`, `TryAppend`/`Append`, `TryResize`/`Resize`, `TryPushBack`/`PushBack`.
- Removal: `TryPopBack`, `PopBack`, `TryPopBackValue`, `Clear`.
- View adapters: `AsBytes`, `AsBytesMut`, `operator ByteSpan`.

Public include:

```cpp
#include <zcore/inline_buffer.hpp>
```

## Usage Example

```cpp
zcore::InlineBuffer<16> buffer;
buffer.TryAppend(zcore::ByteSpan(packetHeader));
```

## Warnings and Edge Cases

- Zero-capacity buffers are valid and can only hold empty content.
- `TryAssign` leaves existing bytes unchanged when input exceeds capacity.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Buffer owns inline bytes only; synchronization policy is external.
