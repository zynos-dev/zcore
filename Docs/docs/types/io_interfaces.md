# `Reader` / `Writer` / `Seeker`

## Purpose

These interfaces define minimal fallible byte I/O contracts for adapters over files, sockets, memory buffers, and protocol streams.

## Invariants and Contracts

- `Reader::Read(ByteSpanMut)` returns bytes read (`0` allowed), or I/O-domain `Error`.
- `Writer::Write(ByteSpan)` returns bytes written (`0` allowed), or I/O-domain `Error`.
- `Writer::Flush()` defaults to `OkStatus()` and is overrideable for buffered sinks.
- `Seeker::Seek(isize, SeekOrigin)` returns absolute byte position or I/O-domain `Error`.
- `Seeker::SeekTo(usize)` and `Rewind()` are deterministic helper adapters over `Seek`.
- I/O-domain errors use `IoErrorCode` and `kIoErrorDomain`.

## API Summary

- Error contracts:
  - `IoErrorCode`: `INVALID_ARGUMENT`, `UNSUPPORTED_OPERATION`, `OUT_OF_RANGE`, `END_OF_STREAM`
  - `kIoErrorDomain`
  - `MakeIoError(...)`
- Interfaces:
  - `class Reader { virtual Result<usize, Error> Read(ByteSpanMut) = 0; }`
  - `class Writer { virtual Result<usize, Error> Write(ByteSpan) = 0; virtual Status Flush(); }`
  - `enum class SeekOrigin { BEGIN, CURRENT, END }`
  - `class Seeker { virtual Result<usize, Error> Seek(isize, SeekOrigin) = 0; SeekTo(...); Rewind(); }`

Public includes:

```cpp
#include <zcore/io_error.hpp>
#include <zcore/reader.hpp>
#include <zcore/writer.hpp>
#include <zcore/seeker.hpp>
```

## Usage Example

```cpp
const auto written = writer->Write(payloadBytes);
const auto position = seeker->Seek(0, zcore::SeekOrigin::END);
```

## Warnings and Edge Cases

- Partial reads/writes are valid; callers handle retry/loop policy.
- Seek implementations must validate range and sign behavior explicitly.

## Thread-Safety and Ownership Notes

- Interfaces provide no implicit synchronization or ownership transfer semantics.
