{
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/.

    Copyright (c) Martijn Coppoolse <https://sourceforge.net/u/vor0nwe>
    Revisions copyright (c) 2022 Robert Di Pardo <dipardo.r@gmail.com>

    Alternatively, the contents of this file may be used under the terms
    of the GNU General Public License Version 2 or later, as described below:

    This file is part of DBGP Plugin for Notepad++
    Copyright (C) 2007  Damjan Zobo Cvetko

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
}

unit nppplugin;

{$IFDEF FPC}{$mode delphiunicode}{$ENDIF}

interface

uses
  Windows,Messages,SysUtils;

{$I '..\Include\SciSupport.inc'}
{$I '..\Include\SciApi.inc'}
{$I '..\Include\Npp.inc'}

  TNppPlugin = class(TObject)
  private
    FClosingBufferID: THandle;
    FConfigDir: string;
    FIsAutoCompletionCandidate: Boolean;
    function IsDarkModeEnabled: Boolean;
    function MinSubsystemIsVista: Boolean;
  protected
    PluginName: nppString;
    FuncArray: array of _TFuncItem;
    function SupportsBigFiles: Boolean;
    function HasV5Apis: Boolean;
    function HasFullRangeApis: Boolean;
    function GetApiLevel: TSciApiLevel;
    function GetNppVersion: Cardinal;
    function GetPluginsConfigDir: string;
    function AddFuncSeparator: Integer;
    function AddFuncItem(Name: nppString; Func: PFUNCPLUGINCMD): Integer; overload;
    function AddFuncItem(Name: nppString; Func: PFUNCPLUGINCMD; ShortcutKey: PShortcutKey): Integer; overload;
    function MakeShortcutKey(const Ctrl, Alt, Shift: Boolean; const AKey: AnsiChar): PShortcutKey;
  public
    NppData: TNppData;
    constructor Create;
    destructor Destroy; override;
    function CmdIdFromDlgId(DlgId: Integer): Integer;

    // Dialogs need extra padding since v8.4.9
    // https://github.com/notepad-plus-plus/notepad-plus-plus/pull/13054#issuecomment-1419562836
    function HasNarrowDialogBorders: Boolean;

    // needed for DLL export.. wrappers are in the main dll file.
    procedure SetInfo(NppData: TNppData); virtual;
    function GetName: nppPChar;
    function GetFuncsArray(var FuncsCount: Integer): Pointer;
    procedure BeNotified(sn: PSCNotification);
    procedure MessageProc(var Msg: TMessage); virtual;

    // hooks
    procedure DoNppnToolbarModification; virtual;
    procedure DoNppnShutdown; virtual;
    procedure DoNppnBufferActivated(const BufferID: THandle); virtual;
    procedure DoNppnFileClosed(const BufferID: THandle); virtual;
    procedure DoAutoCSelection(const hwnd: HWND; const StartPos: Sci_Position; ListItem: nppPChar); virtual; abstract;
    procedure DoCharAdded(const hwnd: HWND; const ch: Integer); virtual; abstract;
    procedure DoUpdateUI(const hwnd: HWND; const updated: Integer); virtual;
    procedure DoModified(const hwnd: HWND; const modificationType: Integer); virtual;

    // df
    function DoOpen(filename: String): boolean; overload;
    function DoOpen(filename: String; Line: Sci_Position): boolean; overload;
    procedure GetFileLine(var filename: String; var Line: Sci_Position);
    function GetWord: string;

    // helpers
    property ConfigDir: string  read GetPluginsConfigDir;
    property DarkModeEnabled: Boolean read IsDarkModeEnabled;
  end;


implementation

{ TNppPlugin }

constructor TNppPlugin.Create;
begin
  inherited;
end;

destructor TNppPlugin.Destroy;
var i: Integer;
begin
  for i:=0 to Length(self.FuncArray)-1 do
  begin
    if (self.FuncArray[i].ShortcutKey <> nil) then
    begin
      Dispose(self.FuncArray[i].ShortcutKey);
    end;
  end;
  inherited;
end;

function TNppPlugin.AddFuncItem(Name: nppString; Func: PFUNCPLUGINCMD): Integer;
var
  i: Integer;
begin
  i := Length(self.FuncArray);
  SetLength(self.FuncArray, i + 1);
  StrPLCopy(self.FuncArray[i].ItemName, Name, 1000);
  self.FuncArray[i].Func := Func;
  self.FuncArray[i].ShortcutKey := nil;
  Result := i;
end;

function TNppPlugin.AddFuncItem(Name: nppString; Func: PFUNCPLUGINCMD;
  ShortcutKey: PShortcutKey): Integer;
var
  i: Integer;
begin
  i := self.AddFuncItem(Name, Func);
  self.FuncArray[i].ShortcutKey := ShortcutKey;
  Result := i;
end;

function TNppPlugin.MakeShortcutKey(const Ctrl, Alt, Shift: Boolean; const AKey: AnsiChar): PShortcutKey;
begin
  Result := New(PShortcutKey);
  with Result^ do
  begin
    IsCtrl := Ctrl;
    IsAlt := Alt;
    IsShift := Shift;
    Key := AKey;
  end;
