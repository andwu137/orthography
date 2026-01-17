:: angn: i dont know if this script works lol

::@echo off

:: prepare
for %%a in (%*) do set "%%~a=1"

if not "%release%"=="1" set debug=1

:: command types
set compiler_libs=-Lvendor/raylib/src/ -lraylib -lmsvcrt -lraylib -lOpenGL32 -lGdi32 -lWinMM -lkernel32 -lshell32 -lUser32 -Xlinker /NODEFAULTLIB:libcmt
set compiler_common=-std=c23 -Wall -Wextra -Wpedantic -Wno-missing-braces -Wno-unused-function -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-unused-but-set-variable -Wno-initializer-overrides
set compiler_debug=call clang -g -O0 -DBUILD_DEBUG=1 %compiler_common%
set compiler_release=call clang -O2 -DBUILD_DEBUG=0 %compiler_common%

if "%debug%"=="1" set compiler=%compiler_debug%
if "%release%"=="1" set compiler=%compiler_release%

:: compile
%compiler% orthography.c %compiler_libs% -o orthography.exe