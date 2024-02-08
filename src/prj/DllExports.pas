unit DLLExports;
{
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) Martijn Coppoolse <https://sourceforge.net/u/vor0nwe>
}
interface
  uses Windows, SysUtils, NppPlugin, U_Npp_HTMLTag;

function isUnicode : Boolean; cdecl;
function getName(): nppPchar; cdecl;
function getFuncsArray(var nFuncs:integer): Pointer; cdecl;
function messageProc(msg: Integer; _wParam: WPARAM; _lParam: LPARAM): LRESULT; cdecl;
procedure setInfo(NppData: TNppData); cdecl;
procedure beNotified(sn: PSciNotification); cdecl;
procedure DLLEntryPoint(dwReason: DWord);

implementation

procedure DLLEntryPoint(dwReason: DWord);
begin
  case dwReason of
  DLL_PROCESS_ATTACH:
  begin
  end;
  DLL_PROCESS_DETACH:
  begin
    try
      if Assigned(Npp) then
        Npp.Free;
    except
      ShowException(ExceptObject, ExceptAddr);
    end;
  end;
  end;
end;

procedure setInfo(NppData: TNppData); cdecl;
begin
  if Assigned(Npp) then
    Npp.SetInfo(NppData);
end;

function getName(): nppPchar; cdecl;
begin
  Result := nil;
  if Assigned(Npp) then
    Result := Npp.GetName;
end;

function getFuncsArray(var nFuncs:integer): Pointer; cdecl;
begin
  Result := nil;
  if Assigned(Npp) then
    Result := Npp.GetFuncsArray(nFuncs);
end;

procedure beNotified(sn: PSciNotification); cdecl;
begin
  if Assigned(Npp) then
    Npp.BeNotified(sn);
end;

function messageProc(msg: Integer; _wParam: WPARAM; _lParam: LPARAM): LRESULT; cdecl;
var xmsg:TMessage;
begin
  xmsg.Msg := msg;
  xmsg.WParam := _wParam;
  xmsg.LParam := _lParam;
  xmsg.Result := 0;
  if Assigned(Npp) then
    Npp.MessageProc(xmsg);
  Result := xmsg.Result;
end;

function isUnicode : Boolean; cdecl;
begin
  Result := true;
end;

end.
