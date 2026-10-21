# Protaco examples

[한국어](README.ko.md) · [README](../README.md) · [Reference](../docs/reference.md)

This folder contains YAML schemas, generated code, and C++, C#, and Python programs for protaco 0.7.0. Pizza orders and delivery routes demonstrate the API. The final section follows the revision history of `Order` to explain version management.

## Contents

1. [Folder layout](#1-folder-layout)
2. [Build and run](#2-build-and-run)
3. [Schemas](#3-schemas)
4. [Program walkthrough](#4-program-walkthrough)
5. [Generated code](#5-generated-code)
6. [Version management walkthrough](#6-version-management-walkthrough)

## 1. Folder layout

```text
examples/
  schemas/
  taco_generated/
    cpp/                  *.g.h, *.enum.h, *.const.h, Protaco.hpp
    cs/                   *.g.cs, *.enum.cs, *.const.cs, Protaco.cs
    py/
      protaco.py
      demo/               *_g.py, *_enum.py, *_const.py
  cpp_examples/
    main.cpp
    example.vcxproj
    example.vcxproj.filters
  cs_examples/
    Program.cs
    example_cs.csproj
    example_cs.net.csproj
  python_examples/
    main.py
  examples.sln
```

The schemas use namespace `demo`. Each generated folder includes its language's runtime. [examples.sln](examples.sln) groups the C++ and .NET Framework C# examples. The .NET 8 C# project and Python program run separately. The root build scripts regenerate all three languages, then build and run the examples.

## 2. Build and run

### Windows

Run from the repository root. Windows requires Visual Studio 2017 or newer with the v141 C++ tools, .NET Framework 4.6.1, and Python 3.8 or newer. Of the six steps in [build.cmd](../build.cmd), step 2 generates the example code and step 6 builds and runs all three examples.

**Windows cmd**

```bat
build.cmd
```

For manual commands, use a Visual Studio developer command prompt with MSBuild on PATH.

**Windows cmd**

```bat
msbuild protaco_compiler\protacoc\protacoc.csproj /p:Configuration=Release
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\taco_generated\cpp --cs_out examples\taco_generated\cs --py_out examples\taco_generated\py
msbuild examples\examples.sln /p:Configuration=Release /p:Platform=x64
bin\Release\protaco_example.exe
bin\Release\protaco_example_cs.exe
python examples\python_examples\main.py
```

[example.vcxproj](cpp_examples/example.vcxproj) adds `$(ProjectDir)..\taco_generated\cpp` to the include path. Its Pre-Build Event uses `bin\Release\protacoc.exe` if available, otherwise `bin\Debug\protacoc.exe`, to generate C++ and C# code. Generate Python code with the `--py_out` command above or the full build script. The project uses v141 x64, C++17, `/W4 /WX`, and `/utf-8`. No separate runtime library is linked. [Cpp14Check.cpp](../protaco_compiler/tests/cpp_tests/Cpp14Check.cpp) checks the generated headers for C++14 compatibility.

### Linux

Linux requires .NET SDK 8 or newer, g++, and Python 3.8 or newer. [build.sh](../build.sh) runs the same six steps.

**sh**

```sh
sh build.sh
```

To build and run just the examples manually:

**sh**

```sh
dotnet build protaco_compiler/protacoc/protacoc.net.csproj -c Release
bin/Release/net8.0/protacoc -I examples/schemas --cpp_out examples/taco_generated/cpp --cs_out examples/taco_generated/cs --py_out examples/taco_generated/py
mkdir -p bin/Release
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pthread -Iexamples/taco_generated/cpp examples/cpp_examples/main.cpp -o bin/Release/protaco_example
bin/Release/protaco_example
dotnet build examples/cs_examples/example_cs.net.csproj -c Release
bin/Release/net8.0/protaco_example_cs
python3 examples/python_examples/main.py
```

The Windows script accepts `MSBUILD` and `PYTHON`; the Linux script accepts `DOTNET`, `CXX`, and `PYTHON`. If .NET is not installed system-wide, use `DOTNET=~/.dotnet/dotnet sh build.sh`. For manual commands, also set `DOTNET_ROOT` to the installation path so the .NET app hosts can find the runtime.

## 3. Schemas

| Schema | Feature | Used by |
|---|---|---|
| [RouteStop](schemas/RouteStop.yaml) | Fixed strings, integer default | `DeliveryRoute.stops` elements |
| [DeliveryRoute](schemas/DeliveryRoute.yaml) | Variable strings, required fields, nested message list | Send/receive, errors, logs, envelopes |
| [Heartbeat](schemas/Heartbeat.yaml) | Fixed fields, `allow_extension: false` | Fixed buffer and View |
| [PizzaOrder](schemas/PizzaOrder.yaml) | Defaults, lists, nested messages | Extension data |
| [Topping](schemas/Topping.yaml) | Small message with fixed fields | `PizzaOrder.toppings` elements |
| [Delivery](schemas/Delivery.yaml) | `list<int>`, `list<string>` | `PizzaOrder.delivery` |
| [DeliveryProof](schemas/DeliveryProof.yaml) | `bytes`, `list<bytes>`, required bytes | Schema for binary delivery data |
| [PizzaSize.enum](schemas/PizzaSize.enum.yaml) | Enum values and names | `Menu.defaultSize` |
| [Menu](schemas/Menu.yaml) | Enum, `map<string, double>`, message list | Store menu |
| [Category](schemas/Category.yaml) | Self-reference through `list<Category>` | Menu subsections |
| [Limits.const](schemas/Limits.const.yaml) | Named constants for code and array lengths | `Board` array lengths |
| [Spot](schemas/Spot.yaml) | `fixed: true` message | `Board.depot` and `Board.spots` |
| [Board](schemas/Board.yaml) | Fixed-length arrays, fixed messages, a fixed-only frame | Shared memory with MutableView |

These excerpts come from the actual schemas, with introductory comments omitted. In the program, `Heartbeat` reports the status of pizza oven `oven-1`.

**YAML · Heartbeat.yaml**

```yaml
message: Heartbeat
version: 1
namespace: demo
# Closed contract: no ext, a frame with an unknown key is rejected.
allow_extension: false

fields:
  - {key: node, type: string(32), desc: device name}
  - {key: seq, type: int64, desc: sequence number}
  - {key: healthy, type: bool, default: true, desc: device is healthy}
  - {key: cpuLoad, type: double, desc: "CPU load (0.0..1.0)"}
  - {key: uptimeSec, type: int, desc: uptime in seconds}
```

**YAML · Menu.yaml**

```yaml
message: Menu
version: 1
namespace: demo
fields:
  - {key: store, type: string(16), desc: store code}
  - {key: defaultSize, type: PizzaSize, default: Large, desc: size when an order does not say}
  # Map types contain ',', so they are quoted inside {...}.
  - {key: prices, type: "map<string, double>", desc: price by dish name}
  - {key: sections, type: list<Category>, desc: top-level sections}
```

**YAML · Category.yaml**

```yaml
message: Category
version: 1
namespace: demo
fields:
  - {key: name, type: string, desc: section name}
  - {key: dishes, type: list<string>, desc: dish names}
  - {key: subsections, type: list<Category>, desc: sections inside this one}
```

**YAML · PizzaSize.enum.yaml**

```yaml
enum: PizzaSize
version: 1
namespace: demo
values:
  - {key: Small, value: 1, desc: 10 inch}
  - {key: Medium, value: 2, desc: 12 inch}
  - {key: Large, value: 3, desc: 14 inch}
```

**YAML · Board.yaml, Spot.yaml, Limits.const.yaml**

```yaml
message: Board
version: 1
namespace: demo
allow_extension: false
fields:
  - {key: depot, type: Spot, desc: where riders start}
  - {key: spots, type: Spot[Limits.MaxSpots], desc: delivery spots}
  - {key: ovens, type: double[3], default: 220.5, desc: oven temperatures}
  # The length comes from an enum value: PizzaSize.Large is 3.
  - {key: queue, type: PizzaSize[PizzaSize.Large], desc: sizes of the next pizzas}
  - {key: busy, type: bool[Limits.MaxRiders], desc: riders out on a delivery}
```

```yaml
message: Spot
version: 1
namespace: demo
fixed: true
fields:
  - {key: lat, type: double}
  - {key: lon, type: double}
  - {key: label, type: string(16)}
```

```yaml
const: Limits
namespace: demo
values:
  - {key: MaxSpots, value: 4, desc: spots on a dispatch board}
  - {key: MaxRiders, value: 8, desc: riders on duty}
```

Quote `map<K, V>` inside YAML `{...}` because it contains a comma. `Category.subsections` is a list of the same message type. Comments on `PizzaOrder.woodFired` and `deepDishCm` explain their use by different stores; comments do not affect serialization. `Board` is fixed-length: its arrays and its `Spot` fields live in the fixed block, so the whole board can sit in shared memory.

## 4. Program walkthrough

The [C++ program](cpp_examples/main.cpp) runs the nine steps below. The [C# program](cs_examples/Program.cs) has eight steps, without the fixed-buffer step. The [Python program](python_examples/main.py) has eight steps, without the zero-copy step. This is actual C++ output. The envelope timestamps vary between runs; the ids follow the order in which the example creates messages.

```text
DeliveryRoute: 182 bytes
  rider=rider-7 first=A-100 at 1 Pizza St in 10 min, note=closest-first
Heartbeat: received uptime=11, same buffer=yes
PizzaOrder ext: secretSauce=yes please, schema key in ext=rejected
decode failed: message name mismatch: expected DeliveryRoute, got Heartbeat
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
Menu: default size Large, margherita 9.5
Menu{store="NAPOLI-1", defaultSize=Large(3), prices={"margherita": 9.5, "marinara": 8}, sections=[Category{name="pizza", dishes=["margherita", "marinara"], subsections=[Category{name="white", dishes=["bianca"], subsections=[]}]}]}
Heartbeat view: node=oven-1 uptime=42
DeliveryRoute adopted the buffer: yes
Envelope: request id=15, reply id=16 corr=15 at 2026-10-20 03:07:13.941056
Board: spots[4]: index out of range (size 4)
Board: 4 spots, first north gate at 37.5665, 245 bytes in the region
Board{depot=Spot{lat=0, lon=0, label="depot"}, spots=[Spot{lat=37.5665, lon=126.978, label="north gate"}, Spot{lat=0, lon=0, label=""}, Spot{lat=0, lon=0, label=""}, Spot{lat=0, lon=0, label=""}], ovens=[220.5, 230, 220.5], queue=[Large(3), Small(1), Small(1)], busy=[false, false, true, false, false, false, false, false]}
```

C# omits `Heartbeat: received uptime=11, same buffer=yes` and `DeliveryRoute adopted the buffer: yes`. Python prints `Heartbeat: received uptime=10` for its fixed-field step and omits the `Heartbeat view: ...` and `DeliveryRoute adopted the buffer: ...` lines. In the last step, C# and Python fill a `Board` object instead of shared memory: they print `Board: 4 spots, first north gate at 37.5665, 245 bytes` without the out-of-range line, then the same `Board{...}` text. The snippets below are excerpts from each program's function bodies. Use the includes, using directives, and imports from the original programs.

### 4.1 Send and receive

**C++**

```cpp
demo::DeliveryRoute route;
route.set_rider("rider-7");
demo::RouteStop stop;
stop.set_orderId("A-100");
stop.set_address("1 Pizza St");
stop.set_etaMin(10);
route.mutable_stops().push_back(stop);
route.set_note("closest-first");

const std::vector<uint8_t>& bytes = route.Serialize();   // send bytes.data(), bytes.size()
std::printf("DeliveryRoute: %zu bytes\n", bytes.size());

const demo::DeliveryRoute got = protaco::ToObject<demo::DeliveryRoute>(bytes);
std::printf("  rider=%s first=%s at %s in %d min, note=%s\n",
    got.get_rider().c_str(), got.get_stops()[0].get_orderId().c_str(), got.get_stops()[0].get_address().c_str(),
    got.get_stops()[0].get_etaMin(), got.get_note().c_str());
```

<details>
<summary>C#</summary>

```csharp
DeliveryRoute route = new DeliveryRoute { Rider = "rider-7", Note = "closest-first" };
route.Stops.Add(new RouteStop { OrderId = "A-100", Address = "1 Pizza St", EtaMin = 10 });

byte[] bytes = route.Serialize();   // same serialization format as the C++ example
Console.WriteLine("DeliveryRoute: " + bytes.Length + " bytes");

DeliveryRoute got = Protaco.Codec.ToObject<DeliveryRoute>(bytes);
Console.WriteLine("  rider=" + got.Rider + " first=" + got.Stops[0].OrderId + " at " + got.Stops[0].Address
    + " in " + got.Stops[0].EtaMin + " min, note=" + got.Note);
```

</details>

<details>
<summary>Python</summary>

```python
route = DeliveryRoute(rider="rider-7", note="closest-first")
route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=10))

data = route.Serialize()   # send these bytes
print("DeliveryRoute: %d bytes" % len(data))

got = protaco.ToObject(DeliveryRoute, data)
stop = got.stops[0]
print("  rider=%s first=%s at %s in %d min, note=%s" % (got.rider, stop.orderId, stop.address, stop.etaMin, got.note))
```

</details>

C++ `Serialize()` returns a reference to the object's frame buffer. Pass `bytes.data()` and `bytes.size()` to the transport. `ToObject<T>` checks the name, version, and fingerprint, and throws `protaco::CParseError` on failure. Calling `mutable_stops()` exposes a reference whose later changes cannot be tracked, so subsequent serialization rebuilds the variable block each time. C# `Serialize()` returns a new array on every call; Python returns `bytes`.

### 4.2 Fixed fields only

**C++**

```cpp
demo::Heartbeat beat;
beat.set_node("oven-1");
beat.set_uptimeSec(10);
const std::vector<uint8_t>& bytes = beat.Serialize();
beat.set_uptimeSec(11);   // written into the same buffer, no need to call Serialize again
std::printf("Heartbeat: received uptime=%d, same buffer=%s\n",
    protaco::ToObject<demo::Heartbeat>(bytes).get_uptimeSec(), &bytes == &beat.Serialize() ? "yes" : "no");
```

<details>
<summary>C#</summary>

Not applicable. The C# program has no fixed-buffer step, and `Serialize()` returns a new array on each call.

</details>

<details>
<summary>Python</summary>

```python
beat = Heartbeat(node="oven-1", uptimeSec=10)
print("Heartbeat: received uptime=%d" % protaco.ToObject(Heartbeat, beat.Serialize()).uptimeSec)
```

</details>

C++ fixed-field setters write directly into the frame buffer. The earlier `bytes` reference sees the new value 11, and `Serialize()` returns the same buffer. Python handles the fixed block with one struct pack/unpack.

### 4.3 Extension data

**C++**

```cpp
demo::PizzaOrder order;
order.set_orderId("A-100");
demo::Topping basil;
basil.set_name("basil");
order.mutable_toppings().push_back(basil);
order.GetMutableExt().SetValue("secretSauce", "yes please");   // key not in the yaml
order.GetMutableExt().SetValue("pineapple", 0);

const demo::PizzaOrder got = protaco::ToObject<demo::PizzaOrder>(order.Serialize());
std::string sauce;
got.GetExt().TryGetValue("secretSauce", &sauce);
std::printf("PizzaOrder ext: secretSauce=%s, schema key in ext=%s\n",
    sauce.c_str(), order.GetMutableExt().SetValue("orderId", 9) ? "accepted (bug)" : "rejected");
```

<details>
<summary>C#</summary>

```csharp
PizzaOrder order = new PizzaOrder { OrderId = "A-100" };
order.Toppings.Add(new Topping { Name = "basil" });
order.Ext.SetValue("secretSauce", "yes please");

PizzaOrder got = Protaco.Codec.ToObject<PizzaOrder>(order.Serialize());
string sauce;
got.Ext.TryGetValue("secretSauce", out sauce);
Console.WriteLine("PizzaOrder ext: secretSauce=" + sauce + ", schema key in ext=" + (order.Ext.SetValue("orderId", 9) ? "accepted (bug)" : "rejected"));
```

</details>

<details>
<summary>Python</summary>

```python
order = PizzaOrder(orderId="A-100")
order.toppings.append(Topping(name="basil"))
order.ext.SetValue("secretSauce", "yes please")   # key not in the yaml
order.ext.SetValue("pineapple", 0, protaco.INT)   # an int needs its kind

got = protaco.ToObject(PizzaOrder, order.Serialize())
sauce = got.ext.TryGetValue("secretSauce", protaco.STRING)
print("PizzaOrder ext: secretSauce=%s, schema key in ext=%s"
      % (sauce.value, "accepted (bug)" if order.ext.SetValue("orderId", 9, protaco.INT) else "rejected"))
```

</details>

`SetValue` and `TryGetValue` write and read values absent from the YAML. Writing the schema key `orderId` into ext is rejected. C++ and C# use the value's type. Python integer values need an explicit kind such as `protaco.INT`.

### 4.4 Error handling

**C++**

```cpp
const std::vector<uint8_t> bytes = demo::Heartbeat().Serialize();
demo::DeliveryRoute wrong;
std::string error;
if (!protaco::TryToObject(bytes.data(), bytes.size(), &wrong, &error))
    std::printf("decode failed: %s\n", error.c_str());
```

<details>
<summary>C#</summary>

```csharp
byte[] bytes = new Heartbeat().Serialize();
DeliveryRoute wrong;
string error;
if (!Protaco.Codec.TryToObject(bytes, out wrong, out error))
    Console.WriteLine("decode failed: " + error);
```

</details>

<details>
<summary>Python</summary>

```python
ok, _, error = protaco.TryToObject(DeliveryRoute, Heartbeat().Serialize())
if not ok:
    print("decode failed: " + error)
```

</details>

`TryToObject` returns success and an error string instead of throwing. In C++, failure leaves the output object `wrong` unchanged. Python's `Result` can be unpacked as `ok, value, error`. This example fails the name check, so it has no field path. Field errors include a path such as `delivery.doorCode` or `sections[0].name`.

### 4.5 Text output

**C++**

```cpp
demo::DeliveryRoute route;
route.set_rider("rider-7");
demo::RouteStop stop;
stop.set_orderId("A-100");
stop.set_address("1 Pizza St");
route.mutable_stops().push_back(stop);
route.GetMutableExt().SetValue("priority", 5);
std::printf("%s\n", route.ToString().c_str());
std::printf("%s\n", route.ToString(protaco::TextStyle::Pretty()).c_str());
```

<details>
<summary>C#</summary>

```csharp
DeliveryRoute route = new DeliveryRoute { Rider = "rider-7" };
route.Stops.Add(new RouteStop { OrderId = "A-100", Address = "1 Pizza St" });
route.Ext.SetValue("priority", 5);
Console.WriteLine(route.ToString());
Console.WriteLine(route.ToString(Protaco.TextStyle.Pretty()));
```

</details>

<details>
<summary>Python</summary>

```python
route = DeliveryRoute(rider="rider-7")
route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St"))
route.ext.SetValue("priority", 5, protaco.INT)
print(route)
print(route.ToString(protaco.TextStyle.Pretty()))
```

</details>

The default output fits on one line. `Pretty()` shows the envelope and fields on separate lines. Since `etaMin` is not assigned, the output shows its default, 15. Extension entries appear under `ext{...}`.

### 4.6 Enums, maps, and recursion

**C++**

```cpp
demo::Menu menu;
menu.set_store("NAPOLI-1");
menu.mutable_prices()["margherita"] = 9.5;
menu.mutable_prices()["marinara"] = 8.0;
demo::Category pizza;
pizza.set_name("pizza");
pizza.mutable_dishes() = { "margherita", "marinara" };
demo::Category white;
white.set_name("white");
white.mutable_dishes().push_back("bianca");
pizza.mutable_subsections().push_back(white);
menu.mutable_sections().push_back(pizza);

const demo::Menu got = protaco::ToObject<demo::Menu>(menu.Serialize());
std::printf("Menu: default size %s, margherita %.1f\n", demo::EnumName(got.get_defaultSize()), got.get_prices().at("margherita"));
std::printf("%s\n", got.ToString().c_str());
```

<details>
<summary>C#</summary>

```csharp
Menu menu = new Menu { Store = "NAPOLI-1" };
menu.Prices["margherita"] = 9.5;
menu.Prices["marinara"] = 8.0;
Category pizza = new Category { Name = "pizza" };
pizza.Dishes.AddRange(new[] { "margherita", "marinara" });
Category white = new Category { Name = "white" };
white.Dishes.Add("bianca");
pizza.Subsections.Add(white);
menu.Sections.Add(pizza);

Menu got = Protaco.Codec.ToObject<Menu>(menu.Serialize());
Console.WriteLine("Menu: default size " + PizzaSizeNames.Name(got.DefaultSize) + ", margherita " + got.Prices["margherita"]);
Console.WriteLine(got.ToString());
```

</details>

<details>
<summary>Python</summary>

```python
menu = Menu(store="NAPOLI-1", prices={"margherita": 9.5, "marinara": 8.0})
pizza = Category(name="pizza", dishes=["margherita", "marinara"])
pizza.subsections.append(Category(name="white", dishes=["bianca"]))
menu.sections.append(pizza)

got = protaco.ToObject(Menu, menu.Serialize())
print("Menu: default size %s, margherita %.1f" % (PizzaSizeNames.Name(got.defaultSize), got.prices["margherita"]))
print(got)
```

</details>

`Menu.defaultSize` defaults to `PizzaSize.Large`. The price map and recursive `Category` list are serialized together. `ToString()` prints enums as `Large(3)` and sorts map keys. Serialization follows container iteration order, so equivalent maps can have different byte ordering across languages or implementations.

### 4.7 Zero-copy

**C++**

```cpp
demo::Heartbeat beat;
beat.set_node("oven-1");
beat.set_uptimeSec(42);
const std::vector<uint8_t> received = beat.Serialize();
demo::Heartbeat::View view;   // valid while received lives
if (protaco::TryView<demo::Heartbeat>(received, &view))
    std::printf("Heartbeat view: node=%s uptime=%d\n", view.get_node().c_str(), view.get_uptimeSec());

std::vector<uint8_t> bytes = demo::DeliveryRoute().Serialize();
const uint8_t* data = bytes.data();
const demo::DeliveryRoute route = protaco::ToObject<demo::DeliveryRoute>(std::move(bytes));
std::printf("DeliveryRoute adopted the buffer: %s\n", route.Serialize().data() == data ? "yes" : "no");
```

<details>
<summary>C#</summary>

C# uses View only; it has no C++ buffer-ownership transfer step.

```csharp
byte[] received = new Heartbeat { Node = "oven-1", UptimeSec = 42 }.Serialize();
Heartbeat.View view;   // valid while received is unchanged
if (Heartbeat.TryView(received, out view))
    Console.WriteLine("Heartbeat view: node=" + view.Node + " uptime=" + view.UptimeSec);
```

</details>

<details>
<summary>Python</summary>

Not applicable. Python has neither View nor the C++ buffer-ownership transfer API.

</details>

`TryView` validates a frame containing only fixed fields and reads fields from the received bytes. A C++ View remains valid while the bytes are alive and unchanged; a C# View remains valid while its array is unchanged. C++ `ToObject(std::move(bytes))` takes ownership of the buffer when its supported version range and fingerprints match the receiving type's layout. `Serialize()` reuses that buffer until variable fields or ext change.

### 4.8 Envelope

**C++**

```cpp
demo::DeliveryRoute request;
demo::DeliveryRoute reply;
reply.SetCorr(request.GetId());
std::string date;
std::string time;
if (reply.TryGetTimeFromTimestamp(&date, &time))
    std::printf("Envelope: request id=%u, reply id=%u corr=%u at %s %s\n", request.GetId(), reply.GetId(), reply.GetCorr(),
        date.c_str(), time.c_str());
```

<details>
<summary>C#</summary>

```csharp
DeliveryRoute request = new DeliveryRoute();
DeliveryRoute reply = new DeliveryRoute();
reply.SetCorr(request.GetId());
string date;
string time;
if (reply.TryGetTimeFromTimestamp(out date, out time))
    Console.WriteLine("Envelope: request id=" + request.GetId() + ", reply id=" + reply.GetId() + " corr=" + reply.GetCorr() + " at " + date + " " + time);
```

</details>

<details>
<summary>Python</summary>

```python
request = DeliveryRoute()
reply = DeliveryRoute()
reply.SetCorr(request.GetId())
when = reply.TryGetTimeFromTimestamp()
if when:
    date, time_text = when.value
    print("Envelope: request id=%d, reply id=%d corr=%d at %s %s" % (request.GetId(), reply.GetId(), reply.GetCorr(), date, time_text))
```

</details>

A new message automatically gets an ID and the current timestamp in UTC microseconds. Set the reply's `corr` to the request ID. `TryGetTimeFromTimestamp` converts the timestamp to local date and time strings. Decoding and nested-field initialization do not consume new IDs.

### 4.9 Shared memory: arrays and fixed messages

**C++**

```cpp
std::vector<uint8_t> region(demo::Board::MutableView::FrameSize());
const demo::Board fresh;   // fill the region once with a new frame
std::memcpy(region.data(), fresh.Serialize().data(), region.size());

demo::Board::MutableView board;   // valid while the region lives
std::string error;
if (!protaco::TryMutableView<demo::Board>(region.data(), region.size(), &board, &error))
{
    std::printf("Board: %s\n", error.c_str());
    return;
}
board.mutable_depot().set_label("depot");
demo::Spot::Ref spot = board.mutable_spots(0);   // written in place, nothing copied
spot.set_lat(37.5665);
spot.set_lon(126.978);
spot.set_label("north gate");
board.set_ovens(1, 230.0);
board.set_queue(0, demo::PizzaSize::Large);
board.set_busy(2, true);
if (!board.set_spots(demo::Limits::MaxSpots, demo::Spot(), &error))
    std::printf("Board: %s\n", error.c_str());

demo::Board::View view;   // another reader of the same region
if (protaco::TryView<demo::Board>(region, &view))
    std::printf("Board: %d spots, first %s at %.4f, %u bytes in the region\n", demo::Limits::MaxSpots,
        view.get_spots(0).get_label().c_str(), view.get_spots(0).get_lat(), static_cast<unsigned>(region.size()));
std::printf("%s\n", protaco::ToObject<demo::Board>(region).ToString().c_str());
```

<details>
<summary>C#</summary>

```csharp
Board board = new Board();
board.Depot.Label = "depot";
board.Spots[0].Lat = 37.5665;
board.Spots[0].Lon = 126.978;
board.Spots[0].Label = "north gate";
board.Ovens[1] = 230.0;
board.Queue[0] = PizzaSize.Large;
board.Busy[2] = true;
byte[] bytes = board.Serialize();

Board.View view;   // valid while bytes is unchanged
if (Board.TryView(bytes, out view))
    Console.WriteLine("Board: " + Limits.MaxSpots + " spots, first " + view.Spots(0).Label + " at " + view.Spots(0).Lat.ToString("F4")
        + ", " + bytes.Length + " bytes");
Console.WriteLine(Protaco.Codec.ToObject<Board>(bytes).ToString());
```

</details>

<details>
<summary>Python</summary>

```python
board = Board()
board.depot.label = "depot"
spot = board.spots[0]
spot.lat = 37.5665
spot.lon = 126.978
spot.label = "north gate"
board.ovens[1] = 230.0
board.queue[0] = PizzaSize.Large
board.busy[2] = True
data = board.Serialize()

got = protaco.ToObject(Board, data)
print("Board: %d spots, first %s at %.4f, %d bytes" % (Limits.MaxSpots, got.spots[0].label, got.spots[0].lat, len(data)))
print(got)
```

</details>

`Board` has only fixed fields, so a frame of it has a fixed size, `FrameSize()`. The C++ step fills a region once with a new frame, then `TryMutableView` writes through it in place: `mutable_spots(0)` returns a `Spot::Ref` into the region, and `set_spots` rejects index 4 because `Limits.MaxSpots` is 4. A `View` over the same region sees the change at once. C# and Python fill a `Board` object and serialize it; C# reads it back in place with `View`. All three print the same `Board{...}` text and 245 bytes.

## 5. Generated code

These are selected excerpts from the actual files in [taco_generated/](taco_generated/). Comments mark omitted members.

### 5.1 C++

The fixed-string setter in [RouteStop.g.h](taco_generated/cpp/RouteStop.g.h) checks the UTF-8 byte limit: 16 bytes for `orderId` and 48 for `address`. The integer setter writes directly to a fixed position in the buffer.

**C++**

```cpp
std::string get_orderId() const
{
    return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 16);
}

bool set_orderId(const std::string& value, std::string* error = nullptr)
{
    return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 16, value, "orderId", error);
}

int32_t get_etaMin() const
{
    return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 64);
}

void set_etaMin(int32_t value)
{
    protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, value);
}
```

This excerpt from [Heartbeat.g.h](taco_generated/cpp/Heartbeat.g.h) shows `View` with the other field accessors omitted.

**C++**

```cpp
class View : public protaco::CView
{
public:
    std::string get_node() const
    {
        return protaco::detail::LoadChars(Fixed() + 0, 32);
    }

    // ...

    int32_t get_uptimeSec() const
    {
        return protaco::detail::Load<int32_t>(Fixed() + 49);
    }
};
```

The enum and name conversion come from [PizzaSize.enum.h](taco_generated/cpp/PizzaSize.enum.h). Undeclared numbers return `nullptr`.

**C++**

```cpp
enum class PizzaSize : int32_t
{
    /// 10 inch
    Small = 1,
    /// 12 inch
    Medium = 2,
    /// 14 inch
    Large = 3,
};

/// Name of a value, removed values included. nullptr for a number never declared.
inline const char* EnumName(PizzaSize value)
{
    switch (static_cast<int32_t>(value))
    {
    case 1:
        return "Small";
    case 2:
        return "Medium";
    case 3:
        return "Large";
    default:
        return nullptr;
    }
}
```

These map accessors come from [Menu.g.h](taco_generated/cpp/Menu.g.h). Exposing a mutable reference records that later changes cannot be tracked.

**C++**

```cpp
const std::unordered_map<std::string, double>& get_prices() const
{
    return prices_;
}

std::unordered_map<std::string, double>& mutable_prices()
{
    cache_.exposed = true;
    return prices_;
}

void set_prices(const std::unordered_map<std::string, double>& value)
{
    prices_ = value;
    cache_.valid = false;
}
```

### 5.2 C#

<details>
<summary>C#</summary>

These properties and the serialization method come from [Menu.g.cs](taco_generated/cs/Menu.g.cs). C# uses PascalCase properties and serializes the object's envelope.

```csharp
public PizzaSize DefaultSize { get; set; } = PizzaSize.Large;

public Dictionary<string, double> Prices { get; set; } = new Dictionary<string, double>();

public List<Category> Sections { get; set; } = new List<Category>();

public byte[] Serialize()
{
    return Protaco.Codec.Serialize(this, GetEnvelope());
}
```

</details>

### 5.3 Python

<details>
<summary>Python</summary>

This is part of the class declaration and the `_Bind` call from [Menu_g.py](taco_generated/py/demo/Menu_g.py). The constructor body is omitted. `_Bind` registers the generated class's fixed and variable field layout with the runtime. Add `examples/taco_generated/py` to `sys.path` to import the `demo` package.

```python
class Menu(protaco.Message):
    """A store menu: an enum field, a map and a tree of sections.
    [version 1; schema 0x1A3924BC; source Menu.yaml]
    """

    __slots__ = ("store", "defaultSize", "prices", "sections")
    MsgName = "Menu"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x1A3924BC

    # ...

Menu._Bind(
    hashes=(0x1A3924BC,),
    allow_extension=True,
    schema_keys=("store", "defaultSize", "prices", "sections"),
    fixed=(
        ("store", "store", protaco.Items.Chars(16), None),
        ("defaultSize", "defaultSize", protaco.Items.Enum(PizzaSize, PizzaSizeNames.Name), None),
    ),
    var=(
        ("prices", "prices", protaco.Items.Map(protaco.Items.String, protaco.Items.Double), False, False),
        ("sections", "sections", protaco.Items.List(protaco.Items.Message(Category)), False, False),
    ),
    text=("store", "defaultSize", "prices", "sections"),
)
```

</details>

## 6. Version management walkthrough

[build1](../protaco_compiler/tests/schemas/build1), [build2](../protaco_compiler/tests/schemas/build2), [build3](../protaco_compiler/tests/schemas/build3), and [build4](../protaco_compiler/tests/schemas/build4) contain successive shipped revisions of the same `Order.yaml`. Tests use separate namespaces so the revisions can coexist in one binary. An application keeps one namespace and revises the file.

### 6.1 Revisions

The [version 1 definition](../protaco_compiler/tests/schemas/build1/Order.yaml), with its introductory comment omitted:

**YAML**

```yaml
message: Order
version: 1
namespace: build1
fields:
  - {key: id, type: string(8)}
  - {key: qty, type: int, default: 1}
  - {key: note, type: string}
  - {key: color, type: string, required: true}
```

The [version 3 definition](../protaco_compiler/tests/schemas/build3/Order.yaml):

**YAML**

```yaml
message: Order
version: 3
min_version: 1
namespace: build3
fields:
  - {key: id, type: string(8)}
  - {key: weight, type: double, default: 0.5, added: 2}
  - {key: qty, type: int, default: 1, removed: 3}
  - {key: note, type: string, removed: 3}
  - {key: color, type: string, required: true}
  - {key: tags, type: list<string>, added: 2}
  - {key: label, type: string, added: 3}
  - {key: rush, type: bool, added: 3}
```

- Version 2 adds fixed `weight` and variable `tags`. Although declared between `id` and `qty`, `weight` is placed after the existing fixed block.
- Version 3 removes `qty` and `note`, then adds `label` and `rush`. Both removed lines remain because `min_version` is 1.
- Version 4 removes the required `color` field and sets `min_version: 2`, ending support for version 1.

### 6.2 Generated classes

| Build | C++ classes | Supported versions |
|---|---|---|
| build1 | `build1::Order` (v1) | 1 |
| build2 | `build2::Order` (v2), `build2::v1::Order` | 1..2 |
| build3 | `build3::Order` (v3), `build3::v1::Order`, `build3::v2::Order` | 1..3 |
| build4 | `build4::Order` (v4), `build4::v2::Order`, `build4::v3::Order` | 2..4 |

An older-version class has the same fingerprint as the class originally shipped at that version. For example, `build3::v1::Order::MsgSchemaHash()` equals `build1::Order::MsgSchemaHash()`.

The corresponding C# names are `build3.Order` and `build3.v1.Order`. In Python, add `protaco_compiler/tests/taco_generated/py` to the module path and use `Order` and `v1.Order` from the same module.

<details>
<summary>Python</summary>

```python
from build3.Order_g import Order, v1

latest = Order()
old = v1.Order()
```

</details>

### 6.3 Fixed block layout

| Version | Fixed fields in order |
|---|---|
| 1 | `id`, `qty` |
| 2 | `id`, `qty`, `weight` |
| 3, 4 | `id`, `qty` (removed, bytes kept), `weight`, `rush` |

Each fixed block is a prefix of later versions' blocks. An older receiver reads only its own version's portion. The removed `qty` bytes contain its default, 1.

### 6.4 Exchanges between builds

A newer frame reaches an older receiver. In `NewerFrameReadAsOwnVersion`, build3 sends a frame and build1 reads the version 1 layout. `qty` is the default 1, and `note` is empty. New variable fields `tags` and `label` remain in ext. build2 reads its own `weight` and `tags` fields.

An older frame reaches a newer receiver. In `OlderFrameNeedsOldType`, reading build1's frame through `build3::Order` gives:

```text
message version too old: Order reads version 3 or newer, got 1. Use the v1 type
```

Inspect version 1 with `PeekHeader()`, then decode through `build3::v1::Order`.

The sender has ended support for an older version. In `SenderMinVersionAboveReader`, build1 receives build4's frame and reports:

```text
message version too new: Order reads version 1, sender supports 2..4
```

build2 is within the supported range, but version 4 removed its required `color` field, so it rejects the frame with:

```text
color: missing required (frame version 4 read as 2)
```

For that peer, send a `build4::v3::Order` or `build4::v2::Order` with `color` populated. The [README](../README.md) also explains the sending and receiving rules.
