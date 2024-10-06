@echo off
::
:: Copyright (c) 2022,2024 Robert Di Pardo <dipardo.r@gmail.com>
::
:: This Source Code Form is subject to the terms of the Mozilla Public License,
:: v. 2.0. If a copy of the MPL was not distributed with this file, You can
:: obtain one at http://mozilla.org/MPL/2.0/.
::
SETLOCAL

set "VERSION=1.5.0"
set "PLUGIN=HTMLTag"
set "PLUGIN_DLL=out\Win32\Release\%PLUGIN%.dll"
set "PLUGINX64_DLL=out\x64\Release\%PLUGIN%.dll"
set "PLUGINarm64_DLL=out\ARM64\Release\%PLUGIN%.dll"
set "PLUGIN_LEGACY_DLL=out\Win32\Release\%PLUGIN%_unicode.dll"
set "SLUG_SRC=out\%PLUGIN%_v%VERSION%"
set "SLUGX64_SRC=out\%PLUGIN%_v%VERSION%_x64"
set "SLUGarm64_SRC=out\%PLUGIN%_v%VERSION%_arm64"
set "SLUG=%SLUG_SRC%.zip"
set "SLUGX64=%SLUGX64_SRC%.zip"
set "SLUGarm64=%SLUGarm64_SRC%.zip"

call %~dp0build.cmd Release x86 clean
call %~dp0build.cmd Release x64
call %~dp0build.cmd Release ARM64
xcopy /DIY *.textile "out\Doc"

:: https://fossil.2of4.net/npp_htmltag/doc/trunk/doc/HTMLTag-readme.txt
echo F | xcopy /DV ".\%PLUGIN_DLL%" ".\%PLUGIN_LEGACY_DLL%"
7z a -tzip "%SLUG%" ".\%PLUGIN_LEGACY_DLL%" ".\dat\*entities.ini" ".\dat\*translations.ini" ".\out\Doc" -y
7z a -tzip "%SLUGX64%" ".\%PLUGINX64_DLL%" ".\dat\*entities.ini" ".\dat\*translations.ini" ".\out\Doc" -y
7z a -tzip "%SLUGarm64%" ".\%PLUGINarm64_DLL%" ".\dat\*entities.ini" ".\dat\*translations.ini" ".\out\Doc" -y

ENDLOCAL
