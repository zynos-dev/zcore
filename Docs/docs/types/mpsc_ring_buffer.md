# `MpscRingBuffer<T, CapacityV>`

## Purpose

`MpscRingBuffer<T, CapacityV>` is a lock-free fixed-capacity queue for many-producer/single-consumer data transfer.

## Invariants and Contracts

- `CapacityV > 0`.
- `T` must be a mutable object type (no references, arrays, functions, or `void`).
- `T` must be move-constructible.
- Multiple producer threads may call push/emplace APIs concurrently.
- Exactly one consumer thread may call pop APIs.
- `TryPush`/`TryEmplace` return `false` when full, no overwrite.
- `TryPop` returns `None` when empty.

## API Summary

- Capacity/state: `Capacity()`, `SizeApprox()`, `Empty()`, `Full()`.
- Producer APIs: `TryPush(const T&)`, `TryPush(T&&)`, `TryEmplace(...)`.
- Consumer API: `TryPop()`.
- Lifecycle: `Clear()`.

Public include:

```cpp
#include <zcore/mpsc_ring_buffer.hpp>
```

## Usage Example

```cpp
zcore::MpscRingBuffer<int, 1024> queue;
queue.TryPush(17);
auto value = queue.TryPop();
```

## Warnings and Edge Cases

- `SizeApprox()` is non-transactional under concurrent producer activity.
- `Clear()` and destruction require external quiescence (no concurrent producer/consumer operations).
- `Clear()`/destruction destroy queued elements in place and do not move queued values.

## Thread-Safety and Ownership Notes

- Lock-free for many producers + one consumer.
- Queue owns inline element storage; no dynamic allocation.
