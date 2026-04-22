# Failure Handling Flow

Use this decision order for public APIs:

1. If absence is normal and expected, return `Option<T>`.
2. If operation can fail recoverably, return `Result<T, E>` (domain-specific `E`) or `Status`.
3. Panic/fatal paths are reserved for invariant violations or unrecoverable system states.

## Practical Rules

- Treat `Option`/`Result` as mandatory handling types (`[[nodiscard]]`).
- Prefer branch-safe accessors first:
  - `Option`: `HasValue()`
  - `Result`: `HasValue()`, `HasError()`, `TryValue()`, `TryError()`
- Prefer combinator chains over ad-hoc branching where it improves clarity:
  - `Map`, `MapOr`, `MapOrElse`, `MapError`, `AndThen`, `OrElse`
  - `Inspect`, `InspectError`

## Error Payload Guidance

- Keep `ErrorCode` stable and machine-readable.
- Keep `ErrorContext` lightweight (`subsystem`, `operation`, `message`, source location).
- Use domain-specific `ErrorDomain` IDs for cross-project consistency.

## Current Accessor Policy

- `Value()` / `Error()` are preconditioned accessors.
- Calling them in the wrong state is a contract violation.
- Use `Try*` accessors in recovery-first code paths.
- Use `Ok()` / `Err()` to explicitly project branch state to `Option`.
