[English](reference.md) · [README로 돌아가기](../README.ko.md)

# Protaco 상세 참조

protacoc 0.5.0과 직렬화 형식 v3의 상세 규칙입니다. 사용 흐름과 생성 API는 [README](../README.ko.md), 실행 예제는 [예제 설명](../examples/README.ko.md)에 있습니다.

1. [스키마 지문](#1-스키마-지문)
2. [명령행](#2-명령행)
3. [진단](#3-진단)
4. [빌드와 시험](#4-빌드와-시험)

## 1. 스키마 지문

지문은 version 하나의 정규 텍스트를 UTF-8로 인코딩한 바이트에 대한 32비트 FNV-1a 해시입니다. offset basis는 2166136261, prime은 16777619입니다.

```text
protaco-schema-1|<name>|<version>|<allow_extension 1 or 0>|<field>;<field>;...

fields  해당 version까지 추가된 필드, 직렬화 순서(added, 다음 선언 순)
field   <key>:<type>[!][=<default>]
        !          required
        =<default> YAML에 적힌 기본값, 따옴표 제외
        ~<key>:<type>   해당 version까지 제거된 고정 필드(바이트 유지)
        해당 version까지 제거된 가변 필드는 제외
type    bool int int64 double string string(N) bytes list<T>
        메시지: <Name>@<그 메시지 지문, 소문자 16진수 8자리>
```

예시는 `protaco-schema-1|Topping|1|1|name:string(24);grams:int=30;`입니다.

- `added`, `removed` 값 자체는 정규 텍스트에 들어가지 않습니다. 새 version을 추가해도 옛 version의 지문은 유지됩니다.
- 중첩 메시지는 `@` 뒤의 지문으로 반영합니다. 안쪽 메시지가 바뀌면 이를 포함하는 메시지들의 지문도 바뀝니다.
- C++은 `MsgSchemaHash()`, C#은 `MsgSchemaHash` 상수로 값을 제공합니다. protacoc가 클래스 docstring에도 기록합니다.
- 프레임은 `min_version`부터 `version`까지 각 version의 지문을 싣습니다. 수신 측은 자기 version의 지문을 비교합니다.

사용 규칙은 [버전 관리](../README.ko.md#7-버전-관리)에 있습니다.

## 2. 명령행

```text
protacoc -I <dir> [-I <dir>...] [--cpp_out <dir>] [--cs_out <dir>]
         [--no_runtime] [--check] [Message ...]
```

`--cpp_out`과 `--cs_out` 중 하나 이상이 필요합니다.

| 옵션 | 의미 |
|---|---|
| `-I <dir>`, `--proto_path <dir>` | 스키마 폴더, 여러 번 지정 가능. 모든 파일의 namespace는 동일 |
| `--cpp_out <dir>` | C++ 출력 폴더 |
| `--cs_out <dir>` | C# 출력 폴더 |
| `--no_runtime` | `Protaco.hpp`, `Protaco.cs`를 기록하지 않음 |
| `--check` | 파일을 쓰지 않고 예상 결과와 비교, 다르면 종료 코드 3 |
| `Message ...` | 지정한 메시지만 출력, 메시지 이름 또는 YAML 경로 |
| `-h`, `--help` | 도움말 |
| `--version` | 생성기 버전 |

namespace는 YAML에서 정합니다. 명령행의 namespace 지정은 지원하지 않습니다. 대상 메시지를 생략하면 입력 폴더의 모든 메시지를 생성합니다. 특정 메시지만 골라도 입력 전체를 읽고 검사하며, 참조하는 메시지 파일까지 자동으로 출력 대상으로 추가하지는 않습니다.

일반 생성과 파일을 쓰지 않는 검사의 예입니다.

```bat
build\bin\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs
build\bin\protacoc.exe -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs --check
```

런타임은 기본적으로 생성 파일 옆에 같은 버전으로 기록됩니다. `--no_runtime`을 쓰는 프로젝트는 같은 생성기 버전의 런타임을 별도로 제공해야 합니다.

## 3. 진단

### 3.1 오류 코드

protacoc는 `file(line,col): error CODE: message` 형식으로 출력합니다. Visual Studio 출력에서 해당 YAML 줄로 이동할 수 있습니다.

YAML 문법입니다.

- PC101: 지원하지 않는 YAML 구문.
- PC102: 흐름 문법 오류. 중괄호, 쉼표, 따옴표 오류와 지원하지 않는 `[...]` 흐름 목록이 해당합니다.

메시지 키와 속성입니다.

- PC201: 알 수 없는 최상위 키. 0.5.0에서 제거한 `owner`도 여기에 해당합니다. 문서용 키는 `desc` 하나입니다.
- PC202: 필수 키 누락. `message`, `version`, `namespace`, `fields`.
- PC203: 알 수 없는 필드 속성. 0.5.0에서 제거한 `eq`는 값이 흐름 목록(`[..]`)이라 구문 검사에서 먼저 PC102가 됩니다.
- PC204: 잘못된 값.
- PC205: 중복 속성.
- PC206: 잘못된 namespace.

필드와 version입니다.

- PC301: 메시지 이름과 파일 이름 불일치.
- PC302: 필드 키 중복.
- PC303: 예약어, 언어 키워드, 생성 멤버 이름과 충돌하는 키. `ext`, `buf`, `cache`, `serialize`, `tostring` 등.
- PC304: 잘못된 타입.
- PC305: 고정 필드에 `required` 지정.
- PC308: 잘못된 기본값. 타입·범위, 정수의 `+`와 앞자리 0, `int64` 최솟값, 비유한 `double`, 문자열의 ASCII·길이 제한 등을 생성 시점에 검사.
- PC310: `string(N)`의 잘못된 N.
- PC311: C# 속성 이름 충돌.
- PC312: 지원 version 범위 또는 `min_version`, `added`, `removed` 값의 범위 오류.
- PC313: `min_version` 이하에서 제거된 가변 필드. 해당 YAML 줄 삭제 필요.
- PC314: 메시지 이름이 `v<N>`. version namespace에 예약된 이름.

메시지와 참조입니다.

- PC401: 메시지 중복.
- PC402: 알 수 없는 메시지 참조.
- PC403: 참조 순환. 전체 참조 경로를 표시.
- PC404: 목록 원소로 사용할 수 없는 타입.
- PC405: 한 실행에 서로 다른 namespace 사용.
- PC406: 중첩 깊이 8 초과.
- PC407: 필드 타입으로 쓰는 메시지에 버전 기능(`min_version`, `added`, `removed`) 사용.

환경 오류입니다.

- PC501: 파일 입출력.
- PC502: 명령행.

### 3.2 종료 코드

| 코드 | 의미 |
|---|---|
| 0 | 성공 |
| 1 | 사용법 오류 |
| 2 | 스키마 오류 |
| 3 | `--check`에서 오래되거나 다른 파일 검출 |

### 3.3 디코딩 오류

C++은 `protaco::Error::what`, C#은 `ProtacoException.What`에서 이유를 확인합니다. 비교하는 값이 있는 오류에는 기대값과 받은 값이 표시됩니다. `path` 또는 `Path`는 `stops[2].orderId` 같은 필드 위치이며, 머리 오류라면 비어 있을 수 있습니다.

- `message name mismatch`: 메시지 이름 불일치.
- `message version too old`: 수신 클래스보다 옛 프레임. 해당 옛 클래스가 생성된 경우 사용할 타입 안내.
- `message version too new`: 수신 클래스가 송신 측 지원 version 범위에 없음. 송신 측 지원 범위 표시.
- `schema mismatch`: 지문 불일치. 양쪽 지문 표시.
- `fixed block size mismatch`: 고정 영역 크기 불일치. 양쪽 크기 표시.
- `missing required`: 필수 가변 필드 누락.
- `type mismatch`: 타입 불일치. 양쪽 타입 표시.
- `unknown key (allow_extension false)`: 닫힌 메시지의 허용되지 않는 키.
- `bad version range`: 잘못된 지원 version 범위.
- `nesting too deep`: 중첩 깊이 초과.
- 잘림 오류: 머리, 본문, 필드 등의 데이터 부족.

더 새 version 프레임의 본문을 읽다가 실패하면 끝에 `(frame version N read as M)`이 붙습니다. 예외와 반환값 방식의 사용법은 [생성 API](../README.ko.md#6-생성-api)에 있습니다.

## 4. 빌드와 시험

### 4.1 실행

저장소 루트의 명령 프롬프트에서 실행합니다.

```bat
set GTEST_ROOT=<googletest 폴더>
build.cmd
```

googletest 폴더에는 `include\`, `lib\gtest.lib`, `lib\gtest_main.lib`가 필요하며 `/MD`로 빌드한 라이브러리를 사용합니다. `GTEST_ROOT`가 없으면 C++ 시험 빌드와 실행을 생략합니다.

`build.cmd`의 단계입니다.

1. protacoc 빌드.
2. `examples/gen`, `tests/gen` 재생성.
3. C++ 시험 빌드.
4. 골든 파일을 사용하는 C++ 시험 수행.
5. C++ 예제 빌드와 실행.
6. C# 시험 빌드와 수행.

이 명령은 생성 코드를 다시 씁니다. 기준 프레임(`tests/golden`)은 `PROTACO_UPDATE_GOLDEN=1`일 때만 C++ 시험이 다시 기록하며, 직렬화 형식이 바뀔 때만 씁니다. `MSBUILD` 환경 변수로 MSBuild 경로를 바꿀 수 있습니다. 기본값은 Visual Studio 2017 Professional입니다. IDE에서는 [protaco.sln](../protaco.sln)을 엽니다.

### 4.2 저장소 구성

| 경로 | 내용 |
|---|---|
| [src/protacoc](../src/protacoc) | 생성기 |
| [runtime/cpp](../runtime/cpp), [runtime/cs](../runtime/cs) | 생성기가 복사하는 런타임 원본 |
| [examples](../examples/README.ko.md) | 예제 스키마, 생성 코드, C++ 프로그램 |
| [tests/schemas](../tests/schemas) | 버전 관리, 중첩, 순환, 오류 코드별 스키마 |
| [tests/golden](../tests/golden) | C++이 기록하고 C#과 비교하는 기준 프레임 |
| [tests/cpp](../tests/cpp) | googletest 시험과 C++14 번역 단위 |
| [tests/cs](../tests/cs) | 생성기와 C# 런타임 시험 실행기 |
| [benchmark](../benchmark/README.ko.md) | protobuf 비교 측정 자료 |
| [docs](.) | 상세 참조와 그림 |

기준 프레임은 `DeliveryRoute.bin`, `PizzaOrder.bin`, `Heartbeat.bin`, `DeliveryProof.bin`입니다.

### 4.3 시험 범위

C++ 시험은 32개입니다.

- 왕복, 고정 필드 직접 기록, 기본값.
- version·지문 불일치 거부.
- 옛 지문 유지, 새 프레임을 수신 측 version으로 해석, 옛 프레임을 옛 클래스로 디코딩, 송신 측 `min_version`이 수신 측보다 높은 경우.
- 닫힌 메시지의 모르는 키 처리와 더 새 필드 버림.
- 중첩 8단계, 필수 필드 누락, 중첩 오류 경로, 확장 데이터 규칙.
- `ToString()` 출력.
- 계약 검사 비용 측정. 라이브러리 간 성능 비교는 [벤치마크](../benchmark/README.ko.md) 참고.
- 모든 길이에서의 잘림, 무작위 비트 반전.
- 골든 바이트와 C++14 번역 단위.
- Serialize 재사용: 값 변경 시 재생성, `get_mutable_` 참조, 복사, ext 대입.
- 가변 필드가 16개를 넘는 본문 디코딩.

C# 시험은 27개입니다.

- 오류 코드별 잘못된 YAML, 순환, namespace 혼용, 중첩 깊이, 필드 타입 메시지의 버전 기능 제한.
- 저장소의 생성 결과와 직접 수정한 결과에 대한 `--check`, 입력 순서 무관.
- 정규 텍스트와 지문 비교.
- 왕복, 버전 관리, 닫힌 메시지, 잘림.
- C++ 골든 파일과 바이트 단위 일치, C++과 같은 `ToString()` 출력.
