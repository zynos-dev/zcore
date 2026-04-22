# `Slice<T>` and `SliceMut<T>`

## Purpose

`Slice<T>` and `SliceMut<T>` are non-owning contiguous views over memory ranges.

- `Slice<T>` is read-only.
- `SliceMut<T>` is mutable and converts to `Slice<const T>`.

## Invariants and Contracts

- Views never allocate.
- Range contract: `size > 0` requires non-null pointer.
- `SliceMut<T>` requires exclusive mutable alias discipline from the caller.
- Lifetimes are external; views are invalid after backing storage invalidation/reallocation.

## API Summary

- Construction from pointer+size, C arrays, `std::array`, and `std::span`.
- Checked/fallible raw construction:
  - `TryFromRaw(ptr, size) -> Option<...>`
- Unchecked raw construction:
  - `FromRawUnchecked(ptr, size)`
- Access:
  - `Data/data`, `Size/size`, `EmptyView/empty`, `begin`, `end`
  - `TryAt(index)`, `operator[]`
- Range operations:
  - `First`, `Last`, `Subslice`
  - `RemovePrefix`, `RemoveSuffix`, `Clear`
- Conversions:
  - `SliceMut<T>::AsConst()` and implicit conversion to `Slice<const T>`
- Byte views:
  - `ByteSpan` (`Slice<const Byte>`)
  - `ByteSpanMut` (`SliceMut<Byte>`)
  - `AsBytes(...)`, `AsWritableBytes(...)`

Public include:

```cpp
#include <zcore/slice.hpp>
```

## Usage Examples

```cpp
int values[]{1, 2, 3, 4};
zcore::Slice<int> read(values);
auto middle = read.Subslice(1, 2);
```

```cpp
zcore::SliceMut<int> write(values);
write[0] = 9;
zcore::Slice<const int> readonly = write;
```

## Warnings and Edge Cases

- Checked pointer+size constructors terminate on `nullptr` with non-zero length.
- `operator[]` is bounds-preconditioned (debug assert).
- `TryFromRaw` should be used at untrusted boundaries.

## Thread-Safety and Ownership Notes

- No internal synchronization.
- Thread-safety follows pointee data and external synchronization policy.
- Ownership remains with backing storage.
