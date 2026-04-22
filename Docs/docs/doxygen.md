# Doxygen API Reference

`zcore` uses Doxygen for API-level reference generated directly from public headers in `include/`.

Generated HTML is a build artifact and is not synced into the source docs tree.

## Generate Locally

Configure with docs enabled:

```sh
cmake -S . -B cmake-build-debug -DZCORE_BUILD_DOCS=ON
```

Disable awesome theme (optional):

```sh
cmake -S . -B cmake-build-debug -DZCORE_BUILD_DOCS=ON -DZCORE_USE_DOXYGEN_AWESOME=OFF
```

Build the Doxygen target:

```sh
cmake --build cmake-build-debug --target zcore_docs_doxygen
```

Or use the aggregate docs target:

```sh
cmake --build cmake-build-debug --target zcore_docs
```

Doxygen output index:

```text
cmake-build-debug/docs/doxygen/html/index.html
```

Warnings log:

```text
cmake-build-debug/docs/doxygen/doxygen-warnings.log
```

## Scope

- Input: `include/` and `README.md`
- Excludes: `tests/` and build directories
- Goal: class/function/contract docs sourced from code comments
- Theme: `doxygen-awesome-css` (enabled by default, fetched by CMake)

## Publishing Strategy

- Do not commit generated Doxygen HTML to the repository.
- Publish Doxygen from CI artifacts (the repository workflow uploads `zcore-doxygen-html`).
- Keep MkDocs source content (`Docs/docs`) limited to authored markdown and static assets.
