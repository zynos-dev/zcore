# `Atomic<T>`

## Purpose

`Atomic<T>` provides typed atomic state with explicit memory-order operations and deterministic low-level synchronization semantics.

## Invariants and Contracts

- `T` must be trivially copyable and non-`const`/non-`volatile`.
- `Atomic<T>` is non-copyable and non-movable.
- Default construction is value-initialized (`T{}`).
- `Store`, `Load`, `Exchange`, and compare-exchange operations accept explicit `std::memory_order`.
- Fetch arithmetic/bitwise operations are available only when the underlying atomic type supports them.
- `kIsAlwaysLockFree` exposes compile-time lock-free capability and `IsLockFree()` exposes runtime capability.

## API Summary

- Construction: `Atomic()`, `Atomic(T initial)`.
- Core ops: `Store`, `Load`, `Exchange`.
- CAS ops: `CompareExchangeWeak`, `CompareExchangeStrong`.
- Optional fetch ops: `FetchAdd`, `FetchSub`, `FetchAnd`, `FetchOr`, `FetchXor`.
- Lock-free queries: `kIsAlwaysLockFree`, `IsLockFree()`.
- `NativeHandle()` for native interop.

Public include:

```cpp
#include <zcore/atomic.hpp>
```

## Usage Example

```cpp
zcore::Atomic<zcore::u32> counter(0U);
counter.FetchAdd(1U, std::memory_order_relaxed);
```

## Warnings and Edge Cases

- Memory ordering is caller policy; incorrect order selection can introduce data races or visibility bugs.
- Compare-exchange may fail spuriously for weak mode; retry loops are required when using `CompareExchangeWeak`.

## Thread-Safety and Ownership Notes

- `Atomic<T>` is synchronization state only and owns no external resources.
- Composite invariants spanning multiple fields require additional synchronization beyond single-atomic operations.