end;

function TNppPlugin.AddFuncSeparator: Integer;
begin
  Result := AddFuncItem('-', nil);
end;

procedure TNppPlugin.GetFileLine(var filename: String; var Line: Sci_Position);
var
  s: array[0..1001] of char;
  r: Sci_Position;
begin
  SendMessage(self.NppData.NppHandle, NPPM_GETFULLCURRENTPATH, 0, LPARAM(@s[0]));
  filename := string(s);

  r := SendMessage(self.NppData.nppScintillaMainHandle, SCI_GETCURRENTPOS, 0, 0);
  Line := SendMessage(self.NppData.nppScintillaSecondHandle, SCI_LINEFROMPOSITION, r, 0);
end;

function TNppPlugin.GetFuncsArray(var FuncsCount: Integer): Pointer;
begin
  FuncsCount := Length(self.FuncArray);
  Result := self.FuncArray;
end;

function TNppPlugin.GetName: nppPChar;
begin
  Result := nppPChar(self.PluginName);
end;

function TNppPlugin.GetPluginsConfigDir: string;
var
  path: array [0..1001] of char;
begin
  if Length(FConfigDir) = 0 then begin
    SendMessage(self.NppData.NppHandle, NPPM_GETPLUGINSCONFIGDIR, 1000, LPARAM(@path[0]));
    FConfigDir := string(path);
  end;
  Result := FConfigDir;
end;

