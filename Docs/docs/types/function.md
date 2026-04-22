# `Function`

## Purpose

`Function<Signature>` is a move-only allocator-aware callable wrapper with explicit fallible binding.

## Invariants and Contracts

- Target callable must be object type, nothrow move-constructible, and nothrow-invocable for `Signature`.
- `TryMake`/`TryBind` use allocator only when target does not fit inline storage.
- `Invoke`/`operator()` require a bound target; empty-call is a contract violation.
- `Reset()` destroys bound target and deallocates heap storage through the original allocator.

## API Summary

- `template <typename SignatureT> class Function`
- `TryMake(Allocator&, callable) -> Result<Function, Error>`
- `TryBind(Allocator&, callable) -> Status`
- `HasValue()`, `IsEmpty()`, `TargetType()`
- `ContainsTarget<T>()`, `TryTarget<T>()`, `Invoke(...)`, `operator()(...)`, `Reset()`

Public include:

```cpp
#include <zcore/function.hpp>
```

## Thread-Safety and Ownership Notes

- `Function` owns target callable lifetime.
- No internal synchronization.
