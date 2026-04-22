# `NonNull<T>`

## Purpose

`NonNull<T>` is a non-owning pointer wrapper that enforces a non-null invariant.

Use it when an API requires a pointer-like reference that must never be null.

## Invariants and Contracts

- `NonNull<T>` never stores `nullptr`.
- Null construction is a contract violation and terminates the process.
- `NonNull<T>` does not own the pointee.
- `NonNull<T>` guarantees non-null only; pointee lifetime/validity is still caller-managed.
- Supports object types and `void`.

## API Summary

- `NonNull<T>(T*)` checked construction.
- `NonNull<T>(T&)` checked-by-construction reference conversion (non-`void` only).
- `Get()`, `Address()`, and explicit `operator T*()`.
- `operator*` / `operator->` for non-`void` `T`.
- `StaticCast<U>()`, `ReinterpretCast<U>()`, `ConstCast()`.
- `UnsafeFromPointerUnchecked(T*)` for privileged/interop paths where invariant is already proven.

Public include:

```cpp
#include <zcore/non_null.hpp>
```

## Usage Examples

```cpp
void Process(zcore::NonNull<const Packet> packet);

Packet packet{};
Process(zcore::NonNull<const Packet>(packet));
```

```cpp
void* raw = AcquireRawBuffer();
auto ptr = zcore::NonNull<void>::UnsafeFromPointerUnchecked(raw);
```

## Warnings and Edge Cases

- `UnsafeFromPointerUnchecked` is an unsafe boundary; passing null is undefined contract behavior.
- Non-null does not imply type correctness after reinterpret casts.
- `NonNull<T>` should not cross ABI boundaries unless layout policy explicitly allows it.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows pointee type and external synchronization policy.
- Ownership is unchanged; this type only strengthens nullability contracts.
