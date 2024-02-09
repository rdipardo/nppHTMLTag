unit LocalizedNppPlugin;

(*
  Copyright (c) 2023 Robert Di Pardo <rob@bunsen.localhost>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.
*)

{$IFNDEF FPC}
{$MESSAGE FATAL 'This unit is only compatible with FPC.'}
{$ENDIF}

interface
uses NppPlugin;

type
  TLocalizedNppPlugin = class(TNppPlugin)
  private
    FLang: string;
    function GetNppConfigDir(Local: Boolean = True): WideString;
    function GetNppInstallDir: WideString;
    procedure SetLanguage(const LangFilePath: WideString);
  protected
    function GetMessage(const Key: string): WideString; virtual; abstract;
  public
    constructor Create;
    property Language: string read FLang;
  end;

implementation
uses
  Classes, SysUtils, laz2_DOM, laz2_XMLRead, ModulePath;

function GetEnvVar(const AVar: String): WideString;
begin
  Result := SysUtils.GetEnvironmentVariable(AVar);
end;

constructor TLocalizedNppPlugin.Create;
var
  IsPortable: Boolean;
begin
  inherited;
  IsPortable := not ((WideCompareText(GetNppInstallDir, GetEnvVar('ProgramFiles')) = 0) or
                     (WideCompareText(GetNppInstallDir, GetEnvVar('ProgramFiles(x86)')) = 0));
  SetLanguage(GetNppConfigDir(IsPortable) + 'nativeLang.xml');
end;

procedure TLocalizedNppPlugin.SetLanguage(const LangFilePath: WideString);
var
  Doc: TXMLDocument;
  Root, Node: TDOMNode;
  hXMLFile: THandle;
  fStream: TStream;
begin
  FLang := 'default';
  Doc := Nil;
  fStream := Nil;
  try
    hXMLFile := FileOpen(LangFilePath, fmOpenRead);
    if hXMLFile <> THandle(-1) then
    begin
      fStream := THandleStream.Create(hXMLFile);
      ReadXMLFile(Doc, fStream);
      if Assigned(Doc) then
      begin
        Root := Doc.DocumentElement.FirstChild;
        while (Assigned(Root) and not Assigned(Root.Attributes)) do
          Root := Root.NextSibling;
        Node := Root.Attributes.GetNamedItem('filename');
        if Assigned(Node) then
          FLang := ChangeFileExt(UTF8Encode(Node.NodeValue), EmptyStr);
      end;
    end;
  finally
    FileClose(hXMLFile);
    if Assigned(Doc) then
      FreeAndNil(Doc);
    if Assigned(fStream) then
      FreeAndNil(fStream);
  end;
end;

function TLocalizedNppPlugin.GetNppConfigDir(Local: Boolean): WideString;
var
  NppDir: WideString;
begin
  if Local then
    NppDir := ExtractFileDir(ExtractFileDir(ExtractFileDir(TModulePath.DLL)))
  else
    NppDir := (WideFormat('%s%s%s', [(GetEnvVar('AppData')), PathDelim, 'Notepad++']));

  Result := IncludeTrailingPathDelimiter(NppDir);
end;

function TLocalizedNppPlugin.GetNppInstallDir: WideString;
begin
  Result := ExtractfileDir(ExtractFileDir(
    IncludeTrailingPathDelimiter(ExtractFileDir(ExtractFileDir(ExtractFileDir(TModulePath.DLL))))));
end;

end.
