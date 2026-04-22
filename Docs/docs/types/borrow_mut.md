# `BorrowMut<T>`

## Purpose

`BorrowMut<T>` provides a non-owning mutable non-null pointer wrapper for explicit borrowed mutation.

## Invariants and Contracts

- `BorrowMut<T>` is non-owning and does not extend lifetime.
- Construction enforces non-null pointer contract.
- Access is mutable (`T*` / `T&`).
- `BorrowMut<void>` is supported for opaque borrowed mutable pointers.
- `BorrowMut<T>` is trivially copyable and pointer-sized.
- `BorrowMut<const T>` is rejected at compile time.

## API Summary

- `BorrowMut(T*)`.
- `BorrowMut(T&)`.
- converting constructors from compatible `BorrowMut<U>` and `NonNull<U>`.
- `UnsafeFromPointerUnchecked(T*)`.
- `IsValidPointer(T*)`.
- `Get()`, `Address()`.
- `AsBorrow()`, implicit conversion to `Borrow<T>`.
- `operator*`, `operator->` (non-void types).

Public include:

```cpp
#include <zcore/borrow_mut.hpp>
```

## Usage Example

```cpp
Widget widget{.value = 7};
zcore::BorrowMut<Widget> borrowed(widget);
borrowed->Set(8);
zcore::Borrow<Widget> readonly = borrowed;
```

## Warnings and Edge Cases

- Null pointer construction is a contract violation.
- Lifetime remains external; dangling borrows are caller errors.

## Thread-Safety and Ownership Notes

- `BorrowMut<T>` has no internal synchronization.
- Thread-safety follows referenced object and external synchronization policy.
