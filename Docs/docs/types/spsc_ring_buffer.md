# `SpscRingBuffer<T, CapacityV>`

## Purpose

`SpscRingBuffer<T, CapacityV>` is a lock-free fixed-capacity queue for single-producer/single-consumer data transfer.

## Invariants and Contracts

- `CapacityV > 0`.
- `T` must be a mutable object type (no references, arrays, functions, or `void`).
- `T` must be move-constructible.
- Exactly one producer thread may call push/emplace APIs.
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
#include <zcore/spsc_ring_buffer.hpp>
```

## Usage Example

```cpp
zcore::SpscRingBuffer<int, 256> queue;
queue.TryPush(42);
auto value = queue.TryPop();
```

## Warnings and Edge Cases

- `SizeApprox()` is non-transactional under concurrent producer/consumer activity.
- `Clear()` and destruction require external quiescence (no concurrent producer/consumer operations).

## Thread-Safety and Ownership Notes

- Lock-free for one producer + one consumer.
- Queue owns inline element storage; no dynamic allocation.