procedure TNppPlugin.BeNotified(sn: PSCNotification);
begin
  try
    if HWND(sn^.nmhdr.hwndFrom) = self.NppData.NppHandle then begin
      case sn.nmhdr.code of
        NPPN_TB_MODIFICATION: begin
          self.DoNppnToolbarModification;
        end;
        NPPN_SHUTDOWN: begin
          self.DoNppnShutdown;
        end;
        NPPN_BUFFERACTIVATED: begin
          self.DoNppnBufferActivated(sn.nmhdr.idFrom);
        end;
        NPPN_FILEBEFORECLOSE: begin
          FClosingBufferID := SendMessage(HWND(sn.nmhdr.hwndFrom), NPPM_GETCURRENTBUFFERID, 0, 0);
        end;
        NPPN_FILECLOSED: begin
          self.DoNppnFileClosed(FClosingBufferID);
        end;
      end;
    end else begin
      case sn.nmhdr.code of
        { https://www.scintilla.org/ScintillaDoc.html#SCN_AUTOCSELECTIONCHANGE }
        SCN_AUTOCSELECTIONCHANGE: FIsAutoCompletionCandidate := (sn.listType = 0);
        SCN_USERLISTSELECTION: FIsAutoCompletionCandidate := False;
        SCN_AUTOCSELECTION: begin
          if FIsAutoCompletionCandidate then
            Self.DoAutoCSelection(HWND(sn.nmhdr.hwndFrom), sn.position, nppPChar(UTF8Decode(PAnsiChar(@sn.text[0]))));
        end;
        SCN_CHARADDED: begin
          if (sn.characterSource = SC_CHARACTERSOURCE_DIRECT_INPUT) then
            Self.DoCharAdded(HWND(sn.nmhdr.hwndFrom), sn.ch);
        end;
        SCN_MODIFIED: begin
          Self.DoModified(HWND(sn.nmhdr.hwndFrom), sn.modificationType);
        end;
        SCN_UPDATEUI: begin
          self.DoUpdateUI(HWND(sn.nmhdr.hwndFrom), sn.updated);
        end;
      end;
    end;
  except
    on E: Exception do begin
      OutputDebugString(PAnsiChar(UTF8Encode(WideFormat('%s> %s: "%s"', [PluginName, E.ClassName, E.Message]))));
    end;
  end;
end;

{$REGION 'Overrides'}
procedure TNppPlugin.MessageProc(var Msg: TMessage);
begin
  Dispatch(Msg);
end;

procedure TNppPlugin.SetInfo(NppData: TNppData);
begin
  self.NppData := NppData;
end;

procedure TNppPlugin.DoNppnShutdown;
begin
  // override these
end;

procedure TNppPlugin.DoNppnToolbarModification;
begin
  // override these
end;

procedure TNppPlugin.DoNppnBufferActivated(const BufferID: THandle);
begin
  // override these
end;

procedure TNppPlugin.DoNppnFileClosed(const BufferID: THandle);
begin
  // override these
end;

procedure TNppPlugin.DoModified(const hwnd: HWND; const modificationType: Integer);
begin
  // override these
end;

procedure TNppPlugin.DoUpdateUI(const hwnd: HWND; const updated: Integer);
begin
  // override these
end;
{$ENDREGION}

function TNppPlugin.GetWord: string;
var
  s: string;
begin
  s := '';
  SetLength(s, 800);
  SendMessage(self.NppData.NppHandle, NPPM_GETCURRENTWORD, 0, LPARAM(PChar(s)));
  Result := s;
end;

function TNppPlugin.DoOpen(filename: String): Boolean;
var
  r: Integer;
  s: array [0..1001] of char;
begin
  // ask if we are not already opened
  r := SendMessage(self.NppData.NppHandle, NPPM_GETFULLCURRENTPATH, 0,
    LPARAM(@s[0]));
  Result := true;
  if WideSameText(string(s), filename) then
    exit;
  r := SendMessage(self.NppData.NppHandle, WM_DOOPEN, 0,
    LPARAM(PChar(filename)));
  Result := (r = 0);
end;

function TNppPlugin.DoOpen(filename: String; Line: Sci_Position): Boolean;
var
  r: Boolean;
begin
  r := self.DoOpen(filename);
  if (r) then
    SendMessage(self.NppData.nppScintillaMainHandle, SCI_GOTOLINE, Line, 0);
  Result := r;
end;

function TNppPlugin.CmdIdFromDlgId(DlgId: Integer): Integer;
begin
  Result := self.FuncArray[DlgId].CmdId;
end;

function TNppPlugin.GetApiLevel: TSciApiLevel;
begin
  if (not self.HasV5Apis) then
    Result := sciApi_LT_5
  else if (not self.HasFullRangeApis) then
    Result := sciApi_GTE_515
  else
    Result := sciApi_GTE_523;
end;

function TNppPlugin.GetNppVersion: Cardinal;
var
  NppVersion: Cardinal;
begin
  NppVersion := SendMessage(self.NppData.NppHandle, NPPM_GETNPPVERSION, 0, 0);
  // retrieve the zero-padded version, if available
  // https://github.com/notepad-plus-plus/notepad-plus-plus/commit/ef609c896f209ecffd8130c3e3327ca8a8157e72
  if ((HIWORD(NppVersion) > 8) or
      ((HIWORD(NppVersion) = 8) and
        (((LOWORD(NppVersion) >= 41) and (not (LOWORD(NppVersion) in [191, 192, 193]))) or
          (LOWORD(NppVersion) in [5, 6, 7, 8, 9])))) then
    NppVersion := SendMessage(self.NppData.NppHandle, NPPM_GETNPPVERSION, 1, 0);

  Result := NppVersion;
end;

function TNppPlugin.SupportsBigFiles: Boolean;
var
  NppVersion: Cardinal;
begin
  NppVersion := GetNppVersion;
  Result :=
    (HIWORD(NppVersion) > 8) or
    ((HIWORD(NppVersion) = 8) and
      // 8.3 => 8,3 *not* 8,30
      ((LOWORD(NppVersion) in [3, 4]) or
       // Also check for N++ versions 8.1.9.1, 8.1.9.2 and 8.1.9.3
       ((LOWORD(NppVersion) > 21) and (not (LOWORD(NppVersion) in [191, 192, 193])))));
end;

function TNppPlugin.HasV5Apis: Boolean;
var
  NppVersion: Cardinal;
begin
  NppVersion := GetNppVersion;
  Result :=
    (HIWORD(NppVersion) > 8) or
    ((HIWORD(NppVersion) = 8) and
        ((LOWORD(NppVersion) >= 4) and
           (not (LOWORD(NppVersion) in [11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 31, 32, 33, 191, 192, 193]))));
end;

function TNppPlugin.HasFullRangeApis: Boolean;
var
  NppVersion: Cardinal;
begin
  NppVersion := GetNppVersion;
  Result :=
    (HIWORD(NppVersion) > 8) or
    ((HIWORD(NppVersion) = 8) and
       ((LOWORD(NppVersion) >= 43) and (not (LOWORD(NppVersion) in [191, 192, 193]))));
end;

function TNppPlugin.IsDarkModeEnabled: Boolean;
var
  NppVersion: Cardinal;
  HasQueryApi: Boolean;
begin
  NppVersion := GetNppVersion;
  HasQueryApi :=
    ((HIWORD(NppVersion) > 8) or
     ((HIWORD(NppVersion) = 8) and
        (((LOWORD(NppVersion) >= 41) and (not (LOWORD(NppVersion) in [191, 192, 193]))))));
  Result := (HasQueryApi and Boolean(SendMessage(self.NppData.NppHandle, NPPM_ISDARKMODEENABLED, 0, 0)));
end;

function TNppPlugin.HasNarrowDialogBorders: Boolean;
begin
    Result :=
      MinSubsystemIsVista and
        (TWinVer(SendMessage(self.NppData.NppHandle, NPPM_GETWINDOWSVERSION, 0, 0)) >= WV_WIN8);
end;

function TNppPlugin.MinSubsystemIsVista: Boolean;
var
  NppVersion: Cardinal;
begin
  NppVersion := GetNppVersion;
  Result :=
    ((HIWORD(NppVersion) > 8) or
     ((HIWORD(NppVersion) = 8) and
        (((LOWORD(NppVersion) >= 49) and (not (LOWORD(NppVersion) in [191, 192, 193]))))));
end;

end.
