# Protaco 상세 참조

[English](reference.md) · [README](../README.ko.md) · [예제](../examples/README.ko.md)

protacoc 0.6.0과 직렬화 형식 v4의 상세 참조입니다. 스키마 작성과 직렬화 형식의 전체 구조는 [README](../README.ko.md), 실행 가능한 프로그램은 [예제](../examples/README.ko.md)에 있습니다.

1. [스키마 지문](#1-스키마-지문)
2. [명령행](#2-명령행)
3. [진단](#3-진단)
   - [생성기 오류 코드](#31-생성기-오류-코드)
   - [디코딩 오류](#32-디코딩-오류)
   - [수신 판정 순서](#33-수신-판정-순서)
4. [C++ API](#4-c-api)
5. [C# API](#5-c-api)
6. [Python API](#6-python-api)
7. [빌드와 시험](#7-빌드와-시험)

## 1. 스키마 지문

지문은 메시지의 한 version을 나타내는 정규 텍스트의 UTF-8 바이트에 대한 32비트 FNV-1a 해시입니다. 프레임은 `min_version`부터 `version`까지의 지문을 순서대로 싣고, 수신 클래스는 자기 version의 지문을 비교합니다. 구현 원본은 [SchemaFingerprint.cs](../protaco_compiler/protacoc/SchemaFingerprint.cs)입니다.

**정규 텍스트**

```text
protaco-schema-1|<name>|<version>|<ext 0/1>|<field>;<field>;...
field = <key>:<type>[!][=<default>]
```

- `name`: 메시지 이름. `version`: 계산할 version의 십진수
- `ext`: `allow_extension`이 true이면 `1`, false이면 `0`
- 필드 순서: `added` 오름차순, 같으면 YAML 선언 순서
- 포함 범위: 계산할 version까지 추가된 필드
- `!`: required 필드. `=<default>`: 명시한 기본값의 텍스트, 문자열의 바깥 따옴표 제외
- 끝 구분자: 각 필드 뒤 `;`. BOM, 마지막 줄바꿈, 추가 공백 없음

고정 필드와 가변 필드를 나누어 정렬하지 않습니다. 실제 본문의 고정 영역 배치와 달리 정규 텍스트는 위 필드 순서를 그대로 사용합니다. 기본값을 생략하면 `=`도 생략합니다.

해당 version에서 제거된 고정 필드는 `~<key>:<type>;`로 남습니다. 바이트 자리를 유지하며 required·default 표기는 붙이지 않습니다. 제거된 가변 필드는 정규 텍스트에서 빠집니다. `added`·`removed` 숫자 자체는 포함하지 않으므로 새 version을 추가해도 옛 version의 지문은 유지됩니다.

| 타입 | 정규 텍스트 |
|---|---|
| 기본 타입 | `bool`, `int`, `int64`, `uint`, `uint64`, `double`, `string`, `string(N)`, `bytes` |
| 목록 | `list<T>`, T도 같은 규칙으로 변환 |
| map | `map<K,V>`, 쉼표 뒤 공백 없음, K·V도 같은 규칙으로 변환 |
| 다른 메시지 | `Name@xxxxxxxx`, 해당 메시지 지문의 소문자 16진수 8자리 |
| 자기 참조 | `Name@self` |
| enum | `enum(Name)`, 값 목록 제외 |

namespace, 주석, `desc`는 지문에 들어가지 않습니다. 다른 메시지의 지문 변경은 이를 참조하는 메시지에도 반영됩니다. enum 값 추가는 지문을 바꾸지 않습니다.

**계산**

입력 `canonical`은 위 규칙으로 만든 UTF-8 문자열입니다. 반환값은 부호 없는 32비트 지문입니다. 각 곱셈은 2³²를 법으로 계산합니다.

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

다음 세 입력은 각각 한 줄이며 줄바꿈은 해시에 포함하지 않습니다. 값은 생성 코드의 `MsgSchemaHash`와 같습니다.

```text
protaco-schema-1|Topping|1|1|name:string(24);grams:int=30;
protaco-schema-1|Heartbeat|1|0|node:string(32);seq:int64;healthy:bool=true;cpuLoad:double;uptimeSec:int;
protaco-schema-1|Order|3|1|id:string(8);~qty:int;color:string!;weight:double=0.5;tags:list<string>;label:string;rush:bool;
```

| 입력 | 지문 |
|---|---|
| Topping version 1 | `0xEB8A11D8` |
| Heartbeat version 1 | `0x2BB85C55` |
| Order version 3 | `0x7073B2E7` |

## 2. 명령행

입력 폴더와 출력 언어를 하나 이상 지정합니다. namespace는 YAML에서 정하며 한 실행의 모든 정의가 같은 namespace를 사용해야 합니다.

```text
protacoc -I <yaml dir> [-I ...] [--cpp_out <dir>] [--cs_out <dir>] [--py_out <dir>] [options] [message...]
```

| 옵션 | 의미 |
|---|---|
| `-I <dir>`, `--proto_path <dir>` | 스키마 폴더, 여러 번 지정 가능 |
| `--cpp_out <dir>` | C++ 출력 폴더 |
| `--cs_out <dir>` | C# 출력 폴더 |
| `--py_out <dir>` | Python 출력 루트 |
| `--no_runtime` | 런타임 파일 출력 생략 |
| `--check` | 파일을 쓰지 않고 예상 바이트와 비교 |
| `name...` | 생성할 메시지·enum 이름 또는 YAML 경로, 생략하면 전체 |
| `-h`, `--help` | 도움말 |
| `--version` | 생성기 버전 |

**출력 파일**

| 언어 | 출력 경로 |
|---|---|
| C++ | `<cpp_out>/<Name>.g.h`, `<Enum>.enum.h`, `Protaco.hpp` |
| C# | `<cs_out>/<Name>.g.cs`, `<Enum>.enum.cs`, `Protaco.cs` |
| Python | `<py_out>/<namespace 폴더>/<Name>_g.py`, `<Enum>_enum.py`; 런타임은 `<py_out>/protaco.py` |

대상을 선택해도 입력 전체를 읽고 검사합니다. 참조하는 정의를 출력 대상에 자동으로 추가하지 않으므로 필요한 메시지와 enum을 함께 선택해야 합니다. YAML 경로도 입력 폴더에서 읽은 정의의 이름으로 선택합니다. `--no_runtime`을 쓰면 같은 생성기 버전의 런타임을 별도로 제공해야 합니다.

**Windows 명령 프롬프트**

```bat
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs --py_out examples\gen\py
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs --py_out examples\gen\py --check
bin\Release\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp Menu Category PizzaSize
```

**종료 코드와 바이트 규칙**

| 코드 | 의미 |
|---|---|
| 0 | 성공, 또는 `--check` 결과 모두 일치 |
| 1 | 명령행 구문 오류 |
| 2 | 스키마·입출력·대상 선택 오류 |
| 3 | `--check`에서 파일 누락 또는 바이트 불일치 |

`--check`는 선택한 출력과 기본 런타임까지 비교하며 파일을 쓰지 않습니다. 불일치 파일은 `path: error PC501: out of date (run protacoc again)`으로 표시합니다. 존재하지 않는 대상을 지정한 PC502도 대상 선택 단계에서 발생하므로 종료 코드는 2입니다.

생성 파일은 모든 OS에서 UTF-8 BOM과 CRLF를 사용합니다. 생성 파일 첫 주석의 원본 sha1은 YAML의 BOM을 제거하고 CRLF를 LF로 맞춘 UTF-8 본문에서 계산합니다. 체크아웃 줄바꿈이 달라도 생성 결과는 같습니다.

## 3. 진단

### 3.1 생성기 오류 코드

위치를 아는 오류는 `파일(줄,열): error PC404: 메시지` 형식으로 표준 오류에 출력합니다. 위치가 없는 오류는 `protacoc: error PC502: 메시지` 형식입니다. 형식과 코드 정의는 [Diag.cs](../protaco_compiler/protacoc/Diag.cs)에 있습니다.

| 코드 | 뜻 | 대표 원인 또는 예 |
|---|---|---|
| PC101 | 지원하지 않는 YAML 구성 | 지원 범위 밖의 YAML 문법 |
| PC102 | 흐름 문법 오류 | 중괄호·따옴표·콜론 오류, `[a, b]`, 따옴표 없는 `map<K, V>` |
| PC201 | 모르는 최상위 키 | `owner` |
| PC202 | 필수 키 또는 값 누락 | `message/version/namespace/fields` 누락, 현재 version에 enum 값 없음 |
| PC203 | 모르는 필드 속성 | `eq` 등 정의되지 않은 속성 |
| PC204 | 잘못된 값 형식 | bool·int·이름 형식 오류 |
| PC205 | 중복 속성 | 같은 항목에 `key` 두 번 |
| PC206 | namespace 오류 | 잘못된 이름 또는 예약어 조각 |
| PC208 | 정의 종류와 파일 접미사 불일치 | enum이 `<Name>.enum.yaml` 밖에 있거나 해당 파일에 `enum:` 없음 |
| PC301 | 정의 이름과 파일 이름 불일치 | `Menu.yaml`에 다른 이름 |
| PC302 | 중복 필드·enum 키 | 같은 `key` 재선언 |
| PC303 | 예약어·이름 규칙 위반 | 생성 멤버·예약 접두와 충돌, 키 255바이트 초과 |
| PC304 | 타입 문법 오류 | `map<string>` |
| PC305 | 고정 필드에 required | `int`, `string(N)` 등에 지정 |
| PC306 | 자기 참조 필드에 required | 단일 Self 또는 `list<Self>`에 지정 |
| PC308 | 잘못된 default | 범위·문자열 제한 위반, enum에 없는 값 이름 |
| PC310 | 고정 문자열 길이 오류 | `string(N)`의 N이 1..4096 밖 |
| PC311 | 생성 속성 이름 충돌 | C# 첫 글자 대문자 변환, Python 예약어의 `_` 접미사 |
| PC312 | version 이력 범위 오류 | `min_version`, `added`, `removed` 범위·순서 |
| PC313 | 제거된 가변 필드 줄 잔존 | `removed <= min_version`, 해당 줄 삭제 필요 |
| PC314 | version용 예약 이름 | 메시지·enum 이름이 `v<N>` |
| PC318 | enum 번호 재사용 | 제거된 값을 포함한 번호 중복 |
| PC401 | 중복 정의 이름 | 메시지·enum 이름 충돌 |
| PC402 | 모르는 타입 참조 | 읽은 정의에 없는 타입 |
| PC403 | 순환·허용되지 않는 자기 참조 | 두 메시지의 상호 참조, map 안의 Self |
| PC404 | 컨테이너에 허용되지 않는 타입 | `list<bool>`, `map<double, int>`, `string(N)` map 값 |
| PC405 | namespace 혼용 | 한 실행에 서로 다른 namespace |
| PC406 | 생성기 중첩 한도 초과 | 자기 참조를 제외한 중첩 100단계 초과 |
| PC407 | 중첩 메시지의 version 기능 | 필드 타입 메시지에 `min_version/added/removed` 사용 |
| PC501 | 파일 입출력 | 입력 폴더 없음, 쓰기 실패, `--check` 불일치 |
| PC502 | 명령행·대상 오류 | 출력 언어 없음, 모르는 옵션 또는 대상 |

### 3.2 디코딩 오류

오류 문자열은 `경로: 이유`이며 경로가 없으면 이유만 나옵니다. 공통 디코딩 검사는 세 언어에서 같은 문구를 사용합니다. 표의 `N`, `R`, `S`, `A`, `B` 등은 실제 값이 들어가는 자리입니다.

- 필드 경로: `delivery.doorCode`
- 목록 경로: 0부터 시작하는 원소 인덱스, `sections[0].name`
- map 경로: 키 값이 아닌 직렬화된 항목의 인덱스, `prices[1]`
- 본문 경로: 최상위이면 빈 경로, 중첩이면 해당 메시지까지의 경로
- 새 메시지 version의 본문 오류: 끝에 ` (frame version S read as R)`. S·R은 메시지 version이며 직렬화 형식 번호 4와 별개

C++ `CParseError::GetError()`의 `Error{path, what}`, C# `ProtacoException.Path/What`, Python `ProtacoError.path/what`에서 경로와 이유를 따로 읽습니다. C++ `what()`, C# `Message`, Python `str(error)`는 합친 문자열입니다. Try API는 같은 문자열을 반환 인자나 Result에 담습니다.

**프레임 검사**

| 문구 | 원인 | 경로 |
|---|---|---|
| `frame too large` | 프레임 크기 한도 초과 | 없음 |
| `frame too short` | 28바이트 기본 헤더, 이름 또는 지문 배열 부족 | 없음 |
| `bad magic` | 시작 두 바이트가 `PC` 아님 | 없음 |
| `unknown frame version N, expected 4` | 직렬화 형식 불일치 | 없음 |
| `bad version range A..B` | 최솟값 0, 역전 또는 지원 version 255개 초과 | 없음 |
| `message name mismatch: expected A, got B` | 수신 클래스와 메시지 이름 불일치 | 없음 |
| `message version too old: N reads version R or newer, got S` | S < R. 지원하는 옛 타입이 있으면 `. Use the vS type` 추가 | 없음 |
| `message version too new: N reads version R, sender supports A..S` | 송신 측 min_version > R | 없음 |
| `schema mismatch: N version R expected 0xAAAAAAAA, got 0xBBBBBBBB` | R의 지문 불일치 | 없음 |
| `frame truncated` | BodyLen보다 실제 본문이 짧음 | 없음 |
| `trailing bytes after frame` | BodyLen 뒤에 추가 바이트 | 없음 |

**본문 검사**

| 문구 | 원인 | 경로 |
|---|---|---|
| `nesting too deep` | 중첩 100단계 초과 | 중첩 메시지 |
| `fixed block truncated` | FixedLen 또는 고정 영역 부족 | 본문 |
| `fixed block size mismatch: expected A, got B` | 같은 version에서 크기 불일치, 새 version에서 필요한 크기보다 작음 | 본문 |
| `field key truncated` | TLV 키 길이 0 또는 키 잘림 | 본문 |
| `field value truncated` | TLV 타입·길이·값 잘림 | 해당 키 |
| `duplicate key` | 같은 TLV 키 중복 | 해당 키 |
| `too many fields` | 본문당 가변 필드·ext 합계 1,024개 초과 | 본문 |
| `missing required` | 필수 가변 필드 누락 | 해당 키 |
| `type mismatch: expected string, got int` | 스키마와 TLV 타입 불일치, 타입 이름은 실제 값 | 해당 키 |
| `unknown key (allow_extension false)` | 같은 version의 닫힌 메시지에 모르는 키 | 해당 키 |
| `scalar size mismatch` | 스칼라의 바이트 길이 불일치 | 목록 원소 또는 map 값 |
| `list header truncated` | 원소 타입·개수 부족 | 목록 필드 |
| `list element type mismatch` | 목록 원소 타입 불일치 | 목록 필드 |
| `list count too large` | 개수가 남은 바이트 / 4 초과 | 목록 필드 |
| `list element truncated` | 원소 길이·값 잘림 | `field[i]` |
| `trailing bytes in list` | 선언한 원소 뒤에 추가 바이트 | 목록 필드 |
| `map header truncated` | 키·값 타입 또는 개수 부족 | map 필드 |
| `map key or value type mismatch` | 키·값 타입 불일치 | map 필드 |
| `map count too large` | 개수가 남은 바이트 / 8 초과 | map 필드 |
| `map key truncated or bad` | 키 길이·값 잘림 또는 키 디코딩 실패 | `field[i]` |
| `map value truncated` | 값 길이·값 잘림 | `field[i]` |
| `duplicate map key` | 같은 map 키 중복 | `field[i]` |
| `trailing bytes in map` | 선언한 항목 뒤에 추가 바이트 | map 필드 |
| `out of memory` | C++ 디코딩 중 할당 실패 | 실패 위치에 따라 다름 |

**확장 데이터 오류**

아래 문구는 ext 호출의 오류 문자열입니다. 디코딩 경로 접두사 대신 문구 안에 키를 표시합니다.

| 문구 | 원인 | 적용 |
|---|---|---|
| `ext key is empty` | 빈 키 | 공통 |
| `ext key is longer than 255 bytes: prefix...` | UTF-8 키 255바이트 초과, 앞부분 표시 | 공통 |
| `ext key starts with '_' (reserved): _x` | 예약 접두사 | 공통 |
| `ext key is a field of the message: rider` | 어느 지원 version에서든 스키마 필드인 키 | 공통 |
| `ext key not found: missing` | 키 없음 | 공통 |
| `ext key channel holds string, not int` | 저장 타입과 요청 타입 불일치 | 공통 |
| `ext key k has a bad int value` | 저장된 스칼라 길이 오류 | 공통 |
| `unsupported ext value type T` | 지원하지 않는 값 타입 | C#·Python; C++은 컴파일 오류 |
| `ext value 5 needs a kind: INT, INT64, UINT or UINT64` | 정수에 kind 생략 | Python |
| `unsupported ext value kind K` | 지원하지 않는 kind | Python |
| `ext is off for this message (allow_extension false)` | 닫힌 메시지의 ext 쓰기 | Python; C++·C#은 해당 공개 접근자 없음 |

**시각 변환 오류**

| 문구 | 원인 | 적용 |
|---|---|---|
| `timestamp N is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)` | 지원 시각 범위 밖 | 공통, 경로 없음 |
| `timestamp N has no local time` | 플랫폼의 로컬 시각 변환 실패 | C++·Python, 경로 없음 |

### 3.3 수신 판정 순서

수신 클래스의 메시지 version을 R, 프레임의 메시지 version을 S라고 하면 `ToObject`·`TryToObject`와 고정 필드 `TryView`는 다음 순서로 검사합니다.

1. 프레임 크기 한도.
2. 헤더 최소 길이, magic, 직렬화 형식, 이름·지문 영역, version 범위.
3. 메시지 이름.
4. S < R 여부. 옛 프레임은 그 version의 생성 클래스로 수신.
5. 송신 측 min_version > R 여부.
6. 송신 프레임에 실린 R의 지문과 수신 클래스 지문.
7. BodyLen과 실제 본문 길이. 잘림과 뒤에 붙은 바이트 검사.
8. 본문 고정 영역, TLV, 필수 필드, 중첩·컨테이너 규칙. S > R이면 본문 오류에 version 꼬리말 추가.

열린 메시지는 모르는 필드를 ext에 보관합니다. 닫힌 메시지는 같은 version의 모르는 키를 거부하고, 더 새 version에서 받은 모르는 필드는 버립니다. `PeekHeader`는 헤더만 읽으므로 이 전체 검사를 대신하지 않습니다.

## 4. C++ API

런타임은 [Protaco.hpp](../include/cpp/Protaco.hpp)이며 C++14와 표준 라이브러리만 필요합니다. 생성 헤더는 `<Name>.g.h`, 클래스는 `<namespace>::<Name>`입니다. 아래 시그니처는 공개 멤버 선언이며 본문을 생략했습니다. `T`·`<k>`는 필드 타입·이름을 나타내는 표기입니다. 코드 예제는 함수 본문에서 실행하며 해당 생성 헤더를 include합니다.

Try API의 실패 이유는 선택 인자 `std::string* error = nullptr`에 기록합니다. 성공 여부는 반환값으로 확인합니다. 성공 시 기존 오류 문자열이 지워진다고 가정하지 않습니다. `out` 등 결과 포인터는 유효한 쓰기 가능한 객체를 가리켜야 합니다.

### 4.1 생성 클래스

**생성자와 고정 필드**

**C++**

```cpp
Name();
T get_<k>() const;
void set_<k>(T value);

std::string get_<k>() const; // string(N)
bool set_<k>(const std::string& value, std::string* error = nullptr);
```

생성자는 스키마 기본값으로 필드를 초기화하고 봉투에 새 id와 현재 시각을 넣습니다. 디코더와 생성 코드가 내부적으로 만드는 중첩 필드는 id를 소비하지 않습니다.

- 매개변수: `value`는 새 필드 값, `error`는 고정 문자열 오류 출력
- 반환값: getter는 값, 숫자·bool·enum setter는 void, 고정 문자열 setter는 성공 여부
- 고정 문자열 오류: UTF-8 바이트 수가 N 초과이면 false, 기존 값 유지. 예: `orderId: 17 bytes, longer than string(16)`
- 배치: bool 1바이트, int·uint·enum 4바이트, int64·uint64·double 8바이트, `string(N)` N바이트
- 주의: 고정 필드 setter는 객체의 프레임 버퍼에 직접 기록. `string(N)`의 N은 1..4096이며 남는 바이트는 0

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

**가변 필드**

**C++**

```cpp
const T& get_<k>() const;
T& mutable_<k>(); // bytes, list, map, message
void set_<k>(const T& value);
```

`string`에는 `mutable_<k>()`가 없습니다. bytes는 `std::vector<uint8_t>`, list는 `std::vector<T>`, map은 `std::unordered_map<K,V>`, 중첩 메시지는 생성 클래스입니다.

- 매개변수: `value`는 복사할 새 값
- 반환값: getter는 읽기 전용 참조, mutable은 수정 가능한 참조, setter는 void
- 수명: 참조는 메시지와 해당 필드 저장 공간의 수명에 종속. 컨테이너 원소의 참조 무효화 규칙도 적용
- 주의: mutable 호출 후에는 변경 여부를 추적할 수 없어 Serialize마다 가변 블록 재생성

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

map 직렬화 순서는 컨테이너 순회 순서입니다. 같은 내용도 언어·구현에 따라 바이트 순서가 다를 수 있습니다. 디코딩은 내용을 복원하며 중복 키를 거부합니다.

**선택 자기 참조 필드**

[시험용 Node](../protaco_compiler/tests/gen/cpp/recursive/Node.g.h)의 단일 `next` 필드 시그니처입니다. 이 예제에서는 메뉴 분류의 연결 노드로 사용합니다.

**C++**

```cpp
bool has_next() const;
const Node& get_next() const;
Node& mutable_next();
void set_next(const Node& value);
void clear_next();
```

- 매개변수: `value`는 깊은 복사할 Node
- 반환값: has는 존재 여부, get은 참조, mutable은 없으면 생성한 노드의 참조
- 기본 상태: 없음. get은 빈 Node 반환, 직렬화 데이터에는 해당 필드 생략
- clear: 값을 제거하고 없는 상태로 복귀
- 주의: 자기 참조는 단일 Self 또는 `list<Self>`만 가능. required·map 안의 Self·상호 순환·version 이력 기능 불가

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

**확장 데이터 접근**

**C++**

```cpp
const protaco::CExt& GetExt() const;
protaco::CExt& GetMutableExt();
```

열린 메시지에만 생성합니다. 매개변수는 없으며 객체가 소유하는 ext의 참조를 반환합니다. 참조 수명은 메시지에 종속됩니다. 키 규칙과 사용 예는 [4.4 확장 데이터](#44-확장-데이터)에 있습니다.

**Serialize와 ToString**

**C++**

```cpp
const std::vector<uint8_t>& Serialize() const;
std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const;
```

Serialize는 봉투를 포함한 전체 프레임의 참조를 반환합니다. 반환한 데이터는 객체를 수정하거나 파괴하기 전까지 사용할 수 있습니다. 비동기 전송 중 객체를 바꿀 필요가 있으면 별도 vector에 복사합니다.

- 매개변수: Serialize는 없음. ToString의 `style`은 [4.5 텍스트](#45-텍스트)의 출력 설정
- 반환값: Serialize는 객체 내부 버퍼 참조, ToString은 독립적인 문자열
- 고정 필드만 있고 ext가 빈 경우: setter가 기록한 버퍼를 그대로 반환
- 가변 필드·ext가 변경되지 않은 경우: 이전 가변 블록 재사용
- 가변 setter·ext 변경 후: 가변 블록 재생성
- mutable 접근자를 한 번이라도 호출한 객체: 이후 Serialize마다 가변 블록 재생성

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

**메타데이터와 옛 version 클래스**

**C++**

```cpp
static const char* MsgName();
static uint16_t MsgVersion();
static uint16_t MsgMinVersion();
static uint32_t MsgSchemaHash();
```

매개변수 없이 클래스의 이름, version, 지원 최솟값, 해당 version의 지문을 반환합니다. 이름 포인터는 정적 문자열을 가리킵니다. 옛 클래스는 같은 헤더의 `<ns>::v<N>::<Name>`에 생성되며 해당 version의 필드만 노출합니다. 최신 클래스가 옛 프레임을 자동으로 최신 객체로 바꾸지는 않습니다.

**C++**

```cpp
const char* name = demo::DeliveryRoute::MsgName();
uint16_t version = demo::DeliveryRoute::MsgVersion();
uint16_t minimum = demo::DeliveryRoute::MsgMinVersion();
uint32_t hash = demo::DeliveryRoute::MsgSchemaHash();

build3::v1::Order oldOrder; // tests/gen/cpp/build3/Order.g.h
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

**View**

고정 필드만 있는 version에는 `class View : public protaco::CView`가 생성됩니다. 각 필드의 `get_<k>() const`와 `GetEnvelope/GetId/GetCorr/GetTimestamp`를 제공합니다. 필드 getter는 값을 반환하며 고정 문자열 getter는 `std::string`을 만듭니다. 따라서 TryView 자체의 무복사 동작과 문자열 반환의 비용은 별개입니다. 생성·수명·예제는 [4.3 디코딩](#43-디코딩)의 TryView 항목에 있습니다.

### 4.2 봉투

**Envelope와 메시지 접근자**

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

`id`는 메시지 id, `corr`는 이 응답이 가리키는 요청 id, `ts`는 1970-01-01 이후 UTC 마이크로초입니다. 메시지 생성 시 id·ts를 자동 설정하고 corr은 0으로 시작합니다. corr은 사용자가 설정합니다.

- 매개변수: 각 setter의 새 값, 또는 봉투 전체 `env`
- 반환값: getter는 값 복사, setter는 void
- 저장 위치: 프레임 헤더. SetTimestampAsNow는 NowUs의 현재 값을 기록
- 주의: 직렬화·디코딩은 새 id를 발급하지 않으며, 디코딩한 봉투를 보존

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

**id와 시계**

**C++**

```cpp
uint32_t NextId();
int64_t NowUs();
void SetClock(int64_t (*nowUs)());
```

- 매개변수: `nowUs`는 UTC 마이크로초를 반환하는 함수. nullptr이면 시스템 시계 복귀
- 반환값: NextId는 1부터 증가하는 모듈 단위 id, NowUs는 현재 시계의 값, SetClock은 void
- 주의: id는 순환 시 0을 건너뛰며 스레드 안전. SetClock은 동기화하지 않으므로 메시지 생성 전에 설정

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

**시각 변환**

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

UTC 마이크로초를 실행 환경의 로컬 시각으로 변환합니다. 메시지 멤버는 GetTimestamp 값을 입력으로 사용합니다.

- 매개변수: `timestampUs` 또는 객체의 ts, 결과를 받을 포인터, 선택 오류 문자열
- 반환값: 변환 성공이면 true. 문자열은 `YYYY-MM-DD`, `hh:mm:ss.ffffff`
- 정수 출력: 년·월·일·시·분·초·밀리초·남은 마이크로초. 소수 초 = millisecond × 1000 + microsecond
- 범위: 2000-01-01 00:00:00 UTC부터 9999-12-31 23:59:59.999999 UTC까지
- 실패: 범위 밖 또는 로컬 변환 실패이면 false. [시각 변환 오류](#32-디코딩-오류) 참고

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

### 4.3 디코딩

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

- 매개변수: `src/size` 또는 `bytes`는 전체 프레임, `out`은 생성 클래스 T의 객체, `error`는 선택 오류 출력
- 반환값: 성공이면 true와 디코딩된 out. 실패이면 false, out의 기존 값 유지
- 오류: 디코딩 실패를 반환값으로 보고. 문자열 형식은 [3.2](#32-디코딩-오류)
- 입력 수명: 일반 오버로드는 호출 중에만 입력 필요. out 자신의 Serialize 버퍼도 입력 가능

rvalue vector는 프레임의 메시지 version·min_version·모든 지문이 T의 배치와 같으면 버퍼를 넘겨받습니다. 성공한 객체가 원래 버퍼를 소유하며, 가변 필드나 ext가 바뀌기 전까지 Serialize가 이를 재사용합니다. 다른 배치의 허용 프레임은 일반 디코딩처럼 복사합니다. 이 최적화는 프레임 버퍼에 관한 것이며 가변 필드 객체의 디코딩은 필요합니다. 실패하더라도 이동한 입력 vector가 보존된다고 가정하지 않습니다.

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

버퍼 소유권을 넘기는 rvalue API: 해당 없음.

</details>

<details>
<summary>Python</summary>

```python
data = DeliveryRoute().Serialize()
result = protaco.TryToObject(DeliveryRoute, data)
if result:
    received = result.value
```

버퍼 소유권을 넘기는 rvalue API: 해당 없음.

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

매개변수와 rvalue 버퍼 인수 조건은 TryToObject와 같습니다. 성공하면 새 T를 반환하고 디코딩 실패 시 CParseError를 던집니다. GetError의 참조와 what의 문자열은 예외 객체의 수명 동안 사용합니다. CParseError는 같은 모듈에서 잡고 DLL 경계를 넘기지 않습니다.

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

- 조건: 해당 version에 가변 필드가 없고 T::View가 생성된 메시지. ext 허용 여부와는 별개
- 매개변수: 입력 프레임, 결과 view, 선택 오류 문자열. T는 명시
- 반환값: 성공이면 true와 읽기 전용 view. 실패는 TryToObject와 같은 판정·문구
- 수명: 입력 바이트가 살아 있고 바뀌지 않는 동안만 유효. view를 읽기 전 성공 여부 확인
- 비용: 성공 경로에서 프레임 복사·할당 없음. ext 값은 view로 접근 불가
- 삭제된 오버로드: 임시 vector를 참조하는 view를 막기 위한 컴파일 오류

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

해당 없음. Python은 객체 디코딩을 사용합니다.

</details>

**PeekHeader와 Header**

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

- 매개변수: 헤더까지 포함된 바이트, 길이, 결과 Header, 선택 오류 문자열
- 반환값: 성공이면 true와 헤더 값. schemaHash는 송신 측 version의 지문
- 주의: 본문이 없어도 헤더가 완전하면 성공. 메시지 타입·본문 길이·본문 유효성 검사는 별도 디코딩에서 진행
- 수명: Header는 이름과 봉투를 소유하므로 입력 바이트 수명에 종속되지 않음
- 전체 프레임 길이: 28 + 이름의 UTF-8 바이트 수 + 4 × 지원 version 수 + bodySize

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

### 4.4 확장 데이터

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

- 매개변수: `key`는 1..255 UTF-8 바이트의 키, `value`는 저장할 값
- 반환값: 성공이면 true. 같은 키가 있으면 타입과 값 교체
- 실패: 빈 키, 긴 키, `_` 접두, 어느 지원 version에서든 메시지 필드인 키
- 지원 타입: 위 오버로드만. 다른 숫자 타입은 원하는 폭으로 명시적 캐스트
- 주의: 문자열·bytes는 복사. `const char*`의 nullptr는 빈 문자열로 처리

**CExt::TryGetValue와 관리 함수**

다음 `V*` 표기는 bool, int32_t, int64_t, uint32_t, uint64_t, double, std::string, std::vector<uint8_t> 각각의 오버로드를 줄여 쓴 것입니다.

**C++**

```cpp
bool TryGetValue(const std::string& key, V* out, std::string* error = nullptr) const;
bool Has(const std::string& key) const;
bool Remove(const std::string& key);
size_t Count() const;
std::vector<std::string> Keys() const;
```

- 매개변수: 조회·삭제할 key, TryGetValue의 결과 out과 선택 error
- 반환값: TryGetValue는 성공 여부와 복사된 값. 실패 시 out 유지
- 타입 검사: 다른 폭의 정수도 다른 타입. 자동 숫자 변환 없음
- 관리 함수: Has는 존재 여부, Remove는 실제 제거 여부, Count는 개수, Keys는 추가 순서의 키 복사본
- 실패 이유: 키 없음, 타입 불일치, 저장된 스칼라 길이 오류

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

### 4.5 텍스트

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

- 설정: separator는 한 줄 항목 구분자, open·close는 괄호, indent는 단계당 공백 수, 0이면 한 줄
- maxText: 문자열 생략 기준. C++은 바이트 수, C#·Python은 문자 수
- envelope: 최상위 봉투 표시. Pretty는 indentWidth와 envelope=true인 TextStyle 반환
- 수명: C++의 구분자 포인터는 ToString 호출이 끝날 때까지 유효한 문자열 필요

필드는 YAML 순서로, ext는 끝의 `ext{...}`로 출력합니다. 문자열은 따옴표·역슬래시·줄바꿈 등을 escape하며 길면 `"abcd"...(+12)`처럼 생략합니다. bytes는 앞 8바이트까지 표시합니다. double은 유효 숫자 15자리이며 nan/inf/-inf를 사용하고 0과 -0은 `0`입니다. enum은 `Large(3)`, 모르는 번호는 `?(9)`, 없는 선택 필드는 `null`입니다. map은 키를 정렬하며 문자열은 코드 포인트 순서, 숫자·enum은 값 순서입니다.

Pretty의 ts는 지원 범위 안이면 로컬 시각, 밖이면 정수로 표시합니다. ASCII 값의 텍스트는 세 언어에서 같습니다.

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

예제 프로그램의 실제 출력입니다. 봉투 시각은 실행마다 달라지고, id는 예제의 메시지 생성 순서로 정해집니다.

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

### 4.6 enum

**C++**

```cpp
// PizzaSize.enum.h, namespace demo
enum class PizzaSize : int32_t { Small = 1, Medium = 2, Large = 3 };
const char* EnumName(PizzaSize value);
bool TryParseEnum(const std::string& name, PizzaSize* out,
                  std::string* error = nullptr);
```

- 매개변수: value는 enum 값, name은 YAML 값 이름, out은 결과 포인터
- 반환값: EnumName은 제거된 값까지 포함한 정적 이름 문자열, 모르는 번호이면 nullptr
- TryParseEnum: 현재 version의 이름만 허용. 실패 시 false, out 유지, `unknown PizzaSize name: Kids` 형태의 error
- 호환성: 수신 측이 모르는 번호도 enum 값으로 보존. 직렬화 타입은 int32
- 주의: 제거된 번호를 재사용하지 않으며 제거한 값 줄도 스키마에 유지

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

### 4.7 상수와 한도

모두 `protaco` namespace에 있습니다.

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

| 상수 | 의미 |
|---|---|
| kFrameVersion | 직렬화 형식 4 |
| kMaxDepth | 메시지 중첩 100단계 |
| kMaxSchemaVersions | 한 프레임이 지원하는 연속 version 최대 255개 |
| kMaxFields | 본문당 가변 TLV와 ext 합계 최대 1,024개 |
| kMaxFrameBytes | 전체 프레임 최대 4 GiB - 1바이트 |
| kMaxKeyLength | 키 최대 255 UTF-8 바이트 |

목록 원소 수는 남은 바이트 / 4, map 항목 수는 남은 바이트 / 8 이하로 검사합니다. C#의 더 작은 배열 한도는 [5.7](#57-상수와-한도)에 있습니다.

런타임 버전 가드는 `PROTACO_RUNTIME_0_6_0`입니다. 출력 폴더별 런타임 사본이 있어도 같은 버전은 한 번만 include됩니다. 다른 런타임 버전을 섞으면 재정의 오류가 발생합니다.

**C++**

```cpp
static_assert(protaco::kFrameVersion == 4, "serialization format");
```

## 5. C# API

[Protaco.cs](../include/cs/Protaco.cs)와 `<Name>.g.cs`를 프로젝트에 포함합니다. .NET Framework 4.6.1과 .NET 8을 지원하며 런타임은 C# 7.3을 사용합니다. 다음은 C++ API와 같은 순서의 이름 대응 및 차이입니다. 동작별 C# 예제는 4장의 접힌 코드에도 있습니다.

### 5.1 생성 클래스

<details>
<summary>C# — 시그니처</summary>

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

클래스는 `Protaco.Message, Protaco.IMessage`를 구현합니다. 필드 이름은 YAML 키의 첫 글자를 대문자로 바꾼 속성입니다. 목록은 List, map은 Dictionary, bytes는 byte[]이며 속성으로 읽고 대입합니다.

- 생성자: 스키마 기본값과 새 봉투로 초기화
- 속성 setter: 새 값 대입. 고정 문자열은 UTF-8 N바이트 초과 시 ArgumentException, 기존 값 유지
- Serialize 반환값: 호출마다 봉투를 포함한 새 byte[]. 이전 배열은 이후 객체 수정에 영향 없음
- ToString 반환값: 문자열. style 인자는 출력 설정
- Ext: 열린 메시지만 공개. 반환값은 메시지 소유 Ext
- 메타데이터: 매개변수 없는 함수 대신 const 필드
- 단일 자기 참조: 기본값 null, null 대입으로 제거
- 옛 version: 같은 파일의 `<ns>.v<N>.<Name>`
- View: 고정 필드만 있는 version의 struct. [5.3](#53-디코딩) 참고

<details>
<summary>C# — 예제</summary>

```csharp
var route = new demo.DeliveryRoute { Rider = "rider-7" };
route.Stops.Add(new demo.RouteStop { OrderId = "A-100", Address = "1 Pizza St" });
byte[] bytes = route.Serialize();
string text = route.ToString();
uint hash = demo.DeliveryRoute.MsgSchemaHash;
```

</details>

### 5.2 봉투

<details>
<summary>C# — 시그니처</summary>

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
public const long TimestampMin = 946684800000000L;
public const long TimestampMax = 253402300799999999L;
public static bool TryGetTime(long timestampUs, out string date, out string time, out string error);
public static bool TryGetTime(long timestampUs, out int year, out int month, out int day,
    out int hour, out int minute, out int second, out int millisecond, out int microsecond,
    out string error);
```

</details>

id·corr·ts의 의미와 시각 범위는 [4.2](#42-봉투)와 같습니다. getter는 값, setter는 void를 반환합니다. GetEnvelope는 struct 복사본을 반환합니다. 시계 콜백은 UTC 마이크로초를 반환해야 하며 null로 시스템 시계를 복원합니다. NextId는 스레드 안전하고, 시계 교체는 메시지 생성 전에 합니다.

시각 변환은 bool로 성공 여부를 반환하고 out 인자에 로컬 시각을 기록합니다. 실패 시 문자열 출력은 null, 정수 출력은 0입니다. 메시지 멤버에는 error 없는 오버로드도 있지만 `Envelopes.TryGetTime`에는 out error가 필요합니다.

<details>
<summary>C# — 예제</summary>

```csharp
var request = new demo.DeliveryRoute();
var reply = new demo.DeliveryRoute();
reply.SetCorr(request.GetId());
bool ok = reply.TryGetTimeFromTimestamp(out string date, out string time, out string error);
```

</details>

### 5.3 디코딩

<details>
<summary>C# — 시그니처</summary>

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

- 매개변수: bytes는 프레임, out은 결과, error는 오류 문자열
- ToObject: 성공하면 T, 디코딩 실패 시 ProtacoException. Message는 `path: reason`
- TryToObject: 성공이면 true. 실패이면 false와 default(T), error에 이유. out 인자는 항상 새로 씀
- PeekHeader: 성공이면 true와 Header. 실패이면 header=null. 본문 검사는 하지 않음
- TryView: 고정 필드 version만 제공. 성공이면 View, 실패이면 default(View)
- View 수명: 내부 참조가 배열을 유지하며 배열이 변경되지 않는 동안 유효. 필드는 읽기 전용 속성, 봉투는 GetEnvelope()
- 버퍼 소유권 이전: 해당 없음

<details>
<summary>C# — 예제</summary>

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

### 5.4 확장 데이터

<details>
<summary>C# — 시그니처</summary>

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

매개변수 key·value·error와 키 규칙은 [4.4](#44-확장-데이터)와 같습니다. T는 bool, int, long, uint, ulong, double, string, byte[]입니다. SetValue와 TryGetValue는 실패 시 false와 오류 문자열을 반환합니다. TryGetValue의 실패 출력은 default(T)입니다. 지원하지 않는 T는 컴파일 단계가 아니라 호출 결과로 거부합니다.

Has는 존재 여부, Remove는 실제 제거 여부, Count는 개수입니다. Keys는 추가 순서의 열거형 속성이므로 열거 중 ext를 변경하지 않습니다. 사용 예는 [4.4의 C# 코드](#44-확장-데이터)에 있습니다.

### 5.5 텍스트

<details>
<summary>C# — 시그니처와 예제</summary>

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

각 속성과 Pretty의 의미는 [4.5](#45-텍스트)와 같습니다. Pretty는 새 TextStyle을 반환합니다. MaxText는 문자 수 기준입니다.

### 5.6 enum

<details>
<summary>C# — 시그니처와 예제</summary>

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

Name은 제거된 값까지 조회하며 모르는 번호이면 null입니다. TryParse는 현재 version의 이름만 허용하고 실패 시 false와 default(PizzaSize)를 반환합니다. 수신한 모르는 번호는 enum 값으로 보존합니다.

### 5.7 상수와 한도

<details>
<summary>C# — 시그니처</summary>

```csharp
// Protaco.Codec:
public const int FrameVersion = 4;
public const int MaxDepth = 100;
public const int MaxSchemaVersions = 255;
public const int MaxFields = 1024;
public const int MaxFrameBytes = 0x7FFFFFC7;
```

</details>

배열 한도 때문에 프레임은 최대 0x7FFFFFC7바이트로 약 2 GiB입니다. 나머지 한도는 [4.7](#47-상수와-한도)과 같습니다. 런타임용 멤버는 생성 클래스의 명시적 IMessage 구현에 있으며 응용 코드에서 직접 호출할 필요가 없습니다.

## 6. Python API

[protaco.py](../include/python/protaco.py)는 Python 3.8 이상과 표준 라이브러리만 사용합니다. Try 계열은 결과를 Result로 반환합니다.

### 6.1 패키지와 import

출력 루트를 sys.path에 넣습니다. namespace `demo`는 `demo/`, `pizza.orders`는 `pizza/orders/`가 됩니다. 생성 모듈끼리는 상대 import를 사용합니다.

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
sys.path.insert(0, "examples/gen/py")
import protaco
from demo.DeliveryRoute_g import DeliveryRoute
from demo.RouteStop_g import RouteStop
from demo.PizzaSize_enum import PizzaSize, PizzaSizeNames
```

</details>

`types` 같은 표준 모듈 이름을 namespace로 쓰면 표준 모듈을 가릴 수 있으므로 피합니다.

### 6.2 생성 클래스와 Serialize

<details>
<summary>Python — 시그니처</summary>

```python
class DeliveryRoute(protaco.Message):
    def __init__(self, *, _stamp=None, **fields): ...

# Inherited members:
def Serialize(self): ...
def ToString(self, style=None): ...
```

</details>

- 매개변수: fields는 YAML 키의 값. _stamp는 생성 코드가 중첩 필드에 사용하는 내부 초기화 선택
- 속성: YAML 키 그대로. Python 예약어이면 끝에 `_` 추가(`from_`, `pass_`)
- 생성자 오류: 모르는 키에 TypeError
- Serialize 반환값: 봉투를 포함한 bytes
- Serialize 오류: 범위를 넘는 숫자, 긴 고정 문자열 등에 ProtacoError. 예: `orderId: 17 bytes, longer than string(16)`
- 컨테이너: list·dict·bytes. 단일 자기 참조의 기본값은 None
- 메타데이터: MsgName, MsgVersion, MsgMinVersion, MsgSchemaHash 클래스 속성
- 옛 version: 같은 생성 모듈의 `v<N>.<Name>`

생성자는 id·ts를 설정합니다. 디코더와 중첩 필드는 `_stamp=protaco.NO_STAMP`로 초기화하여 id를 소비하지 않습니다. 필드의 값 범위 검사는 Serialize 때 적용합니다.

<details>
<summary>Python — 예제</summary>

```python
route = DeliveryRoute(rider="rider-7", note="closest-first")
route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=10))
data = route.Serialize()
schema_hash = DeliveryRoute.MsgSchemaHash

# With protaco_compiler/tests/gen/py on sys.path:
from build3.Order_g import v1
old_order = v1.Order()
restored = protaco.ToObject(v1.Order, old_order.Serialize())
```

</details>

### 6.3 디코딩과 Result

<details>
<summary>Python — 시그니처</summary>

```python
protaco.ToObject(cls, data)
protaco.TryToObject(cls, data)
protaco.PeekHeader(data)
protaco.Result(ok, value=None, error=None)
protaco.ProtacoError(path, what)
```

</details>

- 매개변수: cls는 생성 클래스, data는 프레임 바이트
- ToObject 반환값: cls의 객체. 실패 시 ProtacoError, path·what·str(error)로 확인
- TryToObject 반환값: Result, 성공 value는 객체
- PeekHeader 반환값: Result, 성공 value는 Header. 본문 없이 헤더만 검사
- Header 속성: name, version, minVersion, schemaHash, env, bodySize
- Result: bool(result)는 ok, 성공 결과는 value, 실패 이유 문자열은 error. 실패 value는 None
- 풀기: `ok, value, error = result`
- 오류 처리: 잘못된 프레임은 Try 계열에서 예외 대신 실패 Result

<details>
<summary>Python — 예제</summary>

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

### 6.4 확장 데이터

<details>
<summary>Python — 시그니처</summary>

```python
message.ext.SetValue(key, value, kind=None)
message.ext.TryGetValue(key, kind)
message.ext.Has(key)
message.ext.Remove(key)
message.ext.Count
message.ext.Keys()
```

</details>

key 규칙은 [4.4](#44-확장-데이터)와 같습니다. SetValue는 성공 여부의 Result, TryGetValue는 조회 값의 Result를 반환합니다. 실패 이유는 error에 있습니다.

- kind 생략 가능: bool→BOOL, float→DOUBLE, str→STRING, bytes→BYTES
- 정수: INT, INT64, UINT, UINT64 중 kind 명시 필수
- 조회: 기대하는 kind 필수. 다른 정수 폭으로 자동 변환 없음
- Has·Remove: bool, Count: int 속성, Keys(): 추가 순서의 키 목록 복사본
- 닫힌 메시지: ext 속성은 있지만 쓰기는 `ext is off for this message (allow_extension false)`로 실패

<details>
<summary>Python — 예제</summary>

```python
route = DeliveryRoute()
stored = route.ext.SetValue("priority", 5, protaco.INT)
result = route.ext.TryGetValue("priority", protaco.INT)
if result:
    priority = result.value
```

</details>

### 6.5 봉투와 시각

<details>
<summary>Python — 시그니처</summary>

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
protaco.TryGetTime(timestamp_us)
protaco.TryGetTimeParts(timestamp_us)
protaco.TIMESTAMP_MIN  # 946684800000000
protaco.TIMESTAMP_MAX  # 253402300799999999
```

</details>

id·corr·ts의 의미는 [4.2](#42-봉투)와 같습니다. getter는 값, setter는 None을 반환합니다. GetEnvelope는 복사본을 반환하고 SetEnvelope도 값을 복사합니다. NextId는 스레드 안전합니다. SetClock은 UTC 마이크로초를 반환하는 콜백을 받으며 None이면 시스템 시계로 돌아갑니다. 메시지 생성 전에 설정합니다.

시각 변환의 입력은 UTC 마이크로초이고 결과는 로컬 시각입니다. TryGetTime 및 메시지의 같은 기능은 Result.value에 `(date, time)`을 담습니다. Parts는 `(year, month, day, hour, minute, second, millisecond, microsecond)`입니다. 지원 범위 밖이나 변환 실패는 error에 기록합니다.

<details>
<summary>Python — 예제</summary>

```python
request = DeliveryRoute()
reply = DeliveryRoute()
reply.SetCorr(request.GetId())
result = reply.TryGetTimeFromTimestamp()
if result:
    date, time = result.value
```

</details>

### 6.6 텍스트

<details>
<summary>Python — 시그니처와 예제</summary>

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

설정은 [4.5](#45-텍스트)와 같습니다. Pretty는 새 TextStyle, ToString·str은 문자열을 반환합니다. maxText는 문자 수 기준입니다.

### 6.7 enum

<details>
<summary>Python — 시그니처와 예제</summary>

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

Name은 제거된 값도 조회하며 모르는 번호에는 None을 반환합니다. TryParse는 현재 version의 YAML 이름만 받고 Result.value에 enum 값을 반환합니다. 실패 문자열은 `unknown PizzaSize name: Kids` 형태입니다.

디코더는 모르는 번호를 일반 int로 보존합니다. Python 예약어인 enum 멤버는 `None_`처럼 바뀌지만 Name·TryParse는 원래 YAML 이름을 사용합니다.

### 6.8 상수와 한도

<details>
<summary>Python — 상수</summary>

```python
protaco.VERSION              # "0.6.0"
protaco.FRAME_VERSION        # 4
protaco.MAX_DEPTH            # 100
protaco.MAX_SCHEMA_VERSIONS  # 255
protaco.MAX_FIELDS           # 1024
protaco.MAX_FRAME_BYTES      # 0xFFFFFFFF
```

</details>

한도는 [4.7](#47-상수와-한도)과 같습니다. 고정 영역은 struct.Struct로 pack/unpack합니다. View와 C++의 이동 버퍼 인수 API는 해당 없음입니다.

## 7. 빌드와 시험

### 7.1 실행과 환경

저장소 루트에서 실행합니다. Windows는 Visual Studio 2017 이상(MSBuild 15, v141), .NET Framework 4.6.1, Python 3.8 이상이 필요합니다. Linux는 .NET SDK 8 이상, g++, Python 3.8 이상이 필요합니다. build.sh는 macOS에서도 쓰도록 만들었지만 macOS는 확인하지 않았습니다. 아래 검증 기록은 Windows와 Linux 기준입니다.

**Windows 명령 프롬프트**

```bat
build.cmd
```

**Linux·macOS 셸**

```sh
sh build.sh
# When dotnet is installed under ~/.dotnet:
DOTNET=~/.dotnet/dotnet sh build.sh
```

두 스크립트는 같은 6단계를 실행합니다.

1. protacoc 빌드.
2. 시험·예제 코드 생성.
3. C++ 시험 빌드·실행.
4. C# 시험 빌드·실행.
5. Python 시험 실행.
6. C++·C#·Python 예제 빌드·실행.

| 환경 변수 | 역할 | 기본값 |
|---|---|---|
| MSBUILD | Windows MSBuild 경로 | Visual Studio 2017 Professional의 MSBuild.exe |
| DOTNET | build.sh의 .NET CLI | dotnet |
| CXX | build.sh의 C++ 컴파일러 | g++ |
| PYTHON | Python 실행 파일 | Windows: python, 셸: python3 |

Windows 출력은 `bin/Release`입니다. Linux의 .NET 출력은 `bin/Release/net8.0`, C++ 출력은 `bin/Release`입니다. 중간 산물은 `.IntermediateDirectory/`에 모입니다. Linux 스크립트는 기본 시험·예제를 C++17로 빌드하고 별도 번역 단위를 C++14로 검사합니다.

`protaco.sln`은 생성기와 C++·C# 시험, `examples/examples.sln`은 예제 솔루션입니다. 생성기의 `protacoc.csproj`와 `protacoc.net.csproj`는 각각 .NET Framework 4.6.1과 .NET 8에서 같은 소스를 빌드합니다. C# 시험·예제에도 대응하는 .net.csproj가 있습니다.

빌드는 생성 코드를 다시 씁니다. C++ 골든 파일은 `PROTACO_UPDATE_GOLDEN=1`일 때만 시험에서 다시 기록합니다. 시험 프레임워크 googletest 소스는 저장소에 포함되어 별도 설치가 필요 없습니다.

### 7.2 저장소 구성

```text
include/
  cpp/Protaco.hpp
  cs/Protaco.cs
  python/protaco.py
protaco_compiler/
  protacoc/
  tests/
    cpp/
    cs/
    py/
    schemas/
    gen/
    golden/
    googletest/
examples/
  schemas/
  gen/
  cpp/
  cs/
  py/
docs/
  benchmark/
  img/
bin/
  Release/
.IntermediateDirectory/
```

런타임 원본은 include/, 생성기는 protaco_compiler/protacoc/, 시험은 protaco_compiler/tests/에 있습니다. 예제 스키마와 프로그램은 [examples](../examples/README.ko.md), 성능 측정은 [benchmark](benchmark/README.ko.md)에 있습니다.

### 7.3 시험 범위

0.6.0 검증 기록은 Windows와 Linux 모두 C++ 41개, C# 36개, Python 26개 통과입니다. Linux 환경은 WSL Ubuntu 22.04, g++ 11.4, .NET 8.0.426, Python 3.10입니다. Linux에서 코드를 다시 생성해도 생성 파일 바이트가 같습니다.

시험 이름과 구현은 [C++ 시험](../protaco_compiler/tests/cpp/ProtacoTest.cpp), [C# 시험](../protaco_compiler/tests/cs/Program.cs), [Python 시험](../protaco_compiler/tests/py/test_protaco.py)에 있습니다.

- 생성기: 오류 코드별 YAML, 순환, namespace 혼용, 중첩 한도, 필드 타입 메시지의 version 기능 제한, enum default, 입력 순서 무관, `--check`의 최신·변조 결과 판정
- 스키마 계약: FNV-1a 정규 텍스트, 이름·version·지문 불일치, 옛 지문 유지, 새 프레임을 자기 version으로 읽기, 옛 타입 선택, 송신 min_version 검사
- 값과 컨테이너: 기본값, 고정 문자열 초과, bytes, 부호 없는 정수, UTF-8, enum, map, 자기 참조, 재귀 100단계 한도
- 오류와 경로: 필수 필드 누락, 중첩 경로, 닫힌 메시지의 키 거부·새 필드 버림, 모든 길이의 잘림, C++ 무작위 비트 반전
- 버퍼와 API: C++ 고정 필드 직접 기록·Serialize 재사용·mutable 참조·복사·ext 대입·이동 버퍼 인수, C++·C# TryView, 16개를 넘는 가변 필드, C++14 전용 번역 단위
- 봉투와 텍스트: 생성 시 id·시각 기록, 디코딩의 id 미소비, 시계 교체, 로컬 시각, PeekHeader, 세 언어 ToString 일치, Python 예약어 속성
- 상호 운용: C++ 골든 파일을 C#·Python에서 읽고 동일 입력의 바이트 비교. 골든 파일은 DeliveryRoute, PizzaOrder, Heartbeat, DeliveryProof, Numbers, Pie, Menu, Node

map의 임의 순회 순서까지 언어 간 바이트 동일성을 보장하는 것은 아닙니다. 골든 시험은 해당 시험 데이터의 결과를 비교합니다. 성능 비교와 측정 조건은 [벤치마크](benchmark/README.ko.md)에 있습니다.
