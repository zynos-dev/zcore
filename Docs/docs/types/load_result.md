# `LoadResult`

## Purpose

`LoadResult` is the standard fallible return type for plugin loading operations (`Result<PluginDescriptor, Error>`).

## Invariants and Contracts

- Success payload: `PluginDescriptor`.
- Failure payload: `Error`.
- Load-domain failures use `kLoadErrorDomain` and `LoadErrorCode`.
- `LoadResult` is `[[nodiscard]]` through `Result`.
- No hidden allocation or synchronization.

## API Summary

- Alias: `using LoadResult = Result<PluginDescriptor, Error>`.
- Error codes: `LoadErrorCode`
  - `INVALID_DESCRIPTOR`
  - `NOT_FOUND`
  - `ABI_MISMATCH`
  - `INTERFACE_UNSUPPORTED`
  - `BACKEND_FAILURE`
- Domain/helpers:
  - `kLoadErrorDomain`
  - `MakeLoadError(...)`
  - `Loaded(...)`
  - `LoadFailed(...)`

Public include:

```cpp
#include <zcore/load_result.hpp>
```

## Usage Example

```cpp
zcore::LoadResult LoadPluginMetadata() {
  return zcore::LoadFailed(
      zcore::MakeLoadError(zcore::LoadErrorCode::NOT_FOUND, "load", "plugin file missing"));
}
```

## Warnings and Edge Cases

- `Loaded(...)` does not re-validate descriptor; pass validated descriptors in production loaders.
- Use stable operation strings in `MakeLoadError(...)` for cross-project diagnostics consistency.

## Thread-Safety and Ownership Notes

- Thread behavior follows `Result<PluginDescriptor, Error>` value semantics.
