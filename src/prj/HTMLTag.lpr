library HTMLTag;
{
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) Martijn Coppoolse <https://github.com/vor0nwe>
  Revisions copyright (c) 2022-2024 Robert Di Pardo <dipardo.r@gmail.com>
}
{$warn 2025 OFF}

uses
  Windows, DLLExports;

{$R *.res}
{$if NOT DECLARED(useheaptrace)}
  {$SetPEOptFlags $40}
{$endif}

exports
  setInfo, getName, getFuncsArray, beNotified, messageProc, isUnicode;

begin
  DLL_PROCESS_DETACH_Hook := @DLLEntryPoint;
  DLLEntryPoint(DLL_PROCESS_ATTACH);
end.
