# 벤치마크: protaco와 Protocol Buffers

[English](README.md) · [README로 돌아가기](../../README.ko.md#zero-copy라서-빠릅니다)

이 폴더에는 protaco 0.6.0과 protobuf 3.21.12를 같은 두 메시지로 비교한 코드와 측정 결과가 있습니다. 별도 빌드 환경을 준비해 실행합니다.

## 이 폴더에 있는 것

- [schemas/](schemas/): `Fixed512`와 `Mixed256`의 protaco YAML.
- [bench.proto](bench.proto): 같은 메시지의 타입별 proto3 정의.
- [bench.cpp](bench.cpp): 공통 setter 템플릿, 왕복 검사, 시간 측정, CSV 출력.
- [results.csv](results.csv): 공개한 측정값과 실행 환경.
- [plot.py](plot.py): `results.csv`를 읽어 그림 [benchmark.svg](../img/benchmark.svg)와 표 [benchmark_table.svg](../img/benchmark_table.svg)를 생성하는 스크립트. 두 README가 같은 영문 이미지를 씀.

## 메시지

- `Fixed512`: 고정 영역 512바이트. `name` 1개, `int64` 4개, `int` 32개, `double` 32개, `bool` 8개, `tag` 1개.
- `Mixed256`: 고정 영역 196바이트(`int64` 2개, `int` 16개, `double` 12개, `bool` 4개, `string(16)` 1개)와 가변 문자열 2개. 문자열 값은 24바이트와 16바이트이며, 키·타입·길이를 포함한 TLV는 60바이트로 합계 256바이트.

타입 대응은 `int` → `int32`, `int64` → `int64`, `double` → `double`, `bool` → `bool`, `string(N)`·`string` → `string`입니다. 두 YAML 모두 `allow_extension: false`입니다.

모든 필드에 setter를 호출합니다. 정수·실수·문자열은 proto3 기본값이 아닌 값을 사용합니다. bool은 `((v + n) & 1) != 0`으로 채우므로 절반이 `false`이며, proto3는 이 기본값 필드를 생략합니다. 따라서 proto3가 모든 필드를 기록하는 측정은 아닙니다.

## 측정 방법

- 인코딩: 모든 필드 setter 호출 후 직렬화. protaco는 `Serialize()`, protobuf는 재사용 버퍼에 `SerializeToArray()`.
- 디코딩: 바이트를 객체로 디코딩한 뒤 `i0` 필드 하나 읽기. protaco는 `TryToObject()`, protobuf는 재사용 메시지에 `ParseFromArray()`.
- TryView: `Fixed512` 프레임을 `TryToObject`와 같은 규칙으로 검사한 뒤, 객체 전체로 복사하지 않고 `i0` 필드 하나 읽기.

측정 항목마다 10,000회 warm-up 후 100,000회 호출을 20라운드 반복합니다. 각 라운드의 경과 시간을 호출 수로 나눈 값에서 평균과 표본 표준편차(sd)를 구합니다. `std::chrono::steady_clock`으로 시간을 재며, 입력과 결과에 volatile 값을 사용합니다. 공개 CSV는 세 번 실행한 결과에서 각 행의 평균 시간이 중앙값인 실행을 골랐습니다. 해당 실행의 평균과 sd를 함께 기록합니다.

두 라이브러리 모두 호출 사이에 송신 객체와 디코딩 출력 객체를 재사용합니다. 다만 protaco의 `TryToObject`는 내부에서 새 객체로 디코딩한 뒤 출력 객체에 옮깁니다. protobuf는 출력 메시지의 기존 저장 공간을 다시 사용합니다.

왕복 검사는 인코딩 측정 뒤, 디코딩 측정 전에 실행합니다. 디코딩 후 다시 직렬화한 바이트가 원본과 같은지 확인하며, 실패하면 종료합니다. TryView는 자체 시간 측정 전에 `i0` 값이 같은지 확인합니다.

## 결과

Intel Core i9-13900H, Windows 11, MSVC 14.16(v141) x64 `/O2 /MD` 환경입니다. protaco 0.6.0과 같은 도구 집합으로 빌드한 protobuf 3.21.12 Release 정적 라이브러리를 사용했습니다. 값의 원본은 [results.csv](results.csv)입니다.

![Fixed512와 Mixed256의 Encoding·Decoding 시간](../img/benchmark.svg)

막대는 호출당 평균 시간, error bar는 20라운드의 표준편차입니다. 괄호는 protobuf 시간을 protaco 시간으로 나눈 배율입니다. 패널마다 세로축 범위가 다릅니다.

### Fixed512

| 동작 | protaco | protobuf |
|---|---|---|
| 인코딩 | 23.3 ns (sd 0.2) | 200.1 ns (sd 2.5) |
| 디코딩 | 55.4 ns (sd 1.6) | 252.3 ns (sd 9.2) |
| 디코딩 (TryView) | 11.9 ns (sd 0.5) | — |

protobuf 대비 protaco 인코딩은 8.6배 빠르고(시간 88% 단축), `TryToObject` 디코딩은 4.6배 빠릅니다. TryView는 protobuf 객체 디코딩 시간의 1/21.2입니다.

### Mixed256

| 동작 | protaco | protobuf |
|---|---|---|
| 인코딩 | 20.7 ns (sd 0.5) | 93.3 ns (sd 3.4) |
| 디코딩 | 126.9 ns (sd 1.4) | 164.5 ns (sd 4.9) |

protobuf 대비 protaco 인코딩은 4.5배 빠르고(시간 78% 단축), 디코딩은 1.3배 빠릅니다(시간 23% 단축).

프레임 크기 참고(protaco / protobuf): Fixed512 556 / 571 B, Mixed256 300 / 268 B.

## 결과 읽기

고정 필드는 protaco setter가 프레임 버퍼에 바로 기록하므로 인코딩 작업은 setter 호출에 해당합니다. protobuf는 직렬화할 때 필드를 인코딩합니다.

TryView는 프레임 검사를 거쳐 필요한 필드만 읽습니다. 전체 객체를 만드는 디코딩과는 작업 범위가 다릅니다.

문자열이 있는 디코딩에서는 차이가 작습니다. protaco는 새 객체를 만들므로 MSVC의 15바이트 small-string 버퍼를 넘는 문자열마다 할당이 생깁니다. protobuf는 재사용 메시지의 문자열 저장 공간을 다시 사용합니다.

이 수치는 위 메시지·값·하드웨어·컴파일러에서 얻은 결과입니다. 다른 메시지나 환경의 성능을 보장하지 않습니다.

## 직접 돌리기

먼저 저장소 루트의 `build.cmd`로 protacoc를 빌드합니다. protobuf 3.21.12도 CMake로 빌드합니다. 측정 환경을 맞추려면 v141 x64, Release, 정적 libprotobuf, `/MD` 런타임을 선택하고 같은 버전의 protoc를 사용합니다.

x64 Visual Studio 개발자 명령 프롬프트에서 다음 환경 변수를 실제 경로로 설정합니다.

- `PROTOBUF_SRC`: protobuf 3.21.12 소스 루트.
- `PROTOBUF_LIB`: 빌드한 Release `libprotobuf.lib`의 전체 경로.
- `PROTOC`: 같은 빌드의 `protoc.exe` 전체 경로.

다음 명령은 저장소 루트에서 시작합니다. `gen/`은 protacoc가 생성합니다.

**Windows cmd**

```bat
cd docs\benchmark
..\..\bin\Release\protacoc.exe -I schemas --cpp_out gen
"%PROTOC%" --cpp_out=gen bench.proto
cl /nologo /std:c++17 /O2 /MD /EHsc /Igen /I"%PROTOBUF_SRC%\src" bench.cpp gen\bench.pb.cc /Febench.exe /link "%PROTOBUF_LIB%"
bench.exe
```

실행 출력에서 `csv,`로 시작하는 줄만 추려 접두사를 제거하고 CSV 헤더를 붙입니다. 같은 개발자 환경의 PowerShell에서 `docs/benchmark` 폴더를 작업 경로로 사용합니다. 아래 명령은 한 번의 측정 결과를 `results.csv`에 저장하고 그림을 갱신합니다. `python plot.py`에는 matplotlib이 필요합니다.

**PowerShell**

```powershell
$rows = .\bench.exe | Where-Object { $_.StartsWith("csv,") } | ForEach-Object { $_.Substring(4) }
@("message,operation,library,mean_ns,sd_ns,frame_bytes") + $rows | Set-Content -Encoding ascii results.csv
python plot.py
```

공개 결과와 같은 집계 방식으로 비교하려면 세 번의 실행을 각각 보관합니다. `message`·`operation`·`library`가 같은 행끼리 평균 시간의 중앙값 실행을 선택하고, 그 행의 sd와 프레임 크기도 함께 `results.csv`에 기록한 뒤 `python plot.py`를 실행합니다.
