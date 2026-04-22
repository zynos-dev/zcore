# `Handle<TagT, IndexT, GenerationT>`

## Purpose

`Handle<TagT, IndexT, GenerationT>` provides strongly typed stable slot identity using `(index, generation)` pairs.

## Invariants and Contracts

- Handles are tag-scoped: `Handle<EntityTag>` is distinct from `Handle<ResourceTag>`.
- Default state is invalid (`kInvalidIndex`, `kInvalidGeneration`).
- Validity requires both index and generation to be non-sentinel values.
- `Handle` is a deterministic value type with no allocation.

## API Summary

- `Handle<TagT>()` creates invalid handle.
- `Handle<TagT>(index, generation)` explicit construction.
- `Invalid()` returns canonical invalid handle.
- `FromRawUnchecked(index, generation)` explicit unchecked factory.
- `Index()` / `Generation()` return raw parts.
- `IsValid()` / `IsInvalid()` check validity.
- `SameSlot(other)` checks index equality.
- `Reset()` restores invalid state.
- Full equality and ordering operators (`==`, `<=>`).
- `zcore::hash::Hash<Handle<...>>` specialization for zcore hashing.
- `std::hash` specialization for unordered containers.

Public include:

```cpp
#include <zcore/handle.hpp>
```

## Usage Examples

```cpp
struct EntityTag final {};
using EntityHandle = zcore::Handle<EntityTag>;

EntityHandle handle(42U, 3U);
if (handle.IsValid()) {
  auto index = handle.Index();
  auto generation = handle.Generation();
}
```

```cpp
std::unordered_set<zcore::Handle<EntityTag>> active;
active.insert(zcore::Handle<EntityTag>(7U, 1U));
```

## Warnings and Edge Cases

- `FromRawUnchecked` does not enforce validity checks.
- Sentinel values are template-configurable; choose custom sentinels carefully.
- `SameSlot` compares index only, not generation.

## Thread-Safety and Ownership Notes

- `Handle` is trivially copyable and has no ownership behavior.
- Thread-safety follows normal value-type semantics.

