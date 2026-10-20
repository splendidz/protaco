#!/bin/sh
# build.sh - build.cmd for Linux and macOS: protacoc on .NET 8, regenerate test and example code, run the C++, C# and
# Python tests, build and run the examples.
# Env: DOTNET (default: dotnet, SDK 8 or newer), CXX (default: g++, C++17), PYTHON (default: python3, 3.8 or newer).
# Output: bin/Release (C++), bin/Release/net8.0 (.NET). Intermediates: .IntermediateDirectory.
set -e
cd "$(dirname "$0")"
DOTNET=${DOTNET:-dotnet}
CXX=${CXX:-g++}
PYTHON=${PYTHON:-python3}
# The .NET apphosts (protacoc, protacoc.Tests) find the runtime here when dotnet is not installed system-wide.
DOTNET_ROOT=${DOTNET_ROOT:-$(dirname "$(readlink -f "$(command -v "$DOTNET")")")}
export DOTNET_ROOT
CONFIG=Release
BIN=bin/$CONFIG
NET=$BIN/net8.0
PROTACOC=$NET/protacoc
TESTS=protaco_compiler/tests
OBJ=.IntermediateDirectory/gcc/$CONFIG
GTEST=$TESTS/googletest
CXXFLAGS="-O2 -Wall -Wextra -Werror -pthread"

echo "=== [1/6] build protacoc (.NET 8)"
"$DOTNET" build protaco_compiler/protacoc/protacoc.net.csproj -c $CONFIG -nologo -v quiet

echo "=== [2/6] generate test and example code"
for n in demo scalars enums maps recursive arrays ver_v1 ver_v2 alt partial deep build1 build2 build3 build4; do
    $PROTACOC -I $TESTS/schemas/$n --cpp_out $TESTS/taco_generated/cpp/$n --cs_out $TESTS/taco_generated/cs/$n --py_out $TESTS/taco_generated/py --no_runtime
done
$PROTACOC -I examples/schemas --cpp_out examples/taco_generated/cpp --cs_out examples/taco_generated/cs --py_out examples/taco_generated/py

echo "=== [3/6] build and run C++ tests ($CXX)"
mkdir -p $OBJ $BIN
INC="-Iinclude/cpp -I$TESTS/taco_generated/cpp/demo -I$TESTS/taco_generated/cpp -isystem $GTEST/include -I$GTEST/src/gtest"
$CXX -std=c++17 $CXXFLAGS $INC -c $TESTS/cpp_tests/ProtacoTest.cpp -o $OBJ/ProtacoTest.o
$CXX -std=c++17 $CXXFLAGS $INC -c $TESTS/cpp_tests/Cpp14CheckTest.cpp -o $OBJ/Cpp14CheckTest.o
$CXX -std=c++14 $CXXFLAGS $INC -c $TESTS/cpp_tests/Cpp14Check.cpp -o $OBJ/Cpp14Check.o
$CXX -std=c++17 -O2 -w -pthread $INC -c $GTEST/src/gtest/gtest-all.cc -o $OBJ/gtest-all.o
$CXX -std=c++17 -O2 -w -pthread $INC -c $GTEST/src/gtest/src/gtest_main.cc -o $OBJ/gtest_main.o
$CXX -pthread $OBJ/ProtacoTest.o $OBJ/Cpp14CheckTest.o $OBJ/Cpp14Check.o $OBJ/gtest-all.o $OBJ/gtest_main.o -o $BIN/ProtacoTest
PROTACO_GOLDEN_DIR="$PWD/$TESTS/golden" $BIN/ProtacoTest

echo "=== [4/6] build and run C# tests (.NET 8)"
"$DOTNET" build $TESTS/cs_tests/protacoc.Tests.net.csproj -c $CONFIG -nologo -v quiet
$NET/protacoc.Tests "$PWD/$TESTS"

echo "=== [5/6] run Python tests"
"$PYTHON" $TESTS/py_tests/test_protaco.py "$PWD/$TESTS"

echo "=== [6/6] build and run the examples"
$CXX -std=c++17 $CXXFLAGS -Iexamples/taco_generated/cpp examples/cpp_examples/main.cpp -o $BIN/protaco_example
$BIN/protaco_example
"$DOTNET" build examples/cs_examples/example_cs.net.csproj -c $CONFIG -nologo -v quiet
$NET/protaco_example_cs
"$PYTHON" examples/python_examples/main.py

echo "=== ALL PASSED"
