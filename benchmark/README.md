# Benchmark: protaco and Protocol Buffers

[한국어](README.ko.md) · [Back to README](../README.md#8-performance)

This folder holds only what the measurement used. Build it yourself if you want to run it.

- [schemas/](schemas/): the three messages as protaco YAML.
- [bench.proto](bench.proto): the same messages as proto3, type for type.
- [bench.cpp](bench.cpp): the measurement. One template fills both libraries' messages with the same values (both name setters `set_<field>`).

## Messages

- `Fixed512`: fixed fields only, 512-byte fixed block. 1 name, 4 int64, 32 int, 32 double, 8 bool, 1 tag.
- `Fixed1K`: fixed fields only, 1,024-byte fixed block. 1 name, 8 int64, 64 int, 64 double, 16 bool, 1 tag.
- `Mixed256`: 196-byte fixed block and two variable `string` fields (24 and 16 bytes, 60 bytes with their keys), 256 bytes in all.

Type mapping: `int` → `int32`, `int64` → `int64`, `double` → `double`, `bool` → `bool`, `string(N)` and `string` → `string`. Every value is non-default, so proto3 writes every field.

## What is measured

- Encode: set every field, then serialize. protaco `Serialize()`; protobuf `SerializeToArray()` into a reused buffer.
- Serialize again: serialize the same object once more, nothing changed.
- Decode: bytes to object. protaco `TryToObject()`; protobuf `ParseFromArray()` into a reused message.

Objects are reused across calls, as both libraries recommend. Each case runs 10,000 warm-up calls, then 20 rounds of 100,000 calls; the result is the mean time per call. Before timing, each decode is checked to give back the same bytes.

## Results

Intel Core i9-13900H, Windows 11, MSVC 14.16 (v141) x64 `/O2 /MD`. protobuf 3.21.12, static Release library built with the same toolset. protaco 0.5.0.

| Message, operation | protaco / protobuf | protaco time |
|---|---|---|
| Fixed512 encode | 20.6 / 214.8 ns | 90% less (10.4x) |
| Fixed512 serialize again | 1.2 / 196.5 ns | 99% less |
| Fixed512 decode | 61.6 / 240.1 ns | 74% less (3.9x) |
| Fixed1K encode | 40.1 / 482.2 ns | 92% less (12.0x) |
| Fixed1K serialize again | 1.1 / 449.3 ns | 99% less |
| Fixed1K decode | 72.4 / 578.0 ns | 87% less (8.0x) |
| Mixed256 encode | 20.2 / 98.3 ns | 79% less (4.9x) |
| Mixed256 serialize again | 1.3 / 80.0 ns | 98% less |
| Mixed256 decode | 136.4 / 155.1 ns | 12% less (1.1x) |

Frame size, protaco / protobuf: Fixed512 556 / 571 B, Fixed1K 1,067 / 1,132 B, Mixed256 300 / 268 B.

Repeated runs moved each number by a few percent. Numbers differ between machines.

## Reading the results

- Fixed fields: protaco setters write straight into the frame buffer, so encoding is the setter calls and nothing more. protobuf encodes every field when it serializes.
- Serialize again: protaco reuses the last bytes when nothing changed (C++). protobuf encodes again.
- Decode with strings: each `std::string` longer than the small-string buffer (15 bytes in MSVC) allocates, because protaco decodes into a new object. protobuf reuses the strings of the message it parses into. The gap shrinks as variable fields grow.
- Size: protobuf varints shrink small numbers; protaco keeps fixed widths and a frame header with the message name.

## Running it

1. Build protobuf (3.21.12 was used) and `protacoc` (`build.cmd` in the repository root).
2. `protacoc -I schemas --cpp_out gen` and `protoc --cpp_out=gen bench.proto`.
3. Compile `bench.cpp` and `gen/bench.pb.cc` Release x64 with `gen` and the protobuf `src` folder on the include path, and link `libprotobuf`.
