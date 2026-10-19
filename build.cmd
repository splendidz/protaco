@echo off
rem build.cmd - build protacoc, regenerate test and example code, run the C++ and C# tests, build and run the examples.
rem ASCII only: cmd reads batch files in the OEM code page.
rem Env: MSBUILD (default: VS2017 Professional). Output: bin\Release. Intermediates: .IntermediateDirectory.
setlocal
pushd "%~dp0"
if "%MSBUILD%"=="" set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe"
set CONFIG=Release
set BIN=bin\%CONFIG%
set PROTACOC=%BIN%\protacoc.exe
set TESTS=protaco_compiler\tests

echo === [1/5] build protacoc
"%MSBUILD%" protaco_compiler\protacoc\protacoc.csproj /p:Configuration=%CONFIG% /nologo /v:minimal || goto :fail

echo === [2/5] generate test and example code
for %%N in (demo types ver_v1 ver_v2 alt partial deep build1 build2 build3 build4) do (
    %PROTACOC% -I %TESTS%\schemas\%%N --cpp_out %TESTS%\gen\cpp\%%N --cs_out %TESTS%\gen\cs\%%N --no_runtime || goto :fail
)
%PROTACOC% -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs || goto :fail

echo === [3/5] build and run C++ tests (v141 x64)
"%MSBUILD%" %TESTS%\cpp\ProtacoTest.vcxproj /p:Configuration=%CONFIG% /p:Platform=x64 /nologo /v:minimal || goto :fail
set "PROTACO_GOLDEN_DIR=%CD%\%TESTS%\golden"
%BIN%\ProtacoTest.exe || goto :fail

echo === [4/5] build and run C# tests
"%MSBUILD%" %TESTS%\cs\protacoc.Tests.csproj /p:Configuration=%CONFIG% /nologo /v:minimal || goto :fail
%BIN%\protacoc.Tests.exe "%CD%\%TESTS%" || goto :fail

echo === [5/5] build and run the examples
"%MSBUILD%" examples\examples.sln /p:Configuration=%CONFIG% /p:Platform=x64 /nologo /v:minimal || goto :fail
%BIN%\protaco_example.exe || goto :fail
%BIN%\protaco_example_cs.exe || goto :fail

echo === ALL PASSED
popd
exit /b 0

:fail
echo === FAILED
popd
exit /b 1
