# Include Policy

Canonical public includes use top-level headers:

```cpp
#include <zcore/type_name.hpp>
```

Rules:

- Use `<zcore/*.hpp>` as the stable API entrypoint for product code.
- Treat categorized headers (for example `<zcore/container/*.hpp>`, `<zcore/thread/*.hpp>`) as implementation-layer headers behind top-level facades.
- Do not add new consumer-facing documentation or examples that include categorized paths when a top-level facade exists.

Rationale:

- Top-level includes keep API discovery deterministic.
- Facade-to-module mapping preserves internal layering without expanding the canonical surface.

Migration:

```cpp
// Prefer
#include <zcore/vector.hpp>

// Avoid in product code
#include <zcore/container/vector.hpp>
```
