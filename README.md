# zcore

`zcore` is the foundational C++ library for Zynos Software products.

This repository is intended to provide:
- strongly typed base primitives
- explicit fallibility/absence contracts
- deterministic fixed-capacity containers
- portable low-level utilities with explicit performance and safety contracts

See `Docs/Temp/` for initial design and roadmap drafts.

Canonical public include style is `<zcore/type_name.hpp>`; prefer facade headers over categorized paths.

## Build Notes

- Test framework: GoogleTest.
- Documentation: MkDocs and Doxygen API reference.
- Doxygen target: configure with `-DZCORE_BUILD_DOCS=ON`, then build `zcore_docs_doxygen`.
- Aggregate docs target: `zcore_docs` (depends on `zcore_docs_doxygen`).
- Doxygen theme: `doxygen-awesome-css` enabled by default (`-DZCORE_USE_DOXYGEN_AWESOME=OFF` to disable).
- Doxygen output: `<build-dir>/docs/doxygen/html/index.html`.
- Doxygen HTML is treated as CI/runtime artifact output, not source-controlled docs content.
- CI workflow `.github/workflows/docs.yml` publishes `zcore-doxygen-html` and `zcore-doxygen-warnings` artifacts.
- On Windows with `clang` (MSYS2/clang64), test executables require runtime DLLs (`libc++.dll`, `libunwind.dll`, and related clang runtime DLLs).
- `tests/CMakeLists.txt` copies available clang runtime DLLs beside `zcore_tests.exe` and registers tests with `gtest_add_tests(...)`.
- Hash backend wiring is header-only and uses vendored `xxhash.h`.
- Runtime intrinsics hooks: use `<zcore/intrinsics.hpp>` to provide app-level CPU feature/path selection for modules that dispatch on ISA paths.
