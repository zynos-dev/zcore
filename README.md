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
- Local fuzzing: configure with `-DZCORE_BUILD_FUZZERS=ON` using `clang++` (non-`clang-cl`) so libFuzzer sanitizer runtimes link correctly.
- Current fuzz targets: `zcore_fuzz_utf8`, `zcore_fuzz_hash_map`, `zcore_fuzz_buffered_io`, `zcore_fuzz_string`.
- Fuzzing sanitizer set is configurable via `-DZCORE_FUZZ_SANITIZERS=<comma-separated list>` (default: `fuzzer,address` on Windows, `fuzzer,address,undefined` elsewhere).
- Persistent discovered inputs are saved in a corpus directory; re-use the same corpus path between local runs to keep learning.
- Manual CI fuzzing is available via `.github/workflows/fuzz.yml` (`workflow_dispatch` only) and restores the latest saved corpus artifact for the selected fuzz target by default.
- Example local fuzz run:
  `cmake -S . -B build-fuzz -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_MAKE_PROGRAM="<path-to-ninja>" -DZCORE_BUILD_TESTS=OFF -DZCORE_BUILD_FUZZERS=ON`
  `cmake --build build-fuzz --target zcore_fuzz_hash_map`
  `mkdir fuzz-corpus`
  `build-fuzz/fuzz/zcore_fuzz_hash_map fuzz-corpus -max_total_time=60 -print_final_stats=1`
  `build-fuzz/fuzz/zcore_fuzz_utf8 fuzz-corpus -max_total_time=60 -print_final_stats=1`
- Hash backend wiring is header-only and uses vendored `xxhash.h`.
- Runtime intrinsics hooks: use `<zcore/intrinsics.hpp>` to provide app-level CPU feature/path selection for modules that dispatch on ISA paths.
