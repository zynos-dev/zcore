# `Aabb3`

## Purpose

`Aabb3` is a deterministic 3D axis-aligned bounding box with canonicalized min/max corners.

## Invariants and Contracts

- Stored as `min`/`max` `Vec3`; constructor canonicalizes unordered inputs.
- Default state: zero-size bounds at origin.
- `Contains` and `Intersects` use inclusive edges.
- `ExpandToInclude` and `Merge` monotonically grow bounds.

## Usage Example

```cpp
#include <zcore/aabb3.hpp>

zcore::Aabb3 bounds(zcore::Vec3(-1.0F, -1.0F, -1.0F), zcore::Vec3(1.0F, 1.0F, 1.0F));
```

## Warnings and Edge Cases

- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

