# Contracts

## Ownership and Memory

- No owning raw pointers in public APIs.
- Use `NonNull<T>` when pointer contracts require non-null by type.
- Use tag-scoped `Id<TagT>` for cross-domain identifier safety.
- Use `Slice<T>` / `SliceMut<T>` for contiguous non-owning range contracts.
- Dynamic allocation must be allocator-explicit.
- Ownership transfer must be visible in the type system.

## Errors and Absence

- Recoverable failures return `Result<T, E>` or `Status`.
- Normal absence returns `Option<T>`.
- Exceptions are not used for normal control flow.
- `Option` and `Result` are `[[nodiscard]]` and must be handled explicitly.
- Use panic/fatal paths only for invariant violations or unrecoverable states.
- Contract violations route through a centralized fatal handler (`zcore::detail::ContractViolation`).

### Result Handling Guidance

- Prefer `TryValue()` / `TryError()` in recovery-first code.
- Use `Map`, `MapError`, `AndThen`, and `OrElse` for explicit flow composition.
- Treat direct `Value()` / `Error()` access as preconditioned operations.

## Determinism

- Runtime-critical containers document memory and iteration guarantees.
- Runtime intrinsics dispatch may change code path, never hash/result semantics.
- Hash APIs must encode algorithm + seed/key policy explicitly at call sites that persist/interchange digests.
- Hidden synchronization and hidden global state are forbidden in core paths.

## C++ Ergonomics

- APIs remain idiomatic C++ (`size`, `empty`, `begin`, `end`, `data`).
- RAII and move semantics are preferred.
