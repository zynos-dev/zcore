# `Allocator`

## Purpose

`Allocator` defines a fallible allocation/deallocation interface with explicit size/alignment contracts.

## Invariants and Contracts

- Allocation alignment must be a non-zero power of two.
- `Allocation` is valid when:
  - `size == 0` and `data == nullptr`, or
  - `size > 0`, `data != nullptr`, and `data` is aligned to `alignment`.
- `Allocate(...)` returns `Result<Allocation, Error>`.
- `Deallocate(...)` returns `Status`.
- Invalid requests/descriptors return allocator-domain errors (`kAllocatorErrorDomain`).
- Reporting policy is caller-owned; use `ErrorHandler` helpers when forwarding failures to logging/telemetry sinks.

## API Summary

- `AllocatorErrorCode` (`INVALID_REQUEST`, `OUT_OF_MEMORY`, `INVALID_ALLOCATION`, `UNSUPPORTED_OPERATION`).
- `MakeAllocatorError(...)` allocator-domain error constructor.
- `IsPowerOfTwo(...)`, `IsValidAllocationAlignment(...)`, `IsAddressAligned(...)`.
- `AllocationRequest { size, alignment }`.
- `AllocationRequest::WithDefaultAlignment(sizeBytes)`.
- `Allocation { data, size, alignment }`.
- `Allocation::Empty()`, `IsEmpty()`, `IsValid()`, `AsBytes()`.
- `ValidateAllocationRequest(...)`, `ValidateAllocation(...)`.
- Error reporting helpers: `HandleIfError(Result<T, Error>, ...)`, `HandleIfError(Status, ...)`.
- `Allocator` interface:
  - `Allocate(AllocationRequest)`
  - `Deallocate(Allocation)`
  - `AllocateBytes(size, alignment)`

Public include:

```cpp
#include <zcore/allocator.hpp>
```

## Usage Examples

```cpp
zcore::AllocationRequest request{.size = 256U, .alignment = 16U};
auto allocationResult = allocator->Allocate(request);
if (allocationResult.HasValue()) {
  zcore::Allocation block = allocationResult.Value();
  auto status = allocator->Deallocate(block);
}
```

## Warnings and Edge Cases

- `Allocation::AsBytes()` is empty for zero-sized allocations.
- `Deallocate(...)` must receive a valid descriptor shape; malformed descriptors return error.
- Implementations may return `UNSUPPORTED_OPERATION` for unsupported semantics (for example non-freeing arenas).

## Thread-Safety and Ownership Notes

- Interface does not imply thread safety.
- `Allocation` is a non-owning descriptor; ownership/lifetime policy is implementation-defined.
