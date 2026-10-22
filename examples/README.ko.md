# Protaco 예제

[English](README.md) · [README](../README.ko.md) · [상세 참조](../docs/reference.ko.md)

이 폴더에는 protaco 0.7.1의 YAML 스키마, 생성 코드, C++·C#·Python 예제 프로그램이 있습니다. 피자 주문과 배달 경로로 API를 설명하고, 마지막 장에서 `Order`의 개정 이력으로 버전 관리를 살펴봅니다.

## 목차

1. [폴더 구성](#1-폴더-구성)
2. [빌드와 실행](#2-빌드와-실행)
3. [스키마](#3-스키마)
4. [프로그램 설명](#4-프로그램-설명)
5. [생성 코드](#5-생성-코드)
6. [버전 관리 예제](#6-버전-관리-예제)

## 1. 폴더 구성

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

`schemas/`의 namespace는 `demo`입니다. 생성 파일에는 각 언어의 런타임도 들어 있습니다. [examples.sln](examples.sln)은 C++과 .NET Framework C# 예제를 묶습니다. .NET 8 C# 프로젝트와 Python 프로그램은 별도로 실행합니다. 루트의 빌드 스크립트는 세 언어 코드를 다시 생성한 뒤 예제를 빌드하고 실행합니다.

## 2. 빌드와 실행

### Windows

저장소 루트에서 실행합니다. Windows는 Visual Studio 2017 이상과 v141 C++ 도구, .NET Framework 4.6.1, Python 3.8 이상이 필요합니다. [build.cmd](../build.cmd)의 전체 6단계 중 2단계에서 예제 코드를 생성하고 6단계에서 세 언어 예제를 빌드·실행합니다.

**Windows cmd**

```bat
build.cmd
```

수동으로 실행할 때는 MSBuild가 경로에 있는 Visual Studio 개발자 명령 프롬프트를 사용합니다.

**Windows cmd**

```bat
msbuild protaco_compiler\protacoc\protacoc.csproj /p:Configuration=Release
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\taco_generated\cpp --cs_out examples\taco_generated\cs --py_out examples\taco_generated\py
msbuild examples\examples.sln /p:Configuration=Release /p:Platform=x64
bin\Release\protaco_example.exe
bin\Release\protaco_example_cs.exe
python examples\python_examples\main.py
```

[example.vcxproj](cpp_examples/example.vcxproj)의 include 경로는 `$(ProjectDir)..\taco_generated\cpp`입니다. Pre-Build Event는 `bin\Release\protacoc.exe`가 있으면 사용하고, 없으면 `bin\Debug\protacoc.exe`를 사용해 C++과 C# 코드를 생성합니다. Python 코드는 위의 `--py_out` 명령이나 전체 빌드 스크립트로 생성합니다. 프로젝트 설정은 v141 x64, C++17, `/W4 /WX`, `/utf-8`이며 별도 런타임 라이브러리를 링크하지 않습니다. 생성 헤더의 C++14 호환성은 [Cpp14Check.cpp](../protaco_compiler/tests/cpp_tests/Cpp14Check.cpp)에서 검사합니다.

### Linux

Linux에서는 .NET SDK 8 이상, g++, Python 3.8 이상을 사용합니다. [build.sh](../build.sh)도 같은 6단계를 실행합니다.

**sh**

```sh
sh build.sh
```

예제만 수동으로 빌드·실행하는 명령입니다.

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

Windows 스크립트는 `MSBUILD`·`PYTHON`, Linux 스크립트는 `DOTNET`·`CXX`·`PYTHON` 환경 변수를 받습니다. .NET이 시스템 전체에 설치되어 있지 않으면 `DOTNET=~/.dotnet/dotnet sh build.sh`처럼 실행합니다. 수동 실행에서는 .NET 앱 호스트가 런타임을 찾을 수 있도록 `DOTNET_ROOT`도 설치 경로로 설정합니다.

## 3. 스키마

| 스키마 | 보여 주는 기능 | 쓰는 곳 |
|---|---|---|
| [RouteStop](schemas/RouteStop.yaml) | 고정 문자열, 정수 기본값 | `DeliveryRoute.stops` 원소 |
| [DeliveryRoute](schemas/DeliveryRoute.yaml) | 가변 문자열, required, 중첩 메시지 목록 | 송수신·오류·로그·Envelope |
| [Heartbeat](schemas/Heartbeat.yaml) | 고정 필드, `allow_extension: false` | 고정 버퍼·View |
| [PizzaOrder](schemas/PizzaOrder.yaml) | 기본값, 목록, 중첩 메시지 | 확장 데이터 |
| [Topping](schemas/Topping.yaml) | 작은 고정 필드 메시지 | `PizzaOrder.toppings` 원소 |
| [Delivery](schemas/Delivery.yaml) | `list<int>`, `list<string>` | `PizzaOrder.delivery` |
| [DeliveryProof](schemas/DeliveryProof.yaml) | `bytes`, `list<bytes>`, required bytes | 바이너리 전달용 스키마 |
| [PizzaSize.enum](schemas/PizzaSize.enum.yaml) | enum 값과 이름 | `Menu.defaultSize` |
| [Menu](schemas/Menu.yaml) | enum, `map<string, double>`, 메시지 목록 | 매장 메뉴 |
| [Category](schemas/Category.yaml) | `list<Category>` 자기 참조 | 메뉴의 하위 섹션 |
| [Limits.const](schemas/Limits.const.yaml) | 코드와 배열 길이에 쓰는 이름 붙은 상수 | `Board` 배열 길이 |
| [Spot](schemas/Spot.yaml) | `fixed: true` 메시지 | `Board.depot`, `Board.spots` |
| [Board](schemas/Board.yaml) | 고정 길이 배열, fixed 메시지, fixed 전용 프레임 | MutableView로 다루는 shared memory |

다음은 실제 스키마 발췌입니다. 맨 위 설명 주석은 생략했습니다. `Heartbeat`는 예제에서 피자 오븐 `oven-1`의 상태를 전달합니다.

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

`map<K, V>`는 쉼표를 포함하므로 YAML의 `{...}` 안에서는 따옴표로 감쌉니다. `Category.subsections`는 자기 자신을 담는 목록입니다. `PizzaOrder`의 `woodFired`와 `deepDishCm`은 매장별 용도를 주석으로 설명하며, 주석은 직렬화 동작을 바꾸지 않습니다. `Board`는 고정 길이입니다. 배열과 `Spot` 필드가 모두 fixed 영역에 있으므로 보드 전체를 shared memory에 둘 수 있습니다.

## 4. 프로그램 설명

[C++](cpp_examples/main.cpp)은 아래 9단계를 실행합니다. [C#](cs_examples/Program.cs)은 고정 버퍼 단계가 없는 8단계이고, [Python](python_examples/main.py)은 zero-copy 단계가 없는 8단계입니다. 다음은 C++의 실제 실행 출력입니다. Envelope 시각은 실행마다 달라지고, id는 예제의 메시지 생성 순서로 정해집니다.

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

C# 출력에는 `Heartbeat: received uptime=11, same buffer=yes`와 `DeliveryRoute adopted the buffer: yes` 줄이 없습니다. Python은 고정 필드 단계에서 `Heartbeat: received uptime=10`을 출력하며, `Heartbeat view: ...`와 `DeliveryRoute adopted the buffer: ...` 줄이 없습니다. 마지막 단계에서 C#과 Python은 shared memory 대신 `Board` 객체를 채우며, 범위 밖 줄 없이 `Board: 4 spots, first north gate at 37.5665, 245 bytes`와 같은 `Board{...}` 텍스트를 출력합니다. 아래 코드는 각 프로그램의 함수 본문에서 발췌했습니다. include·using·import는 원본 프로그램을 따릅니다.

### 4.1 송신과 수신

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

C++ `Serialize()`는 객체의 프레임 버퍼 참조를 반환합니다. 전송 수단에 `bytes.data()`와 `bytes.size()`를 넘깁니다. `ToObject<T>`는 이름·version·fingerprint를 검사하고 실패하면 `protaco::CParseError`를 던집니다. `mutable_stops()`를 호출한 객체는 이후 수정을 추적할 수 없으므로 직렬화마다 가변 블록을 다시 만듭니다. C# `Serialize()`는 매번 새 배열을, Python은 `bytes`를 반환합니다.

### 4.2 고정 필드만 있는 메시지

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

해당 없음. C# 프로그램에는 고정 버퍼 단계가 없으며, `Serialize()`는 매번 새 배열을 만듭니다.

</details>

<details>
<summary>Python</summary>

```python
beat = Heartbeat(node="oven-1", uptimeSec=10)
print("Heartbeat: received uptime=%d" % protaco.ToObject(Heartbeat, beat.Serialize()).uptimeSec)
```

</details>

C++의 고정 필드 setter는 프레임 버퍼를 직접 바꿉니다. 앞서 받은 `bytes`에도 11이 반영되고 `Serialize()`는 같은 버퍼를 반환합니다. Python은 고정 영역을 한 번의 struct pack/unpack으로 처리합니다.

### 4.3 확장 데이터

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

`SetValue`와 `TryGetValue`로 YAML에 없는 값을 쓰고 읽습니다. 스키마 키 `orderId`를 ext에 쓰려는 호출은 거부됩니다. C++과 C#은 값 타입을 사용하고, Python의 정수 값에는 `protaco.INT`처럼 타입을 지정해야 합니다.

### 4.4 오류 처리

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

`TryToObject`는 예외 대신 성공 여부와 오류 문자열을 반환합니다. C++은 실패하면 출력 객체 `wrong`을 유지합니다. Python의 `Result`는 `ok, value, error`로 풀 수 있습니다. 이 예제는 이름 검사에서 실패하므로 필드 경로가 없습니다. 필드 오류에는 `delivery.doorCode`, `sections[0].name` 같은 경로가 붙습니다.

### 4.5 텍스트 출력

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

기본 출력은 한 줄이며 `Pretty()`는 Envelope와 필드를 줄별로 보여 줍니다. `etaMin`을 설정하지 않아 기본값 15가 출력됩니다. 확장 항목은 `ext{...}`에 모입니다.

### 4.6 enum·map·재귀

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

`Menu.defaultSize`의 기본값은 `PizzaSize.Large`입니다. 가격 map과 재귀적인 `Category` 목록을 함께 직렬화합니다. `ToString()`은 enum을 `Large(3)`으로 표시하고 map 키를 정렬합니다. map의 직렬화 순서는 컨테이너 순회 순서이므로 같은 내용도 언어·구현에 따라 바이트 순서가 다를 수 있습니다.

### 4.7 zero-copy

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

C#은 View만 사용하며 C++식 버퍼 소유권 이전 단계는 없습니다.

```csharp
byte[] received = new Heartbeat { Node = "oven-1", UptimeSec = 42 }.Serialize();
Heartbeat.View view;   // valid while received is unchanged
if (Heartbeat.TryView(received, out view))
    Console.WriteLine("Heartbeat view: node=" + view.Node + " uptime=" + view.UptimeSec);
```

</details>

<details>
<summary>Python</summary>

해당 없음. Python에는 View와 C++식 버퍼 소유권 이전 API가 없습니다.

</details>

`TryView`는 고정 필드만 있는 메시지의 프레임을 검사한 뒤 받은 바이트에서 필드를 읽습니다. C++은 원본 바이트가 살아 있고 바뀌지 않는 동안, C#은 배열이 바뀌지 않는 동안 View가 유효합니다. C++의 `ToObject(std::move(bytes))`는 지원 version 범위와 fingerprint가 수신 타입의 layout과 일치할 때 버퍼 소유권을 가져갑니다. 이후 가변 필드나 ext를 바꾸기 전까지 `Serialize()`가 그 버퍼를 재사용합니다.

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

새 메시지는 id와 현재 UTC 마이크로초 타임스탬프를 자동으로 받습니다. 응답의 `corr`는 요청의 id로 설정합니다. `TryGetTimeFromTimestamp`는 로컬 날짜·시각 문자열로 변환합니다. 디코딩과 중첩 필드 초기화는 새 id를 소비하지 않습니다.

### 4.9 shared memory: 배열과 fixed 메시지

**C++**

```cpp
std::vector<uint8_t> region(demo::Board::MutableView::FrameSize());
const demo::Board fresh;   // 새 프레임으로 영역을 한 번 채움
std::memcpy(region.data(), fresh.Serialize().data(), region.size());

demo::Board::MutableView board;   // 영역이 살아 있는 동안 유효
std::string error;
if (!protaco::TryMutableView<demo::Board>(region.data(), region.size(), &board, &error))
{
    std::printf("Board: %s\n", error.c_str());
    return;
}
board.mutable_depot().set_label("depot");
demo::Spot::Ref spot = board.mutable_spots(0);   // 제자리에 기록, 복사 없음
spot.set_lat(37.5665);
spot.set_lon(126.978);
spot.set_label("north gate");
board.set_ovens(1, 230.0);
board.set_queue(0, demo::PizzaSize::Large);
board.set_busy(2, true);
if (!board.set_spots(demo::Limits::MaxSpots, demo::Spot(), &error))
    std::printf("Board: %s\n", error.c_str());

demo::Board::View view;   // 같은 영역의 다른 reader
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

Board.View view;   // bytes가 바뀌지 않는 동안 유효
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

`Board`는 fixed 필드만 가지므로 프레임 크기가 `FrameSize()`로 고정됩니다. C++ 단계는 새 프레임으로 영역을 한 번 채운 뒤 `TryMutableView`로 제자리에서 씁니다. `mutable_spots(0)`은 영역 안을 가리키는 `Spot::Ref`를 돌려주고, `Limits.MaxSpots`가 4이므로 `set_spots`는 index 4를 거부합니다. 같은 영역 위의 `View`는 바뀐 값을 바로 봅니다. C#과 Python은 `Board` 객체를 채워 직렬화하고, C#은 `View`로 제자리에서 다시 읽습니다. 세 언어 모두 같은 `Board{...}` 텍스트와 245바이트를 출력합니다.

## 5. 생성 코드

아래는 [taco_generated/](taco_generated/)의 실제 생성 코드에서 필요한 부분만 발췌한 것입니다. 생략한 멤버는 주석으로 표시했습니다.

### 5.1 C++

[RouteStop.g.h](taco_generated/cpp/RouteStop.g.h)의 고정 문자열 setter는 UTF-8 바이트 limit을 검사합니다. `orderId`는 16바이트, `address`는 48바이트입니다. 정수 setter는 버퍼의 고정 위치에 기록합니다.

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

[Heartbeat.g.h](taco_generated/cpp/Heartbeat.g.h)의 `View` 발췌입니다. 나머지 필드 접근자는 생략했습니다.

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

[PizzaSize.enum.h](taco_generated/cpp/PizzaSize.enum.h)의 enum과 이름 변환입니다. 선언되지 않은 번호는 `nullptr`를 반환합니다.

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

[Menu.g.h](taco_generated/cpp/Menu.g.h)의 map 접근자입니다. mutable 참조를 노출하면 이후 변경을 추적할 수 없다는 표시를 남깁니다.

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

[Menu.g.cs](taco_generated/cs/Menu.g.cs)의 속성과 직렬화 메서드입니다. C#은 PascalCase 속성을 사용하고 객체의 Envelope를 직렬화합니다.

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

[Menu_g.py](taco_generated/py/demo/Menu_g.py)의 클래스 선언 일부와 `_Bind` 호출입니다. 생성자 본문은 생략했습니다. `_Bind`는 생성 클래스의 고정·가변 필드 layout을 런타임에 등록합니다. `examples/taco_generated/py`를 `sys.path`에 넣으면 `demo` 패키지를 import할 수 있습니다.

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

## 6. 버전 관리 예제

[build1](../protaco_compiler/tests/schemas/build1), [build2](../protaco_compiler/tests/schemas/build2), [build3](../protaco_compiler/tests/schemas/build3), [build4](../protaco_compiler/tests/schemas/build4)는 같은 `Order.yaml`의 배포 이력입니다. 시험에서는 여러 개정을 한 바이너리에 넣기 위해 namespace를 나눕니다. 사용자 프로젝트에서는 하나의 namespace를 유지하며 파일 내용을 개정합니다.

### 6.1 개정 내용

[version 1](../protaco_compiler/tests/schemas/build1/Order.yaml)의 정의입니다. 맨 위 설명 주석은 생략했습니다.

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

[version 3](../protaco_compiler/tests/schemas/build3/Order.yaml)의 정의입니다.

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

- version 2: 고정 필드 `weight`와 가변 필드 `tags` 추가. `weight`는 `id`와 `qty` 사이에 선언해도 기존 고정 영역 뒤에 layout.
- version 3: `qty`, `note` 제거, `label`, `rush` 추가. `min_version: 1`이므로 제거한 두 줄 유지.
- version 4: 필수였던 `color` 제거, `min_version: 2`로 version 1 지원 종료.

### 6.2 생성 클래스

| 빌드 | C++ 클래스 | 지원 version |
|---|---|---|
| build1 | `build1::Order` (v1) | 1 |
| build2 | `build2::Order` (v2), `build2::v1::Order` | 1..2 |
| build3 | `build3::Order` (v3), `build3::v1::Order`, `build3::v2::Order` | 1..3 |
| build4 | `build4::Order` (v4), `build4::v2::Order`, `build4::v3::Order` | 2..4 |

옛 version 클래스의 fingerprint는 그 version을 배포한 클래스와 같습니다. 예를 들어 `build3::v1::Order::MsgSchemaHash()`와 `build1::Order::MsgSchemaHash()`는 같은 값입니다.

C#의 대응 이름은 `build3.Order`, `build3.v1.Order`입니다. Python은 `protaco_compiler/tests/taco_generated/py`를 모듈 경로에 넣고 같은 모듈의 `Order`와 `v1.Order`를 사용합니다.

<details>
<summary>Python</summary>

```python
from build3.Order_g import Order, v1

latest = Order()
old = v1.Order()
```

</details>

### 6.3 고정 영역 layout

| version | 고정 필드 순서 |
|---|---|
| 1 | `id`, `qty` |
| 2 | `id`, `qty`, `weight` |
| 3, 4 | `id`, `qty`(제거, 바이트 유지), `weight`, `rush` |

각 고정 영역은 이후 version 고정 영역의 앞부분입니다. 옛 수신 측은 자기 version의 부분만 읽습니다. 제거된 `qty`의 바이트에는 기본값 1을 기록합니다.

### 6.4 빌드 간 교환

새 프레임을 옛 수신 측이 받는 경우입니다. `NewerFrameReadAsOwnVersion` 시험에서 build3이 보내면 build1은 version 1 layout으로 읽고, `qty`는 기본값 1, `note`는 빈 문자열이 됩니다. 새 가변 필드 `tags`와 `label`은 ext에 남습니다. build2는 자기 필드인 `weight`와 `tags`를 읽습니다.

옛 프레임을 새 수신 측이 받는 경우입니다. `OlderFrameNeedsOldType` 시험에서 build1의 프레임을 `build3::Order`로 읽으면 다음 오류입니다.

```text
message version too old: Order reads version 3 or newer, got 1. Use the v1 type
```

`PeekHeader()`로 version 1임을 확인하고 `build3::v1::Order`로 디코딩합니다.

송신 측이 옛 version 지원을 종료한 경우입니다. `SenderMinVersionAboveReader` 시험에서 build4의 프레임을 build1이 받으면 다음 오류입니다.

```text
message version too new: Order reads version 1, sender supports 2..4
```

build2는 지원 범위 안이지만 필수 필드 `color`가 version 4에서 제거되어 다음 오류로 거부합니다.

```text
color: missing required (frame version 4 read as 2)
```

이 상대에게는 `color`를 채운 `build4::v3::Order`나 `build4::v2::Order`로 보냅니다. 송수신 규칙은 [README](../README.ko.md)에서도 설명합니다.
