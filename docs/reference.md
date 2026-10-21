# Protaco reference

[한국어](reference.ko.md) · [README](../README.md) · [Examples](../examples/README.md)

Detailed reference for protacoc 0.7.0 and serialization format v4. See the [README](../README.md) for schema authoring and the full serialization layout, and [examples](../examples/README.md) for runnable programs.

1. [Schema fingerprint](#1-schema-fingerprint)
2. [Command line](#2-command-line)
3. [Diagnostics](#3-diagnostics)
   - [Generator error codes](#31-generator-error-codes)
   - [Decode errors](#32-decode-errors)
   - [Receive checks](#33-receive-checks)
4. [C++ API](#4-c-api)
5. [C# API](#5-c-api)
6. [Python API](#6-python-api)
7. [Build and test](#7-build-and-test)

## 1. Schema fingerprint

A fingerprint is a 32-bit FNV-1a hash over the UTF-8 bytes of the canonical text for one message version. A frame carries fingerprints from `min_version` through `version` in order. The receiving class compares the fingerprint for its own version. The implementation is in [SchemaFingerprint.cs](../protaco_compiler/protacoc/SchemaFingerprint.cs).

**Canonical text**

```text
protaco-schema-1|<name>|<version>|<ext 0/1>|<field>;<field>;...
field = <key>:<type>[!][=<default>]
```

- `name`: message name. `version`: the version being hashed, in decimal
- `ext`: `1` when `allow_extension` is true, otherwise `0`. `auto_envelope` changes no bytes and is not part of the text
- Field order: ascending `added`, then YAML declaration order
- Included fields: those added by the version being hashed
- `!`: required field. `=<default>`: explicitly supplied default text, without outer string quotes
- Terminator: `;` after every field. No BOM, final newline, or extra whitespace

Do not regroup fixed and variable fields. Unlike the fixed region of the actual body, canonical text follows the field order above throughout. An omitted default also omits `=`.

A fixed field removed by that version remains as `~<key>:<type>;`. Its byte position stays reserved, and the required and default markers are omitted. A removed variable field is left out. The numbers in `added` and `removed` are not included, so adding a version preserves older fingerprints.

| Type | Canonical text |
|---|---|
| Primitive | `bool`, `int`, `int64`, `uint`, `uint64`, `double`, `string`, `string(N)`, `bytes` |
| List | `list<T>`, applying these rules recursively to T |
| Map | `map<K,V>`, no space after the comma; apply these rules to K and V |
| Other message | `Name@xxxxxxxx`, its fingerprint as eight lowercase hexadecimal digits |
| Inline fixed message | `fixed(Name@xxxxxxxx)` for a field or array element of a `fixed: true` message; inside lists and maps it stays `Name@xxxxxxxx` |
| Self reference | `Name@self` |
| Enum | `enum(Name)`, excluding its value list |
| Fixed-length array | `T[N]`: T by these rules, N the resolved element count (`int[16]`, `enum(Level)[3]`) |

Namespaces, comments, and `desc` are excluded. A change to a referenced message's fingerprint also affects its containing messages. Adding enum values does not change the fingerprint.

**Calculation**

The input `canonical` is the UTF-8 string built by the rules above. The return value is an unsigned 32-bit fingerprint. Each multiplication is modulo 2³².

**C++**

```cpp
#include <cstdint>
#include <string>

uint32_t Fingerprint(const std::string& canonical)
{
    uint32_t hash = 2166136261u;
    for (unsigned char byte : canonical)
        hash = static_cast<uint32_t>((hash ^ byte) * uint64_t{16777619u});
    return hash;
}
```

<details>
<summary>C#</summary>

```csharp
static uint Fingerprint(string canonical)
{
    uint hash = 2166136261u;
    foreach (byte value in System.Text.Encoding.UTF8.GetBytes(canonical))
        hash = unchecked((hash ^ value) * 16777619u);
    return hash;
}
```

</details>

<details>
<summary>Python</summary>

```python
def fingerprint(canonical):
    value = 2166136261
    for byte in canonical.encode("utf-8"):
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value
```

</details>

Each input below is a single line; the newline is not hashed. These values match `MsgSchemaHash` in the generated code.

```text
protaco-schema-1|Topping|1|1|name:string(24);grams:int=30;
protaco-schema-1|Heartbeat|1|0|node:string(32);seq:int64;healthy:bool=true;cpuLoad:double;uptimeSec:int;
protaco-schema-1|Order|3|1|id:string(8);~qty:int;color:string!;weight:double=0.5;tags:list<string>;label:string;rush:bool;
```

| Input | Fingerprint |
|---|---|
| Topping version 1 | `0xEB8A11D8` |
| Heartbeat version 1 | `0x2BB85C55` |
| Order version 3 | `0x7073B2E7` |

## 2. Command line

Specify at least one input folder and one output language. Namespaces come from YAML; all definitions in one invocation must share a namespace.

```text
protacoc -I <yaml dir> [-I ...] [--cpp_out <dir>] [--cs_out <dir>] [--py_out <dir>] [options] [message...]
```

| Option | Meaning |
|---|---|
| `-I <dir>`, `--proto_path <dir>` | Schema folder; repeatable |
| `--cpp_out <dir>` | C++ output folder |
| `--cs_out <dir>` | C# output folder |
| `--py_out <dir>` | Python output root |
| `--no_runtime` | Skip runtime output |
| `--check` | Compare expected bytes without writing |
| `name...` | Message, enum, or const names, or YAML paths; all definitions if omitted |
| `-h`, `--help` | Help |
| `--version` | Generator version |

**Output files**

| Language | Output paths |
|---|---|
| C++ | `<cpp_out>/<Name>.g.h`, `<Enum>.enum.h`, `<Const>.const.h`, `Protaco.hpp` |
| C# | `<cs_out>/<Name>.g.cs`, `<Enum>.enum.cs`, `<Const>.const.cs`, `Protaco.cs` |
| Python | `<py_out>/<namespace folders>/<Name>_g.py`, `<Enum>_enum.py`, `<Const>_const.py`; runtime at `<py_out>/protaco.py` |

Each input folder supplies its `*.yaml` files, subfolders excluded. A file whose name contains `.bak` (any case) is skipped and reported as `protacoc: skipped <path> (.bak in the file name)`; no valid file matches, because a definition name has no `.`.

Selecting targets still loads and validates every input. Referenced definitions are not added to the output selection automatically; select the messages and enums you need together. A YAML path also selects by the name of a definition loaded from the input folders. With `--no_runtime`, supply a runtime from the same generator version separately.

**Windows command prompt**

```bat
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\taco_generated\cpp --cs_out examples\taco_generated\cs --py_out examples\taco_generated\py
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\taco_generated\cpp --cs_out examples\taco_generated\cs --py_out examples\taco_generated\py --check
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\taco_generated\cpp Menu Category PizzaSize
```

**Exit codes and byte rules**

| Code | Meaning |
|---|---|
| 0 | Success, or all `--check` outputs match |
| 1 | Command-line syntax error |
| 2 | Schema, I/O, or target-selection error |
| 3 | Missing file or byte mismatch under `--check` |

`--check` compares selected outputs, including the runtime by default, without writing files. Differences are reported as `path: error PC501: out of date (run protacoc again)`. PC502 for a nonexistent target occurs during target selection, so its exit code is 2.

Generated files use UTF-8 with a BOM and CRLF on every OS. The source sha1 in the first generated comment is computed from the YAML's UTF-8 text after removing the BOM and converting CRLF to LF. Checkout line endings do not affect generated bytes.

## 3. Diagnostics

### 3.1 Generator error codes

Errors with a source position are written to standard error as `file(line,column): error PC404: message`. Errors without a position use `protacoc: error PC502: message`. See [Diag.cs](../protaco_compiler/protacoc/Diag.cs) for the format and code definitions.

| Code | Meaning | Typical cause or example |
|---|---|---|
| PC101 | Unsupported YAML construct | Syntax outside the supported YAML subset |
| PC102 | Flow syntax error | Braces, quotes, or colons; `[a, b]`; unquoted `map<K, V>` |
| PC201 | Unknown top-level key | `owner` |
| PC202 | Missing required key or value | Missing `message/version/namespace/fields`; no enum values in the current version |
| PC203 | Unknown field attribute | Undefined attribute such as `eq` |
| PC204 | Invalid value format | Invalid bool, int, or name |
| PC205 | Duplicate attribute | Two `key` attributes in one item |
| PC206 | Invalid namespace | Invalid name or reserved segment |
| PC208 | Definition kind and suffix differ | Enum or const outside `<Name>.enum.yaml` or `<Name>.const.yaml`, or that file lacks `enum:` or `const:` |
| PC301 | Definition name and file name differ | A different name in `Menu.yaml` |
| PC302 | Duplicate field, enum, or const key | Repeated `key` |
| PC303 | Reserved name or naming violation | Generated member or reserved prefix; key over 255 bytes |
| PC304 | Malformed type | `map<string>`, a const name as a type |
| PC305 | Required fixed field | `required` on `int`, `string(N)`, `T[N]`, etc. |
| PC306 | Required self reference | `required` on Self or `list<Self>` |
| PC308 | Invalid default | Range or string restriction; unknown enum value name |
| PC310 | Invalid fixed length | N in `string(N)` outside 1..4096, `T[N]` with N below 1 or a fixed block over 0x7FFF0000 bytes |
| PC311 | Generated property name collision | C# capitalization or Python keyword suffix `_` |
| PC312 | Invalid version history | Range or ordering of `min_version`, `added`, `removed` |
| PC313 | Obsolete variable field line retained | `removed <= min_version`; delete the line |
| PC314 | Reserved version name | Message or enum named `v<N>` |
| PC318 | Reused enum number | Duplicate number, including removed values |
| PC401 | Duplicate definition name | Message or enum name collision |
| PC402 | Unknown type reference | Type absent from loaded definitions |
| PC403 | Cycle or unsupported self reference | Two messages referring to each other; Self inside a map |
| PC404 | Type forbidden in a container | `list<bool>`, `map<double, int>`, `string(N)` map value, `string[4]`, `int[2][3]`, `list<int[2]>` |
| PC405 | Mixed namespaces | Different namespaces in one invocation |
| PC406 | Generator nesting limit exceeded | More than 100 levels, excluding self references |
| PC407 | Version features on a nested message | `min_version/added/removed` on a message used as a field type |
| PC408 | Rule of a `fixed: true` message | A variable field, versions, `allow_extension: true`, or a field of its own type |
| PC501 | File I/O | Missing input folder, write failure, or `--check` mismatch |
| PC502 | Command line or target error | Missing output language, unknown option or target |

### 3.2 Decode errors

An error string is `path: reason`, or just the reason when the path is empty. Common decode checks use the same wording in all three languages. In the tables, `N`, `R`, `S`, `A`, and `B` are placeholders for actual values.

- Field path: `delivery.doorCode`
- List path: zero-based element index, such as `sections[0].name`
- Map path: serialized entry index, not the key value, such as `prices[1]`
- Body path: empty for the top-level body; the containing message's path for a nested body
- Newer message version body error: suffix ` (frame version S read as R)`. S and R are message versions, separate from serialization format 4

C++ exposes separate fields through `CParseError::GetError()` and `Error{path, what}`; C# uses `ProtacoException.Path/What`; Python uses `ProtacoError.path/what`. C++ `what()`, C# `Message`, and Python `str(error)` return the combined string. Try APIs provide the same string through an output argument or Result.

**Frame checks**

| Message | Cause | Path |
|---|---|---|
| `frame too large` | Frame size limit exceeded | Empty |
| `frame too short` | Incomplete 28-byte base header, name, or fingerprint array | Empty |
| `bad magic` | First two bytes are not `PC` | Empty |
| `unknown frame version N, expected 4` | Serialization format mismatch | Empty |
| `bad version range A..B` | Minimum zero, reversed range, or more than 255 versions | Empty |
| `message name mismatch: expected A, got B` | Message name differs from the receiving class | Empty |
| `message version too old: N reads version R or newer, got S` | S < R; adds `. Use the vS type` when an old type is supported | Empty |
| `message version too new: N reads version R, sender supports A..S` | Sender's min_version > R | Empty |
| `schema mismatch: N version R expected 0xAAAAAAAA, got 0xBBBBBBBB` | Fingerprint for R differs | Empty |
| `frame truncated` | Actual body shorter than BodyLen | Empty |
| `trailing bytes after frame` | Extra bytes after BodyLen | Empty |

**Body checks**

| Message | Cause | Path |
|---|---|---|
| `nesting too deep` | Nesting exceeds 100 levels | Nested message |
| `fixed block truncated` | Incomplete FixedLen or fixed region | Body |
| `fixed block size mismatch: expected A, got B` | Different size at the same version, or insufficient size at a newer version | Body |
| `field key truncated` | Zero TLV key length or truncated key | Body |
| `field value truncated` | Truncated TLV type, length, or value | Key |
| `duplicate key` | Repeated TLV key | Key |
| `too many fields` | More than 1,024 variable fields and ext entries in one body | Body |
| `missing required` | Required variable field absent | Key |
| `type mismatch: expected string, got int` | TLV type differs from schema; actual type names are shown | Key |
| `unknown key (allow_extension false)` | Unknown key in a closed message at the same version | Key |
| `scalar size mismatch` | Incorrect scalar byte length | List element or map value |
| `list header truncated` | Incomplete element type or count | List field |
| `list element type mismatch` | Incorrect list element type | List field |
| `list count too large` | Count exceeds remaining bytes / 4 | List field |
| `list element truncated` | Incomplete element length or value | `field[i]` |
| `trailing bytes in list` | Extra bytes after the declared elements | List field |
| `map header truncated` | Incomplete key type, value type, or count | Map field |
| `map key or value type mismatch` | Incorrect key or value type | Map field |
| `map count too large` | Count exceeds remaining bytes / 8 | Map field |
| `map key truncated or bad` | Incomplete key length or value, or key decode failure | `field[i]` |
| `map value truncated` | Incomplete value length or value | `field[i]` |
| `duplicate map key` | Repeated map key | `field[i]` |
| `trailing bytes in map` | Extra bytes after the declared entries | Map field |
| `out of memory` | Allocation failure during C++ decoding | Depends on failure location |

**Extension errors**

These are error strings from ext calls. Keys appear inside the message rather than in a decode path prefix.

| Message | Cause | Applies to |
|---|---|---|
| `ext key is empty` | Empty key | All languages |
| `ext key is longer than 255 bytes: prefix...` | UTF-8 key exceeds 255 bytes; a prefix is shown | All languages |
| `ext key starts with '_' (reserved): _x` | Reserved prefix | All languages |
| `ext key is a field of the message: rider` | Schema key in any supported version | All languages |
| `ext key not found: missing` | Missing key | All languages |
| `ext key channel holds string, not int` | Stored and requested types differ | All languages |
| `ext key k has a bad int value` | Incorrect stored scalar length | All languages |
| `unsupported ext value type T` | Unsupported value type | C# and Python; compile error in C++ |
| `ext value 5 needs a kind: INT, INT64, UINT or UINT64` | Integer without a kind | Python |
| `unsupported ext value kind K` | Unsupported kind | Python |
| `ext is off for this message (allow_extension false)` | Writing ext on a closed message | Python; no public accessor in C++ or C# |

**Time conversion errors**

| Message | Cause | Applies to |
|---|---|---|
| `timestamp N is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)` | Outside the supported time range | All languages; no path |
| `timestamp N has no local time` | Platform local-time conversion failed | C++ and Python; no path |

### 3.3 Receive checks

Let R be the receiving class's message version and S the frame's message version. `ToObject`, `TryToObject`, and fixed-field `TryView` check in this order:

1. Frame size limit.
2. Minimum header length, magic, serialization format, name and fingerprint areas, and version range.
3. Message name.
4. Whether S < R. Receive an older frame with the generated class for its version.
5. Whether the sender's min_version > R.
6. The frame's fingerprint for R against the receiving class's fingerprint.
7. BodyLen against the actual body length, detecting truncation and trailing bytes.
8. Fixed region, TLVs, required fields, nesting, and container rules. When S > R, body errors gain a version suffix.

Open messages retain unknown fields in ext. Closed messages reject unknown keys at the same version and discard unknown fields received from a newer version. `PeekHeader` only reads the header and does not replace these checks.

## 4. C++ API

The runtime is [Protaco.hpp](../include/cpp/Protaco.hpp), requiring only C++14 and the standard library. Generated headers are `<Name>.g.h`; classes are `<namespace>::<Name>`. Signatures below show public declarations with bodies omitted. `T` and `<k>` denote a field's type and name. Examples run inside a function and require the corresponding generated headers.

Try APIs write failure reasons to the optional `std::string* error = nullptr` argument. Check the return value for success; do not assume success clears an existing error string. Output pointers such as `out` must point to valid writable objects.

### 4.1 Generated classes

**Constructor and fixed fields**

**C++**

```cpp
Name();
T get_<k>() const;
void set_<k>(T value);

std::string get_<k>() const; // string(N)
bool set_<k>(const std::string& value, std::string* error = nullptr);
```

The constructor initializes schema defaults and stamps the envelope with a new id and the current time. Objects created internally by the decoder or as generated nested fields do not consume ids.

- Parameters: `value` is the new field value; `error` receives a fixed-string error
- Returns: getters return values; numeric, bool, and enum setters return void; fixed-string setters return success
- Fixed-string failure: false if the UTF-8 byte count exceeds N; the old value stays. Example: `orderId: 17 bytes, longer than string(16)`
- Layout: bool uses 1 byte; int, uint, and enum use 4; int64, uint64, and double use 8; `string(N)` uses N
- Notes: fixed-field setters write directly into the object's frame buffer. N is 1..4096; unused string bytes are zero

**C++**

```cpp
demo::RouteStop stop;
std::string error;
bool fits = stop.set_orderId("A-100", &error);
stop.set_etaMin(10);
int32_t minutes = stop.get_etaMin();
```

<details>
<summary>C#</summary>

```csharp
var stop = new demo.RouteStop { OrderId = "A-100", EtaMin = 10 };
int minutes = stop.EtaMin; // OrderId overflow throws ArgumentException.
```

</details>

<details>
<summary>Python</summary>

```python
from demo.RouteStop_g import RouteStop

stop = RouteStop(orderId="A-100", etaMin=10)
minutes = stop.etaMin
data = stop.Serialize()  # Field ranges are checked here.
```

</details>

**Fixed-length arrays**

**C++**

```cpp
static constexpr size_t <k>_size();
T get_<k>(size_t index) const;
std::array<T, N> get_<k>() const;
bool set_<k>(size_t index, T value, std::string* error = nullptr);
void set_<k>(const std::array<T, N>& values);

bool set_<k>(size_t index, const std::string& value, std::string* error = nullptr); // string(N) elements
bool set_<k>(const std::array<std::string, N>& values, std::string* error = nullptr);
```

Arrays live in the fixed block like other fixed fields. The element at `index` is read and written in place.

- Parameters: `index` is 0..N-1; `values` is the whole array; `error` receives the reason of a failed set
- Returns: `get_<k>(index)` returns the element, `get_<k>()` a copy of the array. Element setters return success; the whole-array setter of `string(N)` returns success and writes nothing on failure
- Range: `get_<k>(index)` asserts the index in debug builds and does not check it otherwise. `set_<k>(index, ...)` returns false: `counts[4]: index out of range (size 4)`
- `string(N)` element: false if longer than N bytes: `names[1]: 9 bytes, longer than string(8)`
- View: `<k>_size()`, `get_<k>(index)`, `get_<k>()`

C# arrays are get-only `T[]` properties of length N; `Serialize()` throws `ProtacoException` for a `string(N)` element that is too long, and `View` has `Name(index)` (ArgumentOutOfRangeException outside 0..N-1) and `Name()`. Python arrays are lists of N values checked by `Serialize()`.

**Inline fixed messages**

**C++**

```cpp
M::ConstRef get_<k>() const;                       // a field of a fixed: true message M
M::Ref mutable_<k>();
void set_<k>(const M& value);

M::ConstRef get_<k>(size_t index) const;           // M[N]
M::Ref mutable_<k>(size_t index);
bool set_<k>(size_t index, const M& value, std::string* error = nullptr);

// M::ConstRef: the getters of M, M Copy() const, std::string ToString(style) const
// M::Ref: ConstRef plus the setters of M, void Assign(const M& value)
```

A `fixed: true` message used as a field type, or as the element of `T[N]`, sits inline in the fixed block. `ConstRef` and `Ref` point at those bytes inside this message.

- Parameters: `index` is 0..N-1; `value` is an `M` object whose fields are copied in
- Returns: `ConstRef` and `Ref` by value (a pointer inside this message's buffer); the element setter returns success
- Lifetime: a `ConstRef` or `Ref` is valid while the message lives and its buffer is not replaced (decoding into the message, assignment, moving)
- Range: `get_<k>(index)` and `mutable_<k>(index)` assert the index in debug builds; `set_<k>(index, ...)` returns false: `slots[4]: index out of range (size 4)`
- Defaults: a new message copies each inline message's own defaults in
- View: `get_<k>()` and `get_<k>(index)` return `M::ConstRef`

C# fields are `M` objects and arrays get-only `M[]` arrays of N objects; `Serialize()` throws `ProtacoException` (`slots[1]: null fixed message`) for a null element, and `View` returns the struct `M.ConstRef`. Python fields are `M` objects and arrays lists of N objects.

**Variable fields**

**C++**

```cpp
const T& get_<k>() const;
T& mutable_<k>(); // bytes, list, map, message
void set_<k>(const T& value);
```

There is no `mutable_<k>()` for `string`. Bytes use `std::vector<uint8_t>`, lists use `std::vector<T>`, maps use `std::unordered_map<K,V>`, and nested messages use their generated classes.

- Parameters: `value` is the new value to copy
- Returns: getters return const references, mutable accessors return writable references, setters return void
- Lifetime: references depend on the message and its field storage. Container element reference invalidation rules also apply
- Notes: after mutable access, changes cannot be tracked, so every Serialize rebuilds the variable block

**C++**

```cpp
demo::DeliveryRoute route;
route.set_rider("rider-7");
demo::RouteStop stop;
stop.set_orderId("A-100");
stop.set_address("1 Pizza St");
route.mutable_stops().push_back(stop);
route.set_note("closest-first");
const auto& stops = route.get_stops();

demo::Menu menu;
menu.mutable_prices()["margherita"] = 9.5;
```

<details>
<summary>C#</summary>

```csharp
var route = new demo.DeliveryRoute { Rider = "rider-7", Note = "closest-first" };
route.Stops.Add(new demo.RouteStop { OrderId = "A-100", Address = "1 Pizza St" });
var menu = new demo.Menu();
menu.Prices["margherita"] = 9.5;
```

</details>

<details>
<summary>Python</summary>

```python
from demo.DeliveryRoute_g import DeliveryRoute
from demo.RouteStop_g import RouteStop
from demo.Menu_g import Menu

route = DeliveryRoute(rider="rider-7", note="closest-first")
route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St"))
menu = Menu()
menu.prices["margherita"] = 9.5
```

</details>

Maps serialize in container iteration order. Identical contents can produce a different byte order across languages or implementations. Decoding restores the contents and rejects duplicate keys.

**Optional self-reference fields**

These signatures are from the single `next` field in the [test Node](../protaco_compiler/tests/taco_generated/cpp/recursive/Node.g.h). The example uses it as a linked node of menu categories.

**C++**

```cpp
bool has_next() const;
const Node& get_next() const;
Node& mutable_next();
void set_next(const Node& value);
void clear_next();
```

- Parameters: `value` is the Node to deep-copy
- Returns: has reports presence; get returns a reference; mutable returns a reference and creates the node if absent
- Initial state: absent. get returns an empty Node; serialization omits the field
- clear: removes the value and restores the absent state
- Notes: self references may only be Self or `list<Self>`. Required self references, Self inside maps, mutual cycles, and version history features are disallowed

**C++**

```cpp
recursive::Node section;
section.set_name("pizza");
section.mutable_next().set_name("white");
bool present = section.has_next();
const auto& next = section.get_next();
recursive::Node copy = section;
copy.clear_next(); // section keeps its own next.
```

<details>
<summary>C#</summary>

```csharp
var section = new recursive.Node { Name = "pizza" };
section.Next = new recursive.Node { Name = "white" };
bool present = section.Next != null;
section.Next = null;
```

</details>

<details>
<summary>Python</summary>

```python
from recursive.Node_g import Node

section = Node(name="pizza")
section.next = Node(name="white")
present = section.next is not None
section.next = None
```

</details>

**Extension access**

**C++**

```cpp
const protaco::CExt& GetExt() const;
protaco::CExt& GetMutableExt();
```

Only open messages generate these accessors. They take no parameters and return references to the message's ext storage. References depend on the message's lifetime. See [4.4 Extension data](#44-extension-data) for key rules and examples.

**Serialize and ToString**

**C++**

```cpp
const std::vector<uint8_t>& Serialize() const;
std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const;
```

Serialize returns a reference to the complete frame, including its envelope. Use the returned data until the object is modified or destroyed. Copy it to a separate vector if the object must change during an asynchronous send.

- Parameters: none for Serialize; ToString's `style` is the configuration in [4.5 Text](#45-text)
- Returns: Serialize returns the internal buffer by reference; ToString returns an independent string
- Fixed fields only and empty ext: returns the buffer already written by setters
- Unchanged variable fields and ext: reuses the previous variable block
- After a variable setter or ext change: rebuilds the variable block
- After any mutable accessor call: rebuilds the variable block on every subsequent Serialize

**C++**

```cpp
demo::DeliveryRoute route;
route.set_rider("rider-7");
const auto& borrowed = route.Serialize();
auto received = protaco::ToObject<demo::DeliveryRoute>(borrowed);
std::vector<uint8_t> owned = route.Serialize();
std::string log = received.ToString();
```

<details>
<summary>C#</summary>

```csharp
var route = new demo.DeliveryRoute { Rider = "rider-7" };
byte[] owned = route.Serialize(); // New array on each call.
var received = Protaco.Codec.ToObject<demo.DeliveryRoute>(owned);
string log = received.ToString();
```

</details>

<details>
<summary>Python</summary>

```python
route = DeliveryRoute(rider="rider-7")
owned = route.Serialize()  # bytes
received = protaco.ToObject(DeliveryRoute, owned)
log = received.ToString()
```

</details>

**Metadata and older version classes**

**C++**

```cpp
static const char* MsgName();
static uint16_t MsgVersion();
static uint16_t MsgMinVersion();
static uint32_t MsgSchemaHash();
```

These take no parameters and return the class's name, version, minimum supported version, and fingerprint for its version. The name points to a static string. Older classes are generated in the same header as `<ns>::v<N>::<Name>`, exposing that version's fields. The current class does not automatically upgrade old frames.

**C++**

```cpp
const char* name = demo::DeliveryRoute::MsgName();
uint16_t version = demo::DeliveryRoute::MsgVersion();
uint16_t minimum = demo::DeliveryRoute::MsgMinVersion();
uint32_t hash = demo::DeliveryRoute::MsgSchemaHash();

build3::v1::Order oldOrder; // tests/taco_generated/cpp/build3/Order.g.h
auto restored = protaco::ToObject<build3::v1::Order>(oldOrder.Serialize());
```

<details>
<summary>C#</summary>

```csharp
uint hash = demo.DeliveryRoute.MsgSchemaHash;
var oldOrder = new build3.v1.Order();
var restored = Protaco.Codec.ToObject<build3.v1.Order>(oldOrder.Serialize());
```

</details>

<details>
<summary>Python</summary>

```python
from build3.Order_g import v1

schema_hash = DeliveryRoute.MsgSchemaHash
old_order = v1.Order()
restored = protaco.ToObject(v1.Order, old_order.Serialize())
```

</details>

**Common base IMessage**

**C++**

```cpp
class IMessage
{
public:
    virtual ~IMessage();
    virtual const char* Name() const;        // MsgName()
    virtual uint16_t Version() const;        // MsgVersion()
    virtual uint32_t SchemaHash() const;     // MsgSchemaHash()
    virtual protaco::Envelope GetEnvelope() const;
    virtual void SetEnvelope(const protaco::Envelope& env);
    virtual const std::vector<uint8_t>& Serialize() const;
    virtual std::string ToString(const protaco::TextStyle& style) const;
    std::string ToString() const;
    template <class T> T* As();
    template <class T> const T* As() const;
};
```

Every generated class derives from `protaco::IMessage` through `CMessage<T>`. Code that does not know the type handles any message through `IMessage&`, `IMessage*`, or `std::unique_ptr<IMessage>`: name and version, envelope, serialization, and text. `As<T>()` gives the object back as the generated type.

- Returns: `Name`, `Version`, and `SchemaHash` are the static info of the generated type; the other functions behave as the generated functions of the same name
- `As<T>`: nullptr when the object is not a T. The check is the C++ type (`dynamic_cast`, needs RTTI): an older version type `v1::T` and a same-named type of another namespace are other types
- Ownership: deleting through `std::unique_ptr<IMessage>` is safe (virtual destructor). Copy and move the generated type, not IMessage
- Cost: each generated object holds one virtual table pointer. Calls on the generated type are unchanged
- C# classes share the base class `Protaco.Message`; Python needs no base

**C++**

```cpp
std::vector<std::unique_ptr<protaco::IMessage>> outbox;
outbox.push_back(std::make_unique<demo::DeliveryRoute>(route));
outbox.push_back(std::make_unique<demo::Heartbeat>());
for (const std::unique_ptr<protaco::IMessage>& message : outbox)
    std::printf("%s v%u %zu bytes: %s\n", message->Name(), unsigned(message->Version()), message->Serialize().size(), message->ToString().c_str());
demo::Heartbeat* beat = outbox[1]->As<demo::Heartbeat>();
if (beat != nullptr)
    beat->SetTimestampAsNow();
```

**View**

A version containing only fixed fields generates `class View : public protaco::CView`. It exposes `get_<k>() const` for each field and `GetEnvelope/GetId/GetCorr/GetTimestamp`. Field getters return values; fixed-string getters construct `std::string`. The cost of returning a string is separate from TryView's zero-copy operation. See TryView in [4.3 Decoding](#43-decoding) for creation, lifetime, and examples.

### 4.2 Envelope

**Envelope and message accessors**

**C++**

```cpp
struct Envelope {
    uint32_t id = 0;
    uint32_t corr = 0;
    int64_t ts = 0;
};

uint32_t GetId() const;
void SetId(uint32_t id);
uint32_t GetCorr() const;
void SetCorr(uint32_t corr);
int64_t GetTimestamp() const;
void SetTimestamp(int64_t timestampUs);
void SetTimestampAsNow();
protaco::Envelope GetEnvelope() const;
void SetEnvelope(const protaco::Envelope& env);
```

`id` is the message id, `corr` identifies the request this reply answers, and `ts` is UTC microseconds since 1970-01-01. Construction sets id and ts automatically; corr starts at 0 and is set by the caller. A message with `auto_envelope: false`, or any message after `DisableAutoEnvelope()`, starts with id and ts at 0 as well.

- Parameters: each setter's new value, or the complete `env`
- Returns: getters return copies; setters return void
- Storage: the frame header. SetTimestampAsNow writes the current NowUs value
- Notes: serialization and decoding do not issue a new id; decoding preserves the received envelope

**C++**

```cpp
demo::DeliveryRoute request;
demo::DeliveryRoute reply;
reply.SetCorr(request.GetId());
reply.SetTimestampAsNow();
protaco::Envelope saved = reply.GetEnvelope();
reply.SetEnvelope(saved);
```

<details>
<summary>C#</summary>

```csharp
var request = new demo.DeliveryRoute();
var reply = new demo.DeliveryRoute();
reply.SetCorr(request.GetId());
reply.SetTimestampAsNow();
Protaco.Envelope saved = reply.GetEnvelope();
reply.SetEnvelope(saved);
```

</details>

<details>
<summary>Python</summary>

```python
request = DeliveryRoute()
reply = DeliveryRoute()
reply.SetCorr(request.GetId())
reply.SetTimestampAsNow()
saved = reply.GetEnvelope()
reply.SetEnvelope(saved)
```

</details>

**Ids and clock**

**C++**

```cpp
uint32_t NextId();
int64_t NowUs();
void SetClock(int64_t (*nowUs)());
```

- Parameters: `nowUs` is a function returning UTC microseconds; nullptr restores the system clock
- Returns: NextId returns a module-wide id starting at 1; NowUs returns the current clock value; SetClock returns void
- Notes: id generation is thread-safe and skips 0 on wraparound. SetClock is not synchronized; set it before creating messages

**C++**

```cpp
protaco::SetClock(+[]() -> int64_t { return protaco::kTimestampMin; });
demo::DeliveryRoute request;
int64_t now = protaco::NowUs();
uint32_t anotherId = protaco::NextId();
protaco::SetClock(nullptr);
```

<details>
<summary>C#</summary>

```csharp
Protaco.Envelopes.SetClock(() => Protaco.Envelopes.TimestampMin);
var request = new demo.DeliveryRoute();
long now = Protaco.Envelopes.NowUs();
uint anotherId = Protaco.Envelopes.NextId();
Protaco.Envelopes.SetClock(null);
```

</details>

<details>
<summary>Python</summary>

```python
protaco.SetClock(lambda: protaco.TIMESTAMP_MIN)
request = DeliveryRoute()
now = protaco.NowUs()
another_id = protaco.NextId()
protaco.SetClock(None)
```

</details>

**Turning off the automatic envelope**

**C++**

```cpp
void DisableAutoEnvelope();
void EnableAutoEnvelope();
```

- Effect: after DisableAutoEnvelope, new message objects keep id 0 and ts 0 and take no id from NextId. EnableAutoEnvelope restores the default
- Scope: every message of the module (DLL or executable). For one message only, set `auto_envelope: false` in its YAML header
- Returns: void
- Notes: thread-safe. Objects created earlier keep their envelope. Decoding and nested fields are not affected

**C++**

```cpp
protaco::DisableAutoEnvelope();           // at program start
demo::DeliveryRoute request;              // id 0, ts 0
request.SetId(protaco::NextId());
request.SetTimestampAsNow();
```

<details>
<summary>C#</summary>

```csharp
Protaco.Envelopes.DisableAutoEnvelope();
var request = new demo.DeliveryRoute();
request.SetId(Protaco.Envelopes.NextId());
request.SetTimestampAsNow();
```

</details>

<details>
<summary>Python</summary>

```python
protaco.DisableAutoEnvelope()
request = DeliveryRoute()
request.SetId(protaco.NextId())
request.SetTimestampAsNow()
```

</details>

**Time conversion**

**C++**

```cpp
enum : int64_t {
    kTimestampMin = 946684800000000LL,
    kTimestampMax = 253402300799999999LL
};

bool TryGetTime(int64_t timestampUs, std::string* date, std::string* time,
                std::string* error = nullptr);
bool TryGetTime(int64_t timestampUs, int* year, int* month, int* day,
                int* hour, int* minute, int* second,
                int* millisecond, int* microsecond, std::string* error = nullptr);

// Message members:
bool TryGetTimeFromTimestamp(std::string* date, std::string* time,
                             std::string* error = nullptr) const;
bool TryGetTimeFromTimestamp(int* year, int* month, int* day,
                             int* hour, int* minute, int* second,
                             int* millisecond, int* microsecond,
                             std::string* error = nullptr) const;
```

Converts UTC microseconds to the execution environment's local time. Message members use GetTimestamp as input.

- Parameters: `timestampUs` or the object's ts, output pointers, and an optional error string
- Returns: true on success. Strings use `YYYY-MM-DD` and `hh:mm:ss.ffffff`
- Integer outputs: year, month, day, hour, minute, second, millisecond, and remaining microsecond. Fractional microseconds = millisecond × 1000 + microsecond
- Range: 2000-01-01 00:00:00 UTC through 9999-12-31 23:59:59.999999 UTC
- Failure: false for an out-of-range value or failed local conversion. See [time conversion errors](#32-decode-errors)

**C++**

```cpp
demo::DeliveryRoute reply;
std::string date, time, error;
bool ok = reply.TryGetTimeFromTimestamp(&date, &time, &error);
int year, month, day, hour, minute, second, millisecond, microsecond;
bool partsOk = protaco::TryGetTime(reply.GetTimestamp(), &year, &month, &day,
    &hour, &minute, &second, &millisecond, &microsecond, &error);
```

<details>
<summary>C#</summary>

```csharp
var reply = new demo.DeliveryRoute();
bool ok = reply.TryGetTimeFromTimestamp(out string date, out string time, out string error);
bool partsOk = Protaco.Envelopes.TryGetTime(reply.GetTimestamp(),
    out int year, out int month, out int day, out int hour, out int minute,
    out int second, out int millisecond, out int microsecond, out error);
```

</details>

<details>
<summary>Python</summary>

```python
reply = DeliveryRoute()
result = reply.TryGetTimeFromTimestamp()
if result:
    date, time = result.value
parts = protaco.TryGetTimeParts(reply.GetTimestamp())
if parts:
    year, month, day, hour, minute, second, millisecond, microsecond = parts.value
```

</details>

### 4.3 Decoding

**TryToObject**

**C++**

```cpp
template<class T>
bool TryToObject(const uint8_t* src, size_t size, T* out,
                 std::string* error = nullptr);
template<class T>
bool TryToObject(const std::vector<uint8_t>& bytes, T* out,
                 std::string* error = nullptr);
template<class T>
bool TryToObject(std::vector<uint8_t>&& bytes, T* out,
                 std::string* error = nullptr);
```

- Parameters: `src/size` or `bytes` is the complete frame; `out` is an object of generated class T; `error` is optional
- Returns: true and a decoded out on success. On failure, false with out unchanged
- Errors: decode failures are reported by the return value. See [3.2](#32-decode-errors) for string format
- Input lifetime: ordinary overloads require the input only during the call. out's own Serialize buffer is also valid input

The rvalue vector overload adopts the frame buffer when its message version, min_version, and every fingerprint match T's layout. The successful object owns the original buffer, and Serialize reuses it until variable fields or ext change. Other accepted layouts use ordinary copying. This optimization concerns the frame buffer; variable field objects still need decoding. Do not assume the moved input vector is preserved on failure.

**C++**

```cpp
std::vector<uint8_t> bytes = demo::DeliveryRoute().Serialize();
demo::DeliveryRoute received;
std::string error;
bool ok = protaco::TryToObject(bytes, &received, &error);
bool adopted = protaco::TryToObject(std::move(bytes), &received, &error);
```

<details>
<summary>C#</summary>

```csharp
byte[] bytes = new demo.DeliveryRoute().Serialize();
bool ok = Protaco.Codec.TryToObject<demo.DeliveryRoute>(
    bytes, out var received, out string error);
```

Rvalue buffer ownership transfer: not applicable.

</details>

<details>
<summary>Python</summary>

```python
data = DeliveryRoute().Serialize()
result = protaco.TryToObject(DeliveryRoute, data)
if result:
    received = result.value
```

Rvalue buffer ownership transfer: not applicable.

</details>

**ToObject, CParseError, Error**

**C++**

```cpp
template<class T> T ToObject(const uint8_t* src, size_t size);
template<class T> T ToObject(const std::vector<uint8_t>& bytes);
template<class T> T ToObject(std::vector<uint8_t>&& bytes);

struct Error { std::string path; std::string what; };
class CParseError : public std::runtime_error {
public:
    explicit CParseError(const Error& error);
    const Error& GetError() const;
};
```

Parameters and rvalue buffer adoption conditions are the same as for TryToObject. Returns a new T on success; throws CParseError on decode failure. The GetError reference and what string are valid for the exception object's lifetime. Catch CParseError in the same module; do not pass it across a DLL boundary.

**C++**

```cpp
std::vector<uint8_t> bytes = demo::Heartbeat().Serialize();
try {
    auto route = protaco::ToObject<demo::DeliveryRoute>(bytes);
} catch (const protaco::CParseError& e) {
    std::string reason = e.GetError().what;
    std::string path = e.GetError().path;
    std::string combined = e.what();
}
```

<details>
<summary>C#</summary>

```csharp
try {
    var route = Protaco.Codec.ToObject<demo.DeliveryRoute>(
        new demo.Heartbeat().Serialize());
} catch (Protaco.ProtacoException e) {
    string reason = e.What;
    string path = e.Path;
    string combined = e.Message;
}
```

</details>

<details>
<summary>Python</summary>

```python
from demo.Heartbeat_g import Heartbeat

try:
    route = protaco.ToObject(DeliveryRoute, Heartbeat().Serialize())
except protaco.ProtacoError as error:
    reason, path, combined = error.what, error.path, str(error)
```

</details>

**TryView**

**C++**

```cpp
template<class T>
bool TryView(const uint8_t* src, size_t size, typename T::View* view,
             std::string* error = nullptr);
template<class T>
bool TryView(const std::vector<uint8_t>& bytes, typename T::View* view,
             std::string* error = nullptr);
template<class T>
bool TryView(std::vector<uint8_t>&& bytes, typename T::View* view,
             std::string* error = nullptr) = delete;
```

- Requirements: a version with no variable fields and a generated T::View, independent of whether ext is allowed
- Parameters: input frame, output view, and optional error string. Specify T explicitly
- Returns: true with a read-only view on success. Uses the same acceptance checks and error wording as TryToObject
- Lifetime: valid only while the input bytes remain alive and unchanged. Check success before reading the view
- Cost: no copy of the frame. No allocation either while at most 16 entries (ext, or fields of a newer version) follow the fixed block. Ext values are not accessible through the view
- Deleted overload: prevents a view from referencing a temporary vector

**C++**

```cpp
demo::Heartbeat beat;
beat.set_node("oven-1");
beat.set_uptimeSec(42);
const std::vector<uint8_t> bytes = beat.Serialize();
demo::Heartbeat::View view;
std::string error;
if (protaco::TryView<demo::Heartbeat>(bytes, &view, &error)) {
    int32_t uptime = view.get_uptimeSec();
    protaco::Envelope env = view.GetEnvelope();
}
```

<details>
<summary>C#</summary>

```csharp
byte[] bytes = new demo.Heartbeat { Node = "oven-1", UptimeSec = 42 }.Serialize();
if (demo.Heartbeat.TryView(bytes, out var view, out string error)) {
    int uptime = view.UptimeSec;
    Protaco.Envelope env = view.GetEnvelope();
}
```

</details>

<details>
<summary>Python</summary>

Not applicable. Python uses object decoding.

</details>

**TryMutableView**

**C++**

```cpp
template<class T>
bool TryMutableView(uint8_t* data, size_t size, typename T::MutableView* view,
                    std::string* error = nullptr);
template<class T>
bool TryMutableView(std::vector<uint8_t>& bytes, typename T::MutableView* view,
                    std::string* error = nullptr);

static constexpr size_t T::MutableView::FrameSize();
```

- Requirements: the same as TryView; `T::MutableView` is generated next to `T::View`
- Parameters: memory holding a frame of T (for example shared memory filled once from `T().Serialize()`), output view, optional error string
- Returns: true with a view whose getters and setters work in place. Checks and error wording are those of TryView; zeroed memory fails with `bad magic`
- Members: the field getters and setters of T (`mutable_` and `set_` for fixed messages and array elements), the envelope getters and `SetId`, `SetCorr`, `SetTimestamp`, `SetTimestampAsNow`, `SetEnvelope`
- `FrameSize()`: bytes of a new T frame, the size of the memory to prepare
- Lifetime and concurrency: valid while the memory lives. Writes are plain stores; synchronize concurrent readers and writers yourself
- C# and Python: not available

**C++**

```cpp
std::vector<uint8_t> memory = arrays::Rack().Serialize();   // stands in for a shared memory region
arrays::Rack::MutableView rack;
if (protaco::TryMutableView<arrays::Rack>(memory, &rack)) {
    rack.mutable_slots(1).set_id(500);
    rack.set_count(9);
}
```

**PeekHeader and Header**

**C++**

```cpp
struct Header {
    std::string name;
    uint16_t version = 0;
    uint16_t minVersion = 0;
    uint32_t schemaHash = 0;
    Envelope env;
    uint32_t bodySize = 0;
};
bool PeekHeader(const uint8_t* src, size_t size, Header* out,
                std::string* error = nullptr);
```

- Parameters: bytes containing at least the header, their length, output Header, and optional error string
- Returns: true with header values on success. schemaHash is the fingerprint for the sender's version
- Notes: a complete header is sufficient even without the body. Message type, body length, and body validity are checked during decoding
- Lifetime: Header owns its name and envelope and does not depend on the input bytes
- Complete frame size: 28 + UTF-8 name byte count + 4 × supported version count + bodySize

**C++**

```cpp
auto bytes = demo::DeliveryRoute().Serialize();
protaco::Header header;
std::string error;
bool ok = protaco::PeekHeader(bytes.data(), bytes.size(), &header, &error);
```

<details>
<summary>C#</summary>

```csharp
byte[] bytes = new demo.DeliveryRoute().Serialize();
bool ok = Protaco.Codec.PeekHeader(bytes, out var header, out string error);
```

</details>

<details>
<summary>Python</summary>

```python
result = protaco.PeekHeader(DeliveryRoute().Serialize())
if result:
    header = result.value
```

</details>

### 4.4 Extension data

**CExt::SetValue**

**C++**

```cpp
bool SetValue(const std::string& key, bool value, std::string* error = nullptr);
bool SetValue(const std::string& key, int32_t value, std::string* error = nullptr);
bool SetValue(const std::string& key, int64_t value, std::string* error = nullptr);
bool SetValue(const std::string& key, uint32_t value, std::string* error = nullptr);
bool SetValue(const std::string& key, uint64_t value, std::string* error = nullptr);
bool SetValue(const std::string& key, double value, std::string* error = nullptr);
bool SetValue(const std::string& key, const std::string& value, std::string* error = nullptr);
bool SetValue(const std::string& key, const char* value, std::string* error = nullptr);
bool SetValue(const std::string& key, const std::vector<uint8_t>& value,
              std::string* error = nullptr);
template<class T>
bool SetValue(const std::string& key, T value, std::string* error = nullptr) = delete;
```

- Parameters: `key` is a key of 1..255 UTF-8 bytes; `value` is the value to store
- Returns: true on success. Replaces both type and value when the key exists
- Failure: empty or long key, `_` prefix, or a message field key in any supported version
- Supported types: only the overloads above. Explicitly cast other numeric types to the intended width
- Notes: strings and bytes are copied. A null `const char*` means an empty string

**CExt::TryGetValue and management functions**

The `V*` notation below abbreviates separate overloads for bool, int32_t, int64_t, uint32_t, uint64_t, double, std::string, and std::vector<uint8_t>.

**C++**

```cpp
bool TryGetValue(const std::string& key, V* out, std::string* error = nullptr) const;
bool Has(const std::string& key) const;
bool Remove(const std::string& key);
size_t Count() const;
std::vector<std::string> Keys() const;
```

- Parameters: key to query or remove, plus TryGetValue's output and optional error
- Returns: TryGetValue returns success and copies the value; failure leaves out unchanged
- Type checks: integer widths are distinct types. No automatic numeric conversion
- Management: Has reports presence; Remove reports actual removal; Count returns the count; Keys returns a copy in insertion order
- Failure reasons: missing key, type mismatch, or incorrect stored scalar length

**C++**

```cpp
demo::PizzaOrder order;
std::string error;
bool stored = order.GetMutableExt().SetValue("priority", int32_t{5}, &error);
int32_t priority = 0;
bool found = order.GetExt().TryGetValue("priority", &priority, &error);
bool present = order.GetExt().Has("priority");
size_t count = order.GetExt().Count();
auto keys = order.GetExt().Keys();
bool removed = order.GetMutableExt().Remove("priority");
```

<details>
<summary>C#</summary>

```csharp
var order = new demo.PizzaOrder();
bool stored = order.Ext.SetValue("priority", 5, out string error);
bool found = order.Ext.TryGetValue("priority", out int priority, out error);
bool present = order.Ext.Has("priority");
int count = order.Ext.Count;
var keys = order.Ext.Keys;
bool removed = order.Ext.Remove("priority");
```

</details>

<details>
<summary>Python</summary>

```python
from demo.PizzaOrder_g import PizzaOrder

order = PizzaOrder()
stored = order.ext.SetValue("priority", 5, protaco.INT)
found = order.ext.TryGetValue("priority", protaco.INT)
present = order.ext.Has("priority")
count = order.ext.Count
keys = order.ext.Keys()
removed = order.ext.Remove("priority")
```

</details>

### 4.5 Text

**C++**

```cpp
struct TextStyle {
    const char* separator = ", ";
    const char* open = "{";
    const char* close = "}";
    int indent = 0;
    size_t maxText = 64;
    bool envelope = false;
    static TextStyle Pretty(int indentWidth = 2);
};
```

- Settings: separator separates single-line items; open and close are delimiters; indent is spaces per level, with 0 for one line
- maxText: string truncation threshold; bytes in C++, characters in C# and Python
- envelope: displays the top-level envelope. Pretty returns a TextStyle with indentWidth and envelope=true
- Lifetime: C++ delimiter pointers must reference strings valid until ToString completes

Fields appear in YAML order, followed by `ext{...}`. Strings escape quotes, backslashes, newlines, and other special characters; long strings are shortened as `"abcd"...(+12)`. Bytes show at most the first eight bytes. Doubles use 15 significant digits and nan/inf/-inf; both 0 and -0 print as `0`. Enums print as `Large(3)`, unknown numbers as `?(9)`, and absent optional fields as `null`. Maps sort keys by code point for strings and by value for numbers and enums.

Pretty displays ts as local time within the supported range, otherwise as an integer. ASCII values produce the same text in all three languages.

**C++**

```cpp
demo::DeliveryRoute route;
route.set_rider("rider-7");
demo::RouteStop stop;
stop.set_orderId("A-100");
stop.set_address("1 Pizza St");
route.mutable_stops().push_back(stop);
route.GetMutableExt().SetValue("priority", int32_t{5});
std::string compact = route.ToString();
std::string pretty = route.ToString(protaco::TextStyle::Pretty());
```

<details>
<summary>C#</summary>

```csharp
var route = new demo.DeliveryRoute { Rider = "rider-7" };
route.Stops.Add(new demo.RouteStop { OrderId = "A-100", Address = "1 Pizza St" });
route.Ext.SetValue("priority", 5);
string compact = route.ToString();
string pretty = route.ToString(Protaco.TextStyle.Pretty());
```

</details>

<details>
<summary>Python</summary>

```python
route = DeliveryRoute(rider="rider-7")
route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St"))
route.ext.SetValue("priority", 5, protaco.INT)
compact = route.ToString()
pretty = route.ToString(protaco.TextStyle.Pretty())
```

</details>

Actual output from the example program follows. The envelope times vary by run; the ids follow the order in which the example creates messages.

```text
DeliveryRoute{rider="rider-7", stops=[RouteStop{orderId="A-100", address="1 Pizza St", etaMin=15}], note="", ext{priority=5}}
DeliveryRoute{
  envelope{id=8, corr=0, ts=2026-10-20 03:07:13.940328},
  rider="rider-7",
  stops=[
    RouteStop{
      orderId="A-100",
      address="1 Pizza St",
      etaMin=15
    }
  ],
  note="",
  ext{
    priority=5
  }
}
```

### 4.6 Enums

**C++**

```cpp
// PizzaSize.enum.h, namespace demo
enum class PizzaSize : int32_t { Small = 1, Medium = 2, Large = 3 };
const char* EnumName(PizzaSize value);
bool TryParseEnum(const std::string& name, PizzaSize* out,
                  std::string* error = nullptr);
```

- Parameters: value is an enum value; name is a YAML value name; out is the output pointer
- Returns: EnumName returns a static name, including removed values, or nullptr for an unknown number
- TryParseEnum: accepts only names in the current version. Failure returns false, leaves out unchanged, and sets error such as `unknown PizzaSize name: Kids`
- Compatibility: receivers preserve unknown numbers as enum values. The serialized type is int32
- Notes: never reuse a removed number; keep removed value lines in the schema

**C++**

```cpp
demo::PizzaSize size = demo::PizzaSize::Large;
std::string error;
bool ok = demo::TryParseEnum("Medium", &size, &error);
const char* name = demo::EnumName(size);
```

<details>
<summary>C#</summary>

```csharp
bool ok = demo.PizzaSizeNames.TryParse("Medium", out demo.PizzaSize size);
string name = demo.PizzaSizeNames.Name(size);
```

</details>

<details>
<summary>Python</summary>

```python
from demo.PizzaSize_enum import PizzaSize, PizzaSizeNames

result = PizzaSizeNames.TryParse("Medium")
if result:
    size = result.value
    name = PizzaSizeNames.Name(size)
```

</details>

### 4.7 Constants and limits

All are in namespace `protaco`.

**C++**

```cpp
enum : uint32_t {
    kFrameVersion = 4,
    kMaxDepth = 100,
    kMaxSchemaVersions = 255,
    kMaxFields = 1024,
    kMaxFrameBytes = 0xFFFFFFFFu,
    kMaxKeyLength = 255
};
```

| Constant | Meaning |
|---|---|
| kFrameVersion | Serialization format 4 |
| kMaxDepth | 100 levels of message nesting |
| kMaxSchemaVersions | At most 255 consecutive supported versions per frame |
| kMaxFields | At most 1,024 variable TLVs and ext entries per body |
| kMaxFrameBytes | Complete frame limit of 4 GiB - 1 bytes |
| kMaxKeyLength | Keys up to 255 UTF-8 bytes |

List counts are checked against remaining bytes / 4 and map counts against remaining bytes / 8. See [5.7](#57-constants-and-limits) for C#'s smaller array limit.

The runtime version guard is `PROTACO_RUNTIME_0_6_0`. Copies in different output folders include the same runtime version only once. Mixing runtime versions causes redefinition errors.

**C++**

```cpp
static_assert(protaco::kFrameVersion == 4, "serialization format");
```

## 5. C# API

Add [Protaco.cs](../include/cs/Protaco.cs) and the `<Name>.g.cs` files to the project. .NET Framework 4.6.1 and .NET 8 are supported, and the runtime uses C# 7.3. This chapter lists the C# names and differences in the same order as the C++ API. C# examples for each operation also appear in the collapsed code of chapter 4.

### 5.1 Generated classes

<details>
<summary>C# signatures</summary>

```csharp
// Representative members of demo.DeliveryRoute:
public DeliveryRoute();
public string Rider { get; set; }
public List<RouteStop> Stops { get; set; }
public string Note { get; set; }
public Protaco.Ext Ext { get; }
public byte[] Serialize();
public override string ToString();
public string ToString(Protaco.TextStyle style);
public const string MsgName = "DeliveryRoute";
public const ushort MsgVersion = 1;
public const ushort MsgMinVersion = 1;
public const uint MsgSchemaHash = 0x7A0526C6u;
```

</details>

A class implements `Protaco.Message, Protaco.IMessage`. Field names are properties: the YAML key with its first letter capitalized. Lists are List, maps are Dictionary, and bytes are byte[]; all are read and assigned through properties.

- Constructor: schema defaults and a new envelope
- Property setter: assigns the new value. A fixed string longer than N UTF-8 bytes throws ArgumentException and keeps the old value
- Serialize return value: a new byte[] with the envelope on every call. Later changes to the object do not affect an earlier array
- ToString return value: a string. The style argument sets the output format
- Ext: public only on open messages. Returns the Ext owned by the message
- Metadata: const fields instead of parameterless functions
- Single self reference: null by default; assign null to remove it
- Older versions: `<ns>.v<N>.<Name>` in the same file
- View: a struct for versions with only fixed fields. See [5.3](#53-decoding)

<details>
<summary>C# example</summary>

```csharp
var route = new demo.DeliveryRoute { Rider = "rider-7" };
route.Stops.Add(new demo.RouteStop { OrderId = "A-100", Address = "1 Pizza St" });
byte[] bytes = route.Serialize();
string text = route.ToString();
uint hash = demo.DeliveryRoute.MsgSchemaHash;
```

</details>

### 5.2 Envelope

<details>
<summary>C# signatures</summary>

```csharp
// Protaco.Envelope:
public uint Id;
public uint Corr;
public long Ts;

// Inherited message members:
public uint GetId();
public void SetId(uint id);
public uint GetCorr();
public void SetCorr(uint corr);
public long GetTimestamp();
public void SetTimestamp(long timestampUs);
public void SetTimestampAsNow();
public Protaco.Envelope GetEnvelope();
public void SetEnvelope(Protaco.Envelope env);

public bool TryGetTimeFromTimestamp(out string date, out string time);
public bool TryGetTimeFromTimestamp(out string date, out string time, out string error);
public bool TryGetTimeFromTimestamp(out int year, out int month, out int day,
    out int hour, out int minute, out int second, out int millisecond, out int microsecond);
public bool TryGetTimeFromTimestamp(out int year, out int month, out int day,
    out int hour, out int minute, out int second, out int millisecond, out int microsecond,
    out string error);

// Protaco.Envelopes:
public static uint NextId();
public static long NowUs();
public static void SetClock(System.Func<long> nowUs);
public static void DisableAutoEnvelope();
public static void EnableAutoEnvelope();
public const long TimestampMin = 946684800000000L;
public const long TimestampMax = 253402300799999999L;
public static bool TryGetTime(long timestampUs, out string date, out string time, out string error);
public static bool TryGetTime(long timestampUs, out int year, out int month, out int day,
    out int hour, out int minute, out int second, out int millisecond, out int microsecond,
    out string error);
```

</details>

id, corr, ts, and the time range mean the same as in [4.2](#42-envelope). Getters return the value and setters return void. GetEnvelope returns a copy of the struct. The clock callback must return UTC microseconds, and null restores the system clock. NextId is thread-safe; replace the clock before creating messages. DisableAutoEnvelope and EnableAutoEnvelope work per process, as in [4.2](#42-envelope).

Time conversion returns success as a bool and writes the local time to the out arguments. On failure, string outputs are null and integer outputs are 0. The message members also have overloads without error, but `Envelopes.TryGetTime` always takes out error.

<details>
<summary>C# example</summary>

```csharp
var request = new demo.DeliveryRoute();
var reply = new demo.DeliveryRoute();
reply.SetCorr(request.GetId());
bool ok = reply.TryGetTimeFromTimestamp(out string date, out string time, out string error);
```

</details>

### 5.3 Decoding

<details>
<summary>C# signatures</summary>

```csharp
// Protaco.Codec:
public static T ToObject<T>(byte[] bytes) where T : Protaco.IMessage, new();
public static bool TryToObject<T>(byte[] bytes, out T message, out string error)
    where T : Protaco.IMessage, new();
public static bool TryToObject<T>(byte[] bytes, out T message)
    where T : Protaco.IMessage, new();
public static bool PeekHeader(byte[] bytes, out Protaco.Header header, out string error);
public static bool PeekHeader(byte[] bytes, out Protaco.Header header);

// Protaco.ProtacoException : System.Exception:
public ProtacoException(string path, string what);
public string Path { get; }
public string What { get; }

// Protaco.Header:
public string Name;
public ushort Version;
public ushort MinVersion;
public uint SchemaHash;
public Protaco.Envelope Env;
public uint BodySize;

// demo.Heartbeat:
public static bool TryView(byte[] bytes, out View view, out string error);
public static bool TryView(byte[] bytes, out View view);
```

</details>

- Parameters: bytes is the frame, out receives the result, error receives the reason
- ToObject: returns T on success and throws ProtacoException when decoding fails. Message is `path: reason`
- TryToObject: true on success. On failure, false with default(T) and the reason in error. The out argument is always overwritten
- PeekHeader: true and the Header on success. On failure, header is null. The body is not checked
- TryView: only for versions with only fixed fields. A View on success, default(View) on failure
- View lifetime: an internal reference keeps the array, and the view is valid while the array stays unchanged. Fields are read-only properties; the envelope is GetEnvelope()
- Buffer ownership transfer: not applicable

<details>
<summary>C# example</summary>

```csharp
byte[] bytes = new demo.Heartbeat { Node = "oven-1", UptimeSec = 42 }.Serialize();
bool ok = Protaco.Codec.TryToObject<demo.Heartbeat>(bytes, out var message, out string error);
bool headerOk = Protaco.Codec.PeekHeader(bytes, out var header, out error);
if (demo.Heartbeat.TryView(bytes, out var view, out error)) {
    int uptime = view.UptimeSec;
    uint id = view.GetEnvelope().Id;
}
```

</details>

### 5.4 Extension data

<details>
<summary>C# signatures</summary>

```csharp
// Protaco.Ext:
public bool SetValue<T>(string key, T value);
public bool SetValue<T>(string key, T value, out string error);
public bool TryGetValue<T>(string key, out T value);
public bool TryGetValue<T>(string key, out T value, out string error);
public bool Has(string key);
public bool Remove(string key);
public int Count { get; }
public System.Collections.Generic.IEnumerable<string> Keys { get; }
```

</details>

The key, value, and error parameters and the key rules are the same as in [4.4](#44-extension-data). T is bool, int, long, uint, ulong, double, string, or byte[]. SetValue and TryGetValue return false and an error string on failure. A failed TryGetValue outputs default(T). An unsupported T is rejected by the call result, not at compile time.

Has reports whether the key exists, Remove whether something was removed, and Count the number of entries. Keys is an enumerable property in insertion order, so do not change ext while enumerating it. Usage is shown in [the C# code of 4.4](#44-extension-data).

### 5.5 Text

<details>
<summary>C# signatures and example</summary>

```csharp
// Protaco.TextStyle:
public string Separator { get; set; } = ", ";
public string Open { get; set; } = "{";
public string Close { get; set; } = "}";
public int Indent { get; set; }
public int MaxText { get; set; } = 64;
public bool Envelope { get; set; }
public static TextStyle Pretty(int indentWidth = 2);
```

```csharp
string text = new demo.DeliveryRoute().ToString(Protaco.TextStyle.Pretty());
```

</details>

Each property and Pretty mean the same as in [4.5](#45-text). Pretty returns a new TextStyle. MaxText counts characters.

### 5.6 Enums

<details>
<summary>C# signatures and example</summary>

```csharp
// namespace demo
public enum PizzaSize : int { Small = 1, Medium = 2, Large = 3 }
// PizzaSizeNames:
public static string Name(PizzaSize value);
public static bool TryParse(string name, out PizzaSize value);
```

```csharp
bool ok = demo.PizzaSizeNames.TryParse("Medium", out var size);
string name = demo.PizzaSizeNames.Name(size);
```

</details>

Name also finds removed values and returns null for an unknown number. TryParse accepts only names in the current version and returns false with default(PizzaSize) on failure. An unknown number that was received is kept as an enum value.

### 5.7 Constants and limits

<details>
<summary>C# signatures</summary>

```csharp
// Protaco.Codec:
public const int FrameVersion = 4;
public const int MaxDepth = 100;
public const int MaxSchemaVersions = 255;
public const int MaxFields = 1024;
public const int MaxFrameBytes = 0x7FFFFFC7;
```

</details>

Because of the array size limit, a frame is at most 0x7FFFFFC7 bytes, about 2 GiB. The other limits are the same as in [4.7](#47-constants-and-limits). Runtime-only members sit in the explicit IMessage implementation of the generated class; application code does not need to call them.

## 6. Python API

[protaco.py](../include/python/protaco.py) needs Python 3.8 or later and only the standard library. Try functions return a Result.

### 6.1 Packages and imports

Put the output root on sys.path. The namespace `demo` becomes `demo/`, and `pizza.orders` becomes `pizza/orders/`. Generated modules import each other relatively.

```text
<py_out>/
  protaco.py
  demo/
    DeliveryRoute_g.py
    RouteStop_g.py
    PizzaSize_enum.py
```

<details>
<summary>Python</summary>

```python
# From the repository root:
import sys
sys.path.insert(0, "examples/taco_generated/py")
import protaco
from demo.DeliveryRoute_g import DeliveryRoute
from demo.RouteStop_g import RouteStop
from demo.PizzaSize_enum import PizzaSize, PizzaSizeNames
```

</details>

A namespace named like a standard module, such as `types`, can hide that module, so avoid it.

### 6.2 Generated classes and Serialize

<details>
<summary>Python signatures</summary>

```python
class DeliveryRoute(protaco.Message):
    def __init__(self, *, _stamp=None, **fields): ...

# Inherited members:
def Serialize(self): ...
def ToString(self, style=None): ...
```

</details>

- Parameters: fields are values by YAML key. _stamp is an internal option the generated code uses for nested fields
- Attributes: the YAML keys as they are. A Python keyword gets a trailing `_` (`from_`, `pass_`)
- Constructor error: TypeError for an unknown key
- Serialize return value: bytes, envelope included
- Serialize error: ProtacoError for numbers out of range, fixed strings that are too long, and the like. Example: `orderId: 17 bytes, longer than string(16)`
- Containers: list, dict, bytes. A single self reference defaults to None
- Metadata: MsgName, MsgVersion, MsgMinVersion, MsgSchemaHash class attributes
- Older versions: `v<N>.<Name>` in the same generated module

The constructor sets id and ts. The decoder and nested fields initialize with `_stamp=protaco.NO_STAMP` and take no id. Value ranges are checked in Serialize.

<details>
<summary>Python example</summary>

```python
route = DeliveryRoute(rider="rider-7", note="closest-first")
route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=10))
data = route.Serialize()
schema_hash = DeliveryRoute.MsgSchemaHash

# With protaco_compiler/tests/taco_generated/py on sys.path:
from build3.Order_g import v1
old_order = v1.Order()
restored = protaco.ToObject(v1.Order, old_order.Serialize())
```

</details>

### 6.3 Decoding and Result

<details>
<summary>Python signatures</summary>

```python
protaco.ToObject(cls, data)
protaco.TryToObject(cls, data)
protaco.PeekHeader(data)
protaco.Result(ok, value=None, error=None)
protaco.ProtacoError(path, what)
```

</details>

- Parameters: cls is the generated class, data is the frame bytes
- ToObject return value: an object of cls. Raises ProtacoError on failure; read path, what, and str(error)
- TryToObject return value: a Result whose value is the object on success
- PeekHeader return value: a Result whose value is a Header on success. Checks only the header, not the body
- Header attributes: name, version, minVersion, schemaHash, env, bodySize
- Result: bool(result) is ok, value holds the result on success, error holds the reason string on failure. value is None on failure
- Unpacking: `ok, value, error = result`
- Error handling: a bad frame gives a failed Result from Try functions instead of an exception

<details>
<summary>Python example</summary>

```python
data = DeliveryRoute(rider="rider-7").Serialize()
result = protaco.TryToObject(DeliveryRoute, data)
if result:
    received = result.value
else:
    reason = result.error

ok, header, error = protaco.PeekHeader(data)
if ok:
    name = header.name
```

</details>

### 6.4 Extension data

<details>
<summary>Python signatures</summary>

```python
message.ext.SetValue(key, value, kind=None)
message.ext.TryGetValue(key, kind)
message.ext.Has(key)
message.ext.Remove(key)
message.ext.Count
message.ext.Keys()
```

</details>

The key rules are the same as in [4.4](#44-extension-data). SetValue returns a Result of success, and TryGetValue a Result holding the value. The reason for a failure is in error.

- kind may be omitted for: bool→BOOL, float→DOUBLE, str→STRING, bytes→BYTES
- Integers: kind is required, one of INT, INT64, UINT, UINT64
- Reading: the expected kind is required. No automatic conversion between integer widths
- Has and Remove return bool, Count is an int property, Keys() returns a copy of the keys in insertion order
- Closed messages: the ext attribute exists, but writes fail with `ext is off for this message (allow_extension false)`

<details>
<summary>Python example</summary>

```python
route = DeliveryRoute()
stored = route.ext.SetValue("priority", 5, protaco.INT)
result = route.ext.TryGetValue("priority", protaco.INT)
if result:
    priority = result.value
```

</details>

### 6.5 Envelope and time

<details>
<summary>Python signatures</summary>

```python
protaco.Envelope(id=0, corr=0, ts=0)
message.GetId()
message.SetId(id)
message.GetCorr()
message.SetCorr(corr)
message.GetTimestamp()
message.SetTimestamp(timestamp_us)
message.SetTimestampAsNow()
message.GetEnvelope()
message.SetEnvelope(env)
message.TryGetTimeFromTimestamp()
message.TryGetTimeFromTimestampParts()

protaco.NextId()
protaco.NowUs()
protaco.SetClock(now_us=None)
protaco.DisableAutoEnvelope()
protaco.EnableAutoEnvelope()
protaco.TryGetTime(timestamp_us)
protaco.TryGetTimeParts(timestamp_us)
protaco.TIMESTAMP_MIN  # 946684800000000
protaco.TIMESTAMP_MAX  # 253402300799999999
```

</details>

id, corr, and ts mean the same as in [4.2](#42-envelope). Getters return the value and setters return None. GetEnvelope returns a copy, and SetEnvelope copies the value too. NextId is thread-safe. SetClock takes a callback that returns UTC microseconds; None goes back to the system clock. Set it before creating messages. DisableAutoEnvelope and EnableAutoEnvelope work per process, as in [4.2](#42-envelope).

Time conversion takes UTC microseconds and gives local time. TryGetTime and the matching message function put `(date, time)` in Result.value. The Parts variants give `(year, month, day, hour, minute, second, millisecond, microsecond)`. A value outside the supported range, or a failed conversion, is reported in error.

<details>
<summary>Python example</summary>

```python
request = DeliveryRoute()
reply = DeliveryRoute()
reply.SetCorr(request.GetId())
result = reply.TryGetTimeFromTimestamp()
if result:
    date, time = result.value
```

</details>

### 6.6 Text

<details>
<summary>Python signatures and example</summary>

```python
protaco.TextStyle(separator=", ", open="{", close="}", indent=0, maxText=64, envelope=False)
protaco.TextStyle.Pretty(indent_width=2)
message.ToString(style=None)
str(message)
```

```python
compact = str(DeliveryRoute())
pretty = DeliveryRoute().ToString(protaco.TextStyle.Pretty())
```

</details>

The settings are the same as in [4.5](#45-text). Pretty returns a new TextStyle; ToString and str return a string. maxText counts characters.

### 6.7 Enums

<details>
<summary>Python signatures and example</summary>

```python
# PizzaSize is an enum.IntEnum.
PizzaSizeNames.Name(value)
PizzaSizeNames.TryParse(name)
```

```python
result = PizzaSizeNames.TryParse("Medium")
if result:
    size = result.value
    name = PizzaSizeNames.Name(size)
```

</details>

Name also finds removed values and returns None for an unknown number. TryParse accepts only YAML names in the current version and returns the enum value in Result.value. The failure string looks like `unknown PizzaSize name: Kids`.

The decoder keeps an unknown number as a plain int. An enum member that is a Python keyword becomes something like `None_`, but Name and TryParse use the original YAML name.

### 6.8 Constants and limits

<details>
<summary>Python constants</summary>

```python
protaco.VERSION              # "0.7.0"
protaco.FRAME_VERSION        # 4
protaco.MAX_DEPTH            # 100
protaco.MAX_SCHEMA_VERSIONS  # 255
protaco.MAX_FIELDS           # 1024
protaco.MAX_FRAME_BYTES      # 0xFFFFFFFF
```

</details>

The limits are the same as in [4.7](#47-constants-and-limits). The fixed block is packed and unpacked with struct.Struct. View and the C++ API that takes a moved buffer do not apply.

## 7. Build and test

### 7.1 Running and environment

Run from the repository root. Windows needs Visual Studio 2017 or later (MSBuild 15, v141), .NET Framework 4.6.1, and Python 3.8 or later. Linux needs .NET SDK 8 or later, g++, and Python 3.8 or later. build.sh is also intended for macOS, but macOS has not been verified; the verification record below is for Windows and Linux.

**Windows command prompt**

```bat
build.cmd
```

**Linux and macOS shell**

```sh
sh build.sh
# When dotnet is installed under ~/.dotnet:
DOTNET=~/.dotnet/dotnet sh build.sh
```

Both scripts run the same six steps.

1. Build protacoc.
2. Generate test and example code.
3. Build and run the C++ tests.
4. Build and run the C# tests.
5. Run the Python tests.
6. Build and run the C++, C#, and Python examples.

| Environment variable | Role | Default |
|---|---|---|
| MSBUILD | MSBuild path on Windows | MSBuild.exe of Visual Studio 2017 Professional |
| DOTNET | .NET CLI for build.sh | dotnet |
| CXX | C++ compiler for build.sh | g++ |
| PYTHON | Python executable | Windows: python, shell: python3 |

Windows output goes to `bin/Release`. On Linux, .NET output goes to `bin/Release/net8.0` and C++ output to `bin/Release`. Intermediate files gather in `.IntermediateDirectory/`. The Linux script builds the tests and examples as C++17 and checks a separate translation unit as C++14.

`protaco.sln` holds the generator and the C++ and C# tests; `examples/examples.sln` is the examples solution. The generator projects `protacoc.csproj` and `protacoc.net.csproj` build the same sources for .NET Framework 4.6.1 and .NET 8. The C# tests and examples have matching .net.csproj files too.

The build rewrites generated code. The tests rewrite the C++ golden files only when `PROTACO_UPDATE_GOLDEN=1`. The googletest sources ship in the repository, so there is nothing to install.

### 7.2 Repository layout

```text
include/
  cpp/Protaco.hpp
  cs/Protaco.cs
  python/protaco.py
protaco_compiler/
  protacoc/
  tests/
    cpp_tests/
    cs_tests/
    py_tests/
    schemas/
    taco_generated/
    golden/
    googletest/
examples/
  schemas/
  taco_generated/
  cpp_examples/
  cs_examples/
  python_examples/
docs/
  benchmark/
  img/
bin/
  Release/
.IntermediateDirectory/
```

The runtime sources are in include/, the generator in protaco_compiler/protacoc/, and the tests in protaco_compiler/tests/. Example schemas and programs are in [examples](../examples/README.md), and performance measurements in [benchmark](benchmark/README.md).

### 7.3 Test coverage

The 0.7.0 verification record: 48 C++ tests, 42 C# tests, and 30 Python tests pass on both Windows and Linux. The Linux environment is WSL Ubuntu 22.04, g++ 11.4, .NET 8.0.426, and Python 3.10. Regenerating the code on Linux produces byte-identical files.

Test names and implementations are in the [C++ tests](../protaco_compiler/tests/cpp_tests/ProtacoTest.cpp), [C# tests](../protaco_compiler/tests/cs_tests/Program.cs), and [Python tests](../protaco_compiler/tests/py_tests/test_protaco.py).

- Generator: YAML per error code, cycles, mixed namespaces, the nesting limit, the version-feature restriction on field-type messages, enum defaults, input-order independence, and `--check` results for up-to-date and modified files
- Schema contract: FNV-1a canonical text; name, version, and fingerprint mismatches; old fingerprints kept; reading a new frame as one's own version; choosing an older type; the sender's min_version check
- Values and containers: defaults, fixed strings that are too long, bytes, unsigned integers, UTF-8, enums, maps, self references, the 100-level recursion limit
- Errors and paths: missing required fields, nested paths, closed messages rejecting keys and dropping new fields, truncation at every length, C++ random bit flips
- Buffers and API: C++ fixed fields written in place, Serialize reuse, mutable references, copies, ext assignment, moved-buffer arguments; C++ and C# TryView; more than 16 variable fields; a C++14-only translation unit
- Envelope and text: id and time stamped at construction, decoding without taking an id, clock replacement, local time, PeekHeader, ToString identical in all three languages, Python keyword attributes
- Interoperability: C# and Python read the C++ golden files, and byte comparison for the same input. Golden files: DeliveryRoute, PizzaOrder, Heartbeat, DeliveryProof, Numbers, Pie, Menu, Node

Byte identity across languages is not guaranteed for an arbitrary map iteration order. The golden tests compare the results for their test data. Performance comparisons and measurement conditions are in the [benchmark](benchmark/README.md).
