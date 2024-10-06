@echo off
::
:: Copyright (c) 2023,2024 Robert Di Pardo <dipardo.r@gmail.com>
::
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this file,
:: You can obtain one at https://mozilla.org/MPL/2.0/.
::
SETLOCAL EnableDelayedExpansion

set "CMAKE_BUILD_TYPE=Debug"
set "CMAKE_GENERATOR_PLATFORM=x64"

if "%1" NEQ "" ( set "CMAKE_BUILD_TYPE=%1" )
if "%2" NEQ "" ( set "CMAKE_GENERATOR_PLATFORM=%2" )
goto :%CMAKE_BUILD_TYPE%

:Release
:MinSizeRel
del /S /Q /F out\*.zip 2>NUL:

:Debug
:RelWithDebInfo
if /I "%4"=="nmake" (
  set "NMAKE_BUILD=true"
)
if /I "%4"=="clang" (
  set "CMAKE_TOOL_CHAIN=-DCMAKE_CXX_COMPILER=clang++"
  set "NMAKE_BUILD=true"
)
if "%NMAKE_BUILD%" NEQ "" (
  set "CMAKE_GENERATOR=NMake Makefiles"
  set "CMAKE_GENERATOR_PLATFORM=%VSCMD_ARG_TGT_ARCH%"
  echo :: ===================================================
  echo :: NOTE: This environment only supports %VSCMD_ARG_TGT_ARCH% targets
  echo :: ===================================================
 ) else (
  if "%CMAKE_GENERATOR_PLATFORM%"=="x86" ( set "CMAKE_GENERATOR_PLATFORM=Win32" )
  set "CMAKE_GENERATOR=Visual Studio %VisualStudioVersion:.0=%"
  set "CONFIG_PARAMS=-A !CMAKE_GENERATOR_PLATFORM!"
  set "BUILD_PARAMS=--config %CMAKE_BUILD_TYPE%"
)

set "BUILD_DIR=build\!CMAKE_GENERATOR_PLATFORM!\%CMAKE_BUILD_TYPE%"

if /I "%3"=="clean" (
  del /S /Q "%BUILD_DIR%\CMakeCache.txt" 2>NUL:
  set "CLEAN_FIRST=--clean-first"
)

cmake -Hsrc\prj -B%BUILD_DIR% -G"!CMAKE_GENERATOR!" %CONFIG_PARAMS% %CMAKE_TOOL_CHAIN% ^
  -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -DTARGET_PLATFORM=!CMAKE_GENERATOR_PLATFORM!
if %errorlevel% NEQ 0 ( goto :END )
cmake --build %BUILD_DIR% %BUILD_PARAMS% %CLEAN_FIRST%
goto :END

:-?
:help
echo Usage: ".\%~n0 [Debug,Release,MinSizeRel,RelWithDebInfo] [x86,x64,ARM64] [clean] [nmake,clang]"

:END
exit /B %errorlevel%

ENDLOCAL
