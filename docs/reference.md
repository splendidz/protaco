[한국어](reference.ko.md) · [Back to README](../README.md)

# Protaco reference

Detailed rules for protacoc 0.5.0 and serialization format v3. See the [README](../README.md) for the workflow and generated API, and the [example guide](../examples/README.md) for runnable examples.

1. [Schema fingerprint](#1-schema-fingerprint)
2. [Command line](#2-command-line)
3. [Diagnostics](#3-diagnostics)
4. [Build and test](#4-build-and-test)

## 1. Schema fingerprint

A fingerprint is a 32-bit FNV-1a hash over the UTF-8 bytes of one version's canonical text. The offset basis is 2166136261 and the prime is 16777619.

```text
protaco-schema-1|<name>|<version>|<allow_extension 1 or 0>|<field>;<field>;...

fields  fields added by this version, in serialization order (added, then declaration)
field   <key>:<type>[!][=<default>]
        !          required
        =<default> the default as written in YAML, without quotes
        ~<key>:<type>   a fixed field removed by this version (its bytes stay)
        a variable field removed by this version is omitted
type    bool int int64 double string string(N) bytes list<T>
        a message: <Name>@<its fingerprint as 8 lowercase hex digits>
```

Example: `protaco-schema-1|Topping|1|1|name:string(24);grams:int=30;`.

- The `added` and `removed` values themselves are not part of the canonical text. Adding a version preserves older fingerprints.
- Nested messages are represented by the fingerprint after `@`. Changing an inner message also changes the fingerprints of messages containing it.
- C++ exposes `MsgSchemaHash()`; C# exposes the `MsgSchemaHash` constant. protacoc also writes the fingerprint into the class docstring.
- The frame carries one fingerprint for each version from `min_version` through `version`. A receiver compares the fingerprint for its own version.

See [Version management](../README.md#7-version-management) for usage rules.

## 2. Command line

```text
protacoc -I <dir> [-I <dir>...] [--cpp_out <dir>] [--cs_out <dir>]
         [--no_runtime] [--check] [Message ...]
```

At least one of `--cpp_out` and `--cs_out` is required.

| Option | Meaning |
|---|---|
| `-I <dir>`, `--proto_path <dir>` | Schema folder, repeatable; all input files must share a namespace |
| `--cpp_out <dir>` | C++ output folder |
| `--cs_out <dir>` | C# output folder |
| `--no_runtime` | Do not write `Protaco.hpp` or `Protaco.cs` |
| `--check` | Compare expected output without writing files; exit code 3 on differences |
| `Message ...` | Output only selected messages, by message name or YAML path |
| `-h`, `--help` | Show help |
| `--version` | Show the generator version |

Set namespaces in YAML; command-line namespace selection is not supported. Without message targets, all input messages are generated. Selecting targets still loads and validates all inputs, but does not automatically add referenced messages to the output selection.

Generate normally, then check without writing:

```bat
build\bin\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs
build\bin\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs --check
```

By default, a matching runtime is written beside the generated files. Projects using `--no_runtime` must supply the runtime from the same generator version separately.

## 3. Diagnostics

### 3.1 Error codes

protacoc prints `file(line,col): error CODE: message`. Visual Studio can open the corresponding YAML line from its Output pane.

YAML syntax:

- PC101: Unsupported YAML construct.
- PC102: Flow syntax error, including malformed braces, commas, quotes, and unsupported `[...]` flow lists.

Message keys and attributes:

- PC201: Unknown top-level key. This includes `owner`, removed in 0.5.0. `desc` is the only documentation key.
- PC202: Missing required key: `message`, `version`, `namespace`, or `fields`.
- PC203: Unknown field attribute. `eq`, removed in 0.5.0, takes a flow-list value (`[..]`), so it fails earlier with PC102.
- PC204: Invalid value.
- PC205: Duplicate attribute.
- PC206: Invalid namespace.

Fields and versions:

- PC301: Message name differs from the file name.
- PC302: Duplicate field key.
- PC303: Key conflicts with a reserved name, language keyword, or generated member, including `ext`, `buf`, `cache`, `serialize`, and `tostring`.
- PC304: Invalid type.
- PC305: `required` on a fixed field.
- PC308: Invalid default. Generation checks type, range, `+` and leading zeros on integers, the minimum `int64`, non-finite `double` values, and string ASCII and length restrictions.
- PC310: Invalid N in `string(N)`.
- PC311: C# property name collision.
- PC312: Invalid supported version range or out-of-range `min_version`, `added`, or `removed`.
- PC313: Variable field removed at or below `min_version`; delete its YAML line.
- PC314: Message name `v<N>`, reserved for version namespaces.

Messages and references:

- PC401: Duplicate message.
- PC402: Unknown message reference.
- PC403: Reference cycle, reported with its full path.
- PC404: Type not allowed as a list element.
- PC405: Different namespaces in one invocation.
- PC406: Nesting deeper than 8 levels.
- PC407: A message used as a field type uses version features (`min_version`, `added`, or `removed`).

Environment:

- PC501: File I/O.
- PC502: Command line.

### 3.2 Exit codes

| Code | Meaning |
|---|---|
| 0 | Success |
| 1 | Usage error |
| 2 | Schema errors |
| 3 | `--check` found stale or different files |

### 3.3 Decode errors

C++ reports the reason in `protaco::Error::what`; C# uses `ProtacoException.What`. Comparison errors include expected and received values. `path` or `Path` identifies a field such as `stops[2].orderId` and can be empty for header errors.

- `message name mismatch`: Different message name.
- `message version too old`: Frame older than the receiving class. Includes a type hint when that older class was generated.
- `message version too new`: Receiving class outside the sender's supported version range. Includes that range.
- `schema mismatch`: Different fingerprints, with both values shown.
- `fixed block size mismatch`: Different fixed-block sizes, with both shown.
- `missing required`: Required variable field absent.
- `type mismatch`: Different types, with both shown.
- `unknown key (allow_extension false)`: Disallowed key in a closed message.
- `bad version range`: Invalid supported version range.
- `nesting too deep`: Nesting limit exceeded.
- Truncation errors: Insufficient data in a header, body, or field.

A failure while reading a newer frame's body ends with `(frame version N read as M)`. See the [generated API](../README.md#6-generated-api) for throwing and non-throwing decoding.

## 4. Build and test

### 4.1 Running

From a command prompt at the repository root:

```bat
set GTEST_ROOT=<googletest folder>
build.cmd
```

The googletest folder needs `include\`, `lib\gtest.lib`, and `lib\gtest_main.lib`, with libraries built using `/MD`. Without `GTEST_ROOT`, both the C++ test build and execution are skipped.

Steps performed by `build.cmd`:

1. Build protacoc.
2. Regenerate `examples/gen` and `tests/gen`.
3. Build the C++ tests.
4. Run the C++ tests using golden files.
5. Build and run the C++ example.
6. Build and run the C# tests.

This command rewrites generated code. The C++ tests rewrite the reference frames in `tests/golden` only when `PROTACO_UPDATE_GOLDEN=1` is set, which is needed only when the serialization format changes. Set `MSBUILD` to override the MSBuild path; the default is Visual Studio 2017 Professional. For IDE use, open [protaco.sln](../protaco.sln).

### 4.2 Repository layout

| Path | Contents |
|---|---|
| [src/protacoc](../src/protacoc) | Generator |
| [runtime/cpp](../runtime/cpp), [runtime/cs](../runtime/cs) | Runtime sources copied by the generator |
| [examples](../examples/README.md) | Sample schemas, generated code, C++ program |
| [tests/schemas](../tests/schemas) | Version management, nesting, cycles, and diagnostic schemas |
| [tests/golden](../tests/golden) | Reference frames written by C++ and compared with C# |
| [tests/cpp](../tests/cpp) | googletest suite and C++14 translation unit |
| [tests/cs](../tests/cs) | Generator and C# runtime test runner |
| [benchmark](../benchmark/README.md) | Measurements against protobuf |
| [docs](.) | Reference documentation and figures |

The reference frames are `DeliveryRoute.bin`, `PizzaOrder.bin`, `Heartbeat.bin`, and `DeliveryProof.bin`.

### 4.3 Coverage

There are 32 C++ tests covering:

- Round trips, fixed fields written in place, and defaults.
- Rejection of version and fingerprint mismatches.
- Preservation of older fingerprints, reading newer frames as the receiver's version, decoding older frames with older classes, and a sender's `min_version` above the receiver.
- Unknown keys in closed messages and discarding newer fields.
- Nesting to 8 levels, missing required fields, nested error paths, and extension rules.
- `ToString()` output.
- Contract-check cost measurement. Library comparisons are in the [benchmark](../benchmark/README.md).
- Truncation at every length and random bit flips.
- Golden bytes and the C++14 translation unit.
- Serialize reuse: rebuilding after changes, `get_mutable_` references, copying, and ext assignment.
- Decoding a body with more than 16 variable fields.

There are 27 C# tests covering:

- Invalid YAML by error code, cycles, mixed namespaces, nesting depth, and restrictions on version features in field-type messages.
- `--check` against repository output and hand edits, plus input-order independence.
- Fingerprints against canonical text.
- Round trips, version management, closed messages, and truncation.
- Byte-for-byte equality with C++ golden files and matching `ToString()` output.
