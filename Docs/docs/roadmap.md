# Roadmap

Implementation sequencing:

1. Foundation types (primitive aliases, `NonNull`, `Id`, `Flags`, `Handle`, `TypeId`)
2. Error layer (`ErrorDomain`, `ErrorCode`, `Error`, `Option`, `Result`, `Status`)
3. Memory layer (`Allocator`, `Arena`, ownership and slice/span types)
4. Deterministic containers, hash, and text
5. Concurrency/time primitives
6. I/O, serialization, and diagnostics interfaces

Detailed drafts:
- `Docs/Temp/zcore-design-principles.md`
- `Docs/Temp/zcore-implementation-checklist.v1.md`
- `Docs/Temp/zcore-production-architecture-guide.md`
