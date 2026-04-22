# `Aabb2`

## Purpose

`Aabb2` is a deterministic 2D axis-aligned bounding box with canonicalized min/max corners.

## Invariants and Contracts

- Stored as `min`/`max` `Vec2`; constructor canonicalizes unordered inputs.
- Default state: zero-size bounds at origin.
- `Contains` and `Intersects` use inclusive edges.
- `ExpandToInclude` and `Merge` monotonically grow bounds.

## Usage Example

```cpp
#include <zcore/aabb2.hpp>

zcore::Aabb2 bounds(zcore::Vec2(4.0F, -2.0F), zcore::Vec2(-1.0F, 5.0F));
```

## Warnings and Edge Cases

- Equality uses exact float comparison.

## Thread-Safety and Ownership Notes

- Pure value type; copy/share by value.

