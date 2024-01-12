@echo off
::
:: Copyright (c) 2022 Robert Di Pardo <dipardo.r@gmail.com>
::
:: This Source Code Form is subject to the terms of the Mozilla Public License,
:: v. 2.0. If a copy of the MPL was not distributed with this file, You can
:: obtain one at http://mozilla.org/MPL/2.0/.
::
SETLOCAL

set "VERSION=1.4.3"
set "PLUGIN=HTMLTag"
set "PLUGIN_DLL=out\i386-win32\Release\%PLUGIN%.dll"
set "PLUGINX64_DLL=out\x86_64-win64\Release\%PLUGIN%.dll"
set "PLUGIN_LEGACY_DLL=out\i386-win32\Release\%PLUGIN%_unicode.dll"
set "SLUG_SRC=out\%PLUGIN%_v%VERSION%"
set "SLUGX64_SRC=out\%PLUGIN%_v%VERSION%_x64"
set "SLUG=%SLUG_SRC%.zip"
set "SLUGX64=%SLUGX64_SRC%.zip"

call %~dp0build.cmd Release i386 clean
if %errorlevel% NEQ 0 (
	exit /B %errorlevel%
)
call %~dp0build.cmd Release x86_64
xcopy /DIY *.textile "out\Doc"

:: https://fossil.2of4.net/npp_htmltag/doc/trunk/doc/HTMLTag-readme.txt
echo F | xcopy /DV ".\%PLUGIN_DLL%" ".\%PLUGIN_LEGACY_DLL%"
upx.exe --best -q ".\%PLUGIN_LEGACY_DLL%"
upx.exe --best -q ".\%PLUGINX64_DLL%"
7z a -tzip "%SLUG%" ".\%PLUGIN_LEGACY_DLL%" ".\dat\*entities.ini" ".\out\Doc" -y
7z a -tzip "%SLUGX64%" ".\%PLUGINX64_DLL%" ".\dat\*entities.ini" ".\out\Doc" -y

ENDLOCAL
