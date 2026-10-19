# Benchmark: protaco and Protocol Buffers

[한국어](README.ko.md) · [Back to README](../../README.md#84-measured-against-protocol-buffers)

This folder contains code and results comparing protaco 0.6.0 and protobuf 3.21.12 on the same two messages. Prepare a separate build environment to run it.

## Files in this folder

- [schemas/](schemas/): protaco YAML for `Fixed512` and `Mixed256`.
- [bench.proto](bench.proto): the same messages mapped type for type to proto3.
- [bench.cpp](bench.cpp): shared setter templates, round-trip checks, timing, and CSV output.
- [results.csv](results.csv): published measurements and their environment.
- [plot.py](plot.py): reads `results.csv` and generates [benchmark.svg](../img/benchmark.svg).

## Messages

- `Fixed512`: a 512-byte fixed block. One `name`, four `int64` fields, 32 `int` fields, 32 `double` fields, eight `bool` fields, and one `tag`.
- `Mixed256`: a 196-byte fixed block (two `int64` fields, 16 `int` fields, 12 `double` fields, four `bool` fields, and one `string(16)`) plus two variable strings. Their values occupy 24 and 16 bytes. Including keys, types, and lengths, their TLVs occupy 60 bytes, for a total of 256 bytes.

Type mapping: `int` → `int32`, `int64` → `int64`, `double` → `double`, `bool` → `bool`, and `string(N)`/`string` → `string`. Both YAML schemas set `allow_extension: false`.

Every field's setter is called. Integers, floating-point values, and strings use non-default proto3 values. Booleans use `((v + n) & 1) != 0`, so half are `false` and proto3 omits those default-valued fields. This is therefore not a measurement in which proto3 writes every field.

## Measurement method

- Encode: set every field, then serialize. protaco calls `Serialize()`; protobuf calls `SerializeToArray()` with a reused buffer.
- Decode: decode bytes into an object, then read one field, `i0`. protaco calls `TryToObject()`; protobuf calls `ParseFromArray()` on a reused message.
- TryView: validate a `Fixed512` frame using the same rules as `TryToObject`, then read `i0` without copying the frame into a full object.

Each case runs 10,000 warm-up calls followed by 20 rounds of 100,000 calls. Elapsed time per round is divided by the call count; the reported mean and sample standard deviation (sd) are calculated across those rounds. Timing uses `std::chrono::steady_clock`, with volatile values for inputs and results. The published CSV selects, for each row, the run with the median mean time out of three runs. Its mean and sd are kept together.

Both libraries reuse the sending object and decoding output object between calls. However, protaco's `TryToObject` decodes into a new internal object before moving it to the output. protobuf reuses storage in the output message.

Round-trip checks run after encode timing and before decode timing. They verify that decoding and serializing again produces the original bytes, and exit on failure. TryView checks that `i0` matches before its own timed loop.

## Results

Intel Core i9-13900H, Windows 11, MSVC 14.16 (v141), x64 `/O2 /MD`. The comparison uses protaco 0.6.0 and a protobuf 3.21.12 Release static library built with the same toolset. Values come from [results.csv](results.csv).

![Encode and decode times for Fixed512 and Mixed256](../img/benchmark.svg)

Bars show mean time per call; whiskers show standard deviation over 20 rounds. Parentheses show protobuf time divided by protaco time. Each panel has its own vertical scale.

### Fixed512

| Operation | protaco | protobuf |
|---|---|---|
| Encode | 23.3 ns (sd 0.2) | 200.1 ns (sd 2.5) |
| Decode | 55.4 ns (sd 1.6) | 252.3 ns (sd 9.2) |
| Decode (TryView) | 11.9 ns (sd 0.5) | — |

Compared with protobuf, protaco encoding is 8.6× faster (88% less time) and `TryToObject` decoding is 4.6× faster. TryView takes 1/21.2 of protobuf's object-decoding time.

### Mixed256

| Operation | protaco | protobuf |
|---|---|---|
| Encode | 20.7 ns (sd 0.5) | 93.3 ns (sd 3.4) |
| Decode | 126.9 ns (sd 1.4) | 164.5 ns (sd 4.9) |

Compared with protobuf, protaco encoding is 4.5× faster (78% less time) and decoding is 1.3× faster (23% less time).

Frame sizes for reference (protaco / protobuf): Fixed512 556 / 571 B; Mixed256 300 / 268 B.

## Reading the results

protaco fixed-field setters write directly into the frame buffer, so encoding consists of the setter calls. protobuf encodes fields during serialization.

TryView validates the frame and reads only the requested field. Its scope differs from decoding into a full object.

The decoding gap is smaller with strings. protaco creates a new object, so each string exceeding MSVC's 15-byte small-string buffer requires an allocation. protobuf reuses string storage in its output message.

These numbers apply to the messages, values, machine, and compiler above. They do not guarantee performance for other messages or environments.

## Running it

First build protacoc with `build.cmd` at the repository root. Build protobuf 3.21.12 with CMake as well. To match the measured configuration, select v141 x64, Release, a static libprotobuf, and the `/MD` runtime. Use protoc from the same version.

In an x64 Visual Studio developer command prompt, set these environment variables to your actual paths:

- `PROTOBUF_SRC`: protobuf 3.21.12 source root.
- `PROTOBUF_LIB`: full path to the built Release `libprotobuf.lib`.
- `PROTOC`: full path to `protoc.exe` from the same build.

Start these commands at the repository root. protacoc creates `gen/`.

**Windows cmd**

```bat
cd docs\benchmark
..\..\bin\Release\protacoc.exe -I schemas --cpp_out gen
"%PROTOC%" --cpp_out=gen bench.proto
cl /nologo /std:c++17 /O2 /MD /EHsc /Igen /I"%PROTOBUF_SRC%\src" bench.cpp gen\bench.pb.cc /Febench.exe /link "%PROTOBUF_LIB%"
bench.exe
```

Keep output lines starting with `csv,`, strip that prefix, and prepend the CSV header. In PowerShell with the same developer environment, use `docs/benchmark` as the working directory. These commands save one run to `results.csv` and update the figure. `python plot.py` requires matplotlib.

**PowerShell**

```powershell
$rows = .\bench.exe | Where-Object { $_.StartsWith("csv,") } | ForEach-Object { $_.Substring(4) }
@("message,operation,library,mean_ns,sd_ns,frame_bytes") + $rows | Set-Content -Encoding ascii results.csv
python plot.py
```

To use the published aggregation method, retain three separate runs. For each `message`/`operation`/`library` combination, select the run with the median mean time. Copy that row's sd and frame size with it into `results.csv`, then run `python plot.py`.
