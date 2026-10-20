@echo off
rem regress.cmd - compares the working tree with an older commit on the same messages, protaco only: C++, C# and Python.
rem Run in an x64 Visual Studio developer command prompt: docs\benchmark\regress\regress.cmd <git ref> [runs]
rem The ref needs docs\benchmark\schemas, examples\schemas, include\ and protaco_compiler\protacoc. Default: 5 runs per side.
rem Needs git and python (PYTHON, default python) on PATH, and the Windows tar. Work files: .IntermediateDirectory\regress.
rem ASCII only: cmd reads batch files in the OEM code page.
setlocal
if "%~1"=="" (
    echo usage: regress.cmd ^<git ref^> [runs]
    exit /b 1
)
set REF=%~1
set RUNS=%~2
if "%RUNS%"=="" set RUNS=5
if "%PYTHON%"=="" set PYTHON=python
pushd "%~dp0..\..\.."
set ROOT=%CD%
set HERE=%ROOT%\docs\benchmark\regress
set WORK=%ROOT%\.IntermediateDirectory\regress
set OLD=%WORK%\old
if exist "%OLD%" rmdir /s /q "%OLD%"
mkdir "%OLD%" || goto :fail
del /q "%WORK%\*_old_*.txt" "%WORK%\*_new_*.txt" 2>nul

echo === [1/5] extract %REF% and build both generators
git archive %REF% | "%SystemRoot%\System32\tar.exe" -x -f - -C "%OLD%" || goto :fail
msbuild "%OLD%\protaco_compiler\protacoc\protacoc.csproj" /p:Configuration=Release /nologo /v:minimal || goto :fail
msbuild "%ROOT%\protaco_compiler\protacoc\protacoc.csproj" /p:Configuration=Release /nologo /v:minimal || goto :fail

echo === [2/5] generate the messages with each generator
call :generate "%OLD%" "%OLD%\bin\Release\protacoc.exe" "%WORK%\old_gen" || goto :fail
call :generate "%ROOT%" "%ROOT%\bin\Release\protacoc.exe" "%WORK%\new_gen" || goto :fail

echo === [3/5] build C++ (v141 x64 /O2 /MD) and C# against each runtime
cl /nologo /std:c++17 /O2 /MD /EHsc /W3 /I "%OLD%\include\cpp" /I "%WORK%\old_gen\cpp" "%HERE%\regress.cpp" /Fo"%WORK%\regress_old.obj" /Fe"%WORK%\regress_old.exe" || goto :fail
cl /nologo /std:c++17 /O2 /MD /EHsc /W3 /I "%ROOT%\include\cpp" /I "%WORK%\new_gen\cpp" "%HERE%\regress.cpp" /Fo"%WORK%\regress_new.obj" /Fe"%WORK%\regress_new.exe" || goto :fail
msbuild "%HERE%\regress.csproj" /p:Variant=old "/p:RuntimeCs=%OLD%\include\cs\Protaco.cs" "/p:GenDir=%WORK%\old_gen\cs" "/p:WorkDir=%WORK%" /nologo /v:minimal || goto :fail
msbuild "%HERE%\regress.csproj" /p:Variant=new "/p:RuntimeCs=%ROOT%\include\cs\Protaco.cs" "/p:GenDir=%WORK%\new_gen\cs" "/p:WorkDir=%WORK%" /nologo /v:minimal || goto :fail

echo === [4/5] run old and new in turn, %RUNS% times each
for /l %%i in (1,1,%RUNS%) do (
    "%WORK%\regress_old.exe" > "%WORK%\cpp_old_%%i.txt" || goto :fail
    "%WORK%\regress_new.exe" > "%WORK%\cpp_new_%%i.txt" || goto :fail
    "%WORK%\cs\old\regress_old.exe" > "%WORK%\cs_old_%%i.txt" || goto :fail
    "%WORK%\cs\new\regress_new.exe" > "%WORK%\cs_new_%%i.txt" || goto :fail
    "%PYTHON%" -I "%HERE%\regress.py" "%OLD%\include\python" "%WORK%\old_gen\py" > "%WORK%\py_old_%%i.txt" || goto :fail
    "%PYTHON%" -I "%HERE%\regress.py" "%ROOT%\include\python" "%WORK%\new_gen\py" > "%WORK%\py_new_%%i.txt" || goto :fail
)

echo === [5/5] median of the runs, change from %REF% to the working tree
"%PYTHON%" -I "%HERE%\compare.py" "%WORK%" || goto :fail
popd
exit /b 0

:fail
echo === FAILED
popd
exit /b 1

rem :generate <tree> <protacoc.exe> <output root>: the benchmark messages and the example messages of one tree.
:generate
"%~2" -I "%~1\docs\benchmark\schemas" --cpp_out "%~3\cpp\bench" --cs_out "%~3\cs\bench" --py_out "%~3\py" --no_runtime || exit /b 1
"%~2" -I "%~1\examples\schemas" --cpp_out "%~3\cpp\demo" --cs_out "%~3\cs\demo" --py_out "%~3\py" --no_runtime || exit /b 1
exit /b 0
