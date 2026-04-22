# `CpuFeatures` Runtime Detection

## Purpose

`CpuFeatures` provides runtime CPU capability detection for architecture-specific dispatch (for example hashing/vectorized paths) without changing API semantics.

## Invariants and Contracts

- Detection is explicit (`DetectCpuFeatures`) with cached access (`GetCpuFeatures`).
- Runtime detection changes implementation path only; algorithm outputs must remain deterministic.
- x86/x64 detection uses CPUID and XGETBV checks for AVX/AVX2 OS support.
- ARM feature flags are best-effort in header-only mode.

## API Summary

- `enum class CpuArchitecture`
- `struct CpuFeatures`
  - x86 flags: `sse2`, `sse41`, `avx`, `avx2`, `fma`, `bmi1`, `bmi2`, `aesNi`, `pclmulQdq`, `osXsave`
  - ARM flags: `neon`, `aes`, `crc32`
- `DetectCpuFeatures()`
- `GetCpuFeatures()`

Public include:

```cpp
#include <zcore/cpu_features.hpp>
```

## Usage Example

```cpp
const zcore::CpuFeatures features = zcore::GetCpuFeatures();
if (features.avx2) {
  // dispatch AVX2 path
} else {
  // dispatch scalar path
}
```

## Warnings and Edge Cases

- ARM flags may reflect compile-time feature visibility in this initial header-only implementation.
- Feature presence should not be treated as a security boundary.
- For app-level override of feature/path policy, use `zcore::intrinsics` hooks.

## Thread-Safety and Ownership Notes

- `GetCpuFeatures()` uses static initialization and is safe for concurrent reads.
