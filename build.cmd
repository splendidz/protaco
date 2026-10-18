@echo off
rem build.cmd - build protacoc, regenerate code, run C++ (gtest) and C# tests, run the example.
rem ASCII only: cmd reads batch files in the OEM code page.
rem Env: MSBUILD (default: VS2017 Professional), GTEST_ROOT (googletest with include\ and lib\, /MD).
rem Without GTEST_ROOT the C++ tests are skipped.
setlocal
pushd "%~dp0"
if "%MSBUILD%"=="" set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\MSBuild.exe"
set CONFIG=Release
set PROTACOC=build\bin\protacoc.exe

echo === [1/6] build protacoc
"%MSBUILD%" src\protacoc\protacoc.csproj /p:Configuration=%CONFIG% /nologo /v:minimal || goto :fail

echo === [2/6] generate examples and test schemas
%PROTACOC% -I examples\schemas --cpp_out examples\gen\cpp --cs_out examples\gen\cs || goto :fail
for %%N in (ver_v1 ver_v2 alt partial deep build1 build2 build3 build4) do (
    %PROTACOC% -I tests\schemas\%%N --cpp_out tests\gen\cpp\%%N --cs_out tests\gen\cs\%%N --no_runtime || goto :fail
)

if "%GTEST_ROOT%"=="" (
    echo === [3/6] GTEST_ROOT not set: C++ tests skipped
    goto :example
)
echo === [3/6] build C++ tests (v141 x64)
"%MSBUILD%" tests\cpp\ProtacoTest.vcxproj /p:Configuration=%CONFIG% /p:Platform=x64 /nologo /v:minimal || goto :fail

echo === [4/6] run C++ tests
set "PROTACO_GOLDEN_DIR=%CD%\tests\golden"
build\bin\%CONFIG%\ProtacoTest.exe || goto :fail

:example
echo === [5/6] build and run the C++ example
"%MSBUILD%" examples\cpp\example.vcxproj /p:Configuration=%CONFIG% /p:Platform=x64 /nologo /v:minimal || goto :fail
build\bin\%CONFIG%\protaco_example.exe || goto :fail

echo === [6/6] build and run C# tests
"%MSBUILD%" tests\cs\protacoc.Tests.csproj /p:Configuration=%CONFIG% /nologo /v:minimal || goto :fail
build\bin\protacoc.Tests.exe "%CD%" || goto :fail

echo === ALL PASSED
popd
exit /b 0

:fail
echo === FAILED
popd
exit /b 1
