# 벤치마크: protaco와 Protocol Buffers

[English](README.md) · [README로 돌아가기](../../README.ko.md#8-성능)

이 폴더에는 측정에 쓴 파일만 있습니다. 직접 돌려 보려면 빌드 환경은 따로 만들어야 합니다.

- [schemas/](schemas/): 메시지 3개의 protaco YAML.
- [bench.proto](bench.proto): 같은 메시지를 타입 그대로 옮긴 proto3.
- [bench.cpp](bench.cpp): 측정 코드. 두 라이브러리 모두 setter 이름이 `set_<필드>`라서, 템플릿 하나가 양쪽 메시지에 같은 값을 채웁니다.

## 메시지

- `Fixed512`: 고정 필드만, 고정 영역 512바이트. name 1, int64 4, int 32, double 32, bool 8, tag 1.
- `Fixed1K`: 고정 필드만, 고정 영역 1,024바이트. name 1, int64 8, int 64, double 64, bool 16, tag 1.
- `Mixed256`: 고정 영역 196바이트와 가변 `string` 필드 2개(24바이트, 16바이트, 키 포함 60바이트). 합계 256바이트.

타입 대응: `int` → `int32`, `int64` → `int64`, `double` → `double`, `bool` → `bool`, `string(N)`과 `string` → `string`. 모든 값이 기본값이 아니므로 proto3도 모든 필드를 기록합니다.

## 측정 항목

- 인코딩: 모든 필드를 set한 뒤 직렬화. protaco는 `Serialize()`, protobuf는 재사용 버퍼에 `SerializeToArray()`.
- 다시 직렬화: 값을 바꾸지 않고 같은 객체를 한 번 더 직렬화.
- 디코딩: 바이트를 객체로. protaco는 `TryToObject()`, protobuf는 재사용 메시지에 `ParseFromArray()`.

두 라이브러리가 권하는 대로 객체는 호출마다 재사용합니다. 항목마다 10,000회 예열 후 100,000회씩 20라운드를 돌리고, 호출 1회 평균 시간을 냅니다. 측정 전에 디코딩 결과가 같은 바이트로 돌아오는지 확인합니다.

## 결과

Intel Core i9-13900H, Windows 11, MSVC 14.16(v141) x64 `/O2 /MD`. protobuf 3.21.12(같은 도구 집합으로 빌드한 Release 정적 라이브러리), protaco 0.5.0.

| 메시지, 동작 | protaco / protobuf | protaco 소요 시간 |
|---|---|---|
| Fixed512 인코딩 | 20.6 / 214.8 ns | 90% 짧음(10.4배) |
| Fixed512 다시 직렬화 | 1.2 / 196.5 ns | 99% 짧음 |
| Fixed512 디코딩 | 61.6 / 240.1 ns | 74% 짧음(3.9배) |
| Fixed1K 인코딩 | 40.1 / 482.2 ns | 92% 짧음(12.0배) |
| Fixed1K 다시 직렬화 | 1.1 / 449.3 ns | 99% 짧음 |
| Fixed1K 디코딩 | 72.4 / 578.0 ns | 87% 짧음(8.0배) |
| Mixed256 인코딩 | 20.2 / 98.3 ns | 79% 짧음(4.9배) |
| Mixed256 다시 직렬화 | 1.3 / 80.0 ns | 98% 짧음 |
| Mixed256 디코딩 | 136.4 / 155.1 ns | 12% 짧음(1.1배) |

프레임 크기(protaco / protobuf): Fixed512 556 / 571 B, Fixed1K 1,067 / 1,132 B, Mixed256 300 / 268 B.

여러 번 돌리면 각 수치가 몇 % 안에서 움직였습니다. 수치는 장비마다 다릅니다.

## 결과 읽기

- 고정 필드: protaco setter가 프레임 버퍼에 바로 기록하므로 인코딩은 setter 호출이 전부입니다. protobuf는 직렬화할 때 필드마다 인코딩합니다.
- 다시 직렬화: 값이 그대로면 protaco(C++)는 마지막 바이트를 그대로 돌려줍니다. protobuf는 다시 인코딩합니다.
- 문자열이 있는 디코딩: protaco는 새 객체로 디코딩하므로, small-string 버퍼(MSVC 15바이트)보다 긴 `std::string`마다 할당이 생깁니다. protobuf는 파싱 대상 메시지의 문자열을 재사용합니다. 가변 필드가 커질수록 차이가 줄어듭니다.
- 크기: protobuf는 varint로 작은 수를 줄입니다. protaco는 고정 폭을 유지하고 프레임 머리에 메시지 이름을 싣습니다.

## 직접 돌리기

1. protobuf(측정에는 3.21.12)와 `protacoc`(저장소 루트의 `build.cmd`)를 빌드합니다.
2. `protacoc -I schemas --cpp_out gen`, `protoc --cpp_out=gen bench.proto`를 실행합니다.
3. `bench.cpp`와 `gen/bench.pb.cc`를 Release x64로 컴파일합니다. include 경로에 `gen`과 protobuf `src` 폴더를 넣고 `libprotobuf`를 링크합니다.
