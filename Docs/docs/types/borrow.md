# `Borrow<T>`

## Purpose

`Borrow<T>` provides a non-owning immutable non-null pointer wrapper for explicit borrowed access.

## Invariants and Contracts

- `Borrow<T>` is non-owning and does not extend lifetime.
- Construction enforces non-null pointer contract.
- Access is immutable (`const T*` / `const T&`).
- `Borrow<void>` is supported for opaque borrowed pointers.
- `Borrow<T>` is trivially copyable and pointer-sized.

## API Summary

- `Borrow(const T*)`.
- `Borrow(const T&)`.
- converting constructors from compatible `Borrow<U>` and `NonNull<U>`.
- `UnsafeFromPointerUnchecked(const T*)`.
- `IsValidPointer(const T*)`.
- `Get()`, `Address()`.
- `operator*`, `operator->` (non-void types).

Public include:

```cpp
#include <zcore/borrow.hpp>
```

## Usage Example

```cpp
const Widget widget{.value = 7};
zcore::Borrow<Widget> borrowed(widget);
int value = borrowed->Read();
```

## Warnings and Edge Cases

- Null pointer construction is a contract violation.
- Lifetime remains external; dangling borrows are caller errors.

## Thread-Safety and Ownership Notes

- `Borrow<T>` has no internal synchronization.
- Thread-safety follows referenced object and external synchronization policy.

