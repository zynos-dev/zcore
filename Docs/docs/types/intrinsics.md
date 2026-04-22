# `intrinsics.hpp`

## Purpose

`zcore::intrinsics` provides process-level hooks for runtime CPU feature sourcing and ISA path selection.

## Invariants and Contracts

- Default behavior uses builtin `GetCpuFeatures()` and normalized path selection.
- Feature/path hooks are explicit and process-global.
- Dispatch path selection changes implementation path only, never algorithm output contracts.
- Hook registration is allocation-free and thread-safe via atomic callback pointers.

## API Summary

- `enum class Path`
  - `SCALAR`
  - `X86_SSE2`
  - `X86_AVX2`
  - `ARM_NEON`
- Hook types:
  - `FeatureProviderFn = CpuFeatures (*)() noexcept`
  - `PathSelectorFn = Path (*)(const CpuFeatures&) noexcept`
- Selection helpers:
  - `SelectPathForFeatures(const CpuFeatures&)`
  - `GetFeatures()`
  - `GetPath()`
  - `SelectDispatch(path, scalar, x86Sse2, x86Avx2, armNeon)`
- Hook control:
  - `SetFeatureProvider(...)`, `ResetFeatureProvider()`
  - `SetPathSelector(...)`, `ResetPathSelector()`

Public include:

```cpp
#include <zcore/intrinsics.hpp>
```

## Usage Example

```cpp
static zcore::CpuFeatures AppFeatures() noexcept { return zcore::GetCpuFeatures(); }
zcore::intrinsics::SetFeatureProvider(&AppFeatures);
const zcore::intrinsics::Path path = zcore::intrinsics::GetPath();
```

## Warnings and Edge Cases

- Hooks are global to the process; set/reset during startup, before worker threads.
- Callbacks must be `noexcept` and deterministic for stable runtime behavior.

## Thread-Safety and Ownership Notes

- Callback pointers are atomically loaded/stored.
- No ownership transfer; caller retains callback lifetime responsibility.
