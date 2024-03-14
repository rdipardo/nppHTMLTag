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
    function GetNativeLangDir: WideString;
  protected
    procedure SetLanguage;
    function GetMessage(const Key: string): WideString; virtual; abstract;
  public
    constructor Create;
    property Language: string read FLang;
  end;

implementation
uses
  Classes, SysUtils, laz2_DOM, laz2_XMLRead;

constructor TLocalizedNppPlugin.Create;
begin
  inherited;
  FLang := 'default';
end;

procedure TLocalizedNppPlugin.SetLanguage;
var
  Doc: TXMLDocument;
  Root, Node: TDOMNode;
  hXMLFile: THandle;
  fStream: TStream;
begin
  Doc := Nil;
  fStream := Nil;
  try
    hXMLFile := FileOpen(GetNativeLangDir() + 'nativeLang.xml', fmOpenRead);
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

function TLocalizedNppPlugin.GetNativeLangDir: WideString;
begin
  Result := EmptyWideStr;
  if Self.NppData.NppHandle <> 0 then
    Result := IncludeTrailingPathDelimiter(ExtractFileDir(ExtractFileDir(Self.GetPluginsConfigDir)));
end;

end.
