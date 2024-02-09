unit U_Entities;

{
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) Martijn Coppoolse <https://sourceforge.net/u/vor0nwe>
  Revisions copyright (c) 2022 Robert Di Pardo <dipardo.r@gmail.com>
}

interface
uses
  Classes,
  IniFiles,
  NppSimpleObjects,
  U_Npp_HTMLTag;

type
  TEntityReplacementScope = (ersSelection, ersDocument, ersAllDocuments);
  TEntityReplacementOption = (eroEncodeLineBreaks);
  TEntityReplacementOptions = set of TEntityReplacementOption;

procedure EncodeEntities(Scope: TEntityReplacementScope = ersSelection; const Options: TEntityReplacementOptions = []);
function  DoEncodeEntities(var Text: WideString; const Entities: THashedStringList; const Options: TEntityReplacementOptions): Integer;

function DecodeEntities(Scope: TEntityReplacementScope = ersSelection): Integer;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation
uses
  SysUtils, Windows, StrUtils,
  NppPlugin,
  Utf8IniFiles,
  ModulePath;

var
  EntityLists: TStringList;

{ ------------------------------------------------------------------------------------------------ }
function LoadEntities(ANpp: TNppPluginHTMLTag; ASet: string = 'HTML 5'): THashedStringList;
var
  Mappings: TUtf8IniFile;
  IniFile: WideString;
  Lines: TStringList;
  Line, Value: string;
  i, CodePoint: integer;
  ErrMsg: WideString;
begin
  if not Assigned(EntityLists) then begin
    EntityLists := TStringList.Create;
    EntityLists.CaseSensitive := False;
  end;

  i := EntityLists.IndexOf(ASet);
  if i >= 0 then begin
    Result := THashedStringList(EntityLists.Objects[i]);
  end else begin
    IniFile := ANpp.Entities;
    ErrMsg := WideFormat('%s must be saved in'#13#10'%s', [ExtractFileName(IniFile), ExtractFileDir(IniFile)]);
    if not FileExists(IniFile) then
      IniFile := Npp.DefaultEntitiesPath;
      ErrMsg := Concat(ErrMsg, WideFormat(#13#10'or %s in'#13#10'%s', [ExtractFileName(IniFile), TModulePath.DLL]));
    if not FileExists(IniFile) then begin
      MessageBoxW(ANpp.App.WindowHandle, PWideChar(ErrMsg), PWideChar('Missing Entities File'), MB_ICONERROR);
      FreeAndNil(EntityLists);
      Result := nil;
      Exit;
    end else begin
      Result := THashedStringList.Create;
      Result.NameValueSeparator := '=';
      Result.CaseSensitive := True;
      Result.Duplicates := dupIgnore;

      EntityLists.AddObject(ASet, Result);

      Lines := TStringList.Create;
      try
        Lines.CaseSensitive := True;
        Lines.Duplicates := dupAccept;
        Mappings := TUtf8IniFile.Create(IniFile, [ifoCaseSensitive, ifoStripComments]);
        Mappings.ReadSection(ASet, Lines);
        for Line in Lines do begin
          Value := Mappings.ReadString(ASet, Line, EmptyStr);
          i := Pos(';', Value) - 1;
          if i <= 0 then i := Length(Value);
          if TryStrToInt(Trim(Copy(Value, 0, i)), CodePoint) then begin
            Result.AddPair(Line, IntToStr(CodePoint));
            Result.AddPair(IntToStr(CodePoint), Line);
          end;
        end;
      finally
        FreeAndNil(Lines);
        FreeAndNil(Mappings);
      end;

    end;
  end;
end{LoadEntities};


{ ------------------------------------------------------------------------------------------------ }
procedure EncodeEntities(Scope: TEntityReplacementScope; const Options: TEntityReplacementOptions);
var
  doc: TActiveDocument;
  DocIndex: Integer;
  Text: WideString;
  { - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - }
  function FetchEntities: THashedStringList;
  begin
    if doc.Language = L_XML then begin
      Result := LoadEntities(npp, 'XML');
    end else begin
      Result := LoadEntities(npp);
    end;
  end;
  { - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - }
begin

  case Scope of
    ersDocument: begin
      doc := npp.App.ActiveDocument;
      Text := doc.Text;
      if DoEncodeEntities(Text, FetchEntities, Options) > 0 then begin
        doc.Text := Text;
      end;
    end;

    ersAllDocuments: begin
      for DocIndex := 0 to npp.App.Editors.Count - 1 do begin
        doc := npp.App.Editors[DocIndex];
        Text := doc.Text;
        if DoEncodeEntities(Text, FetchEntities, Options) > 0 then begin
          doc.Text := Text;
        end;
      end;
    end;

    else begin // ersSelection
      doc := npp.App.ActiveDocument;
      Text := doc.Selection.Text;
      if DoEncodeEntities(Text, FetchEntities, Options) > 0 then begin
        doc.ReplaceSelection(Text);
        doc.Selection.ClearSelection;
      end;
    end;
  end{case};
end{EncodeEntities};

{ ------------------------------------------------------------------------------------------------ }
function DoEncodeEntities(var Text: WideString; const Entities: THashedStringList; const Options: TEntityReplacementOptions): Integer;
var
  Doc: TActiveDocument;
  CharIndex, EntityIndex: integer;
  ReplaceEntity, MultiSel: boolean;
  EncodedEntity: WideString;
  EntitiesReplaced: integer;
begin
  EntitiesReplaced := 0;
  if not Assigned(Entities) then begin
    Result := EntitiesReplaced;
    Exit;
  end;

  EncodedEntity := '';
  Doc := Npp.App.ActiveDocument;
  MultiSel := (doc.SelectionMode = smStreamMulti);
  for CharIndex := Length(Text) downto 1 do begin
    EntityIndex := Entities.IndexOfName(IntToStr(integer(Ord(Text[CharIndex]))));
    if EntityIndex > -1 then begin
      ReplaceEntity := True;
      EncodedEntity := UTF8Decode(Entities.ValueFromIndex[EntityIndex]);
    end else if Ord(Text[CharIndex]) > 127 then begin
      ReplaceEntity := True;
      EncodedEntity := WideFormat('#%s', [IntToStr(Ord(Text[CharIndex]))]);
    end else if (eroEncodeLineBreaks in Options) and (Ord(Text[CharIndex]) in [10, 13]) then begin
      ReplaceEntity := True;
      EncodedEntity := WideFormat('#%s', [IntToStr(Ord(Text[CharIndex]))]);
    end else begin
      ReplaceEntity := False;
    end;
    if ReplaceEntity then begin
      if MultiSel then begin
        doc.SelectMultiple(doc.Selection.StartPos + Pos(Text[CharIndex], Text) - 1, doc.CharWidth);
        Text := '&' + EncodedEntity + ';';
      end else begin
        Text := Copy(Text, 1, CharIndex - 1)
                + '&' + EncodedEntity + ';'
                + Copy(Text, CharIndex + 1);
      end;
      Inc(EntitiesReplaced);
      if MultiSel then Break;
    end;
  end;
  Result := EntitiesReplaced;
end;

{ ------------------------------------------------------------------------------------------------ }
function DecodeEntities(Scope: TEntityReplacementScope = ersSelection): Integer;
const
  scDigits = '0123456789';
  scHexLetters = 'ABCDEFabcdef';
  scLetters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
var
  doc: TActiveDocument;
  Entities: THashedStringList;
  Text: WideString;
  CharIndex, EntityIndex: integer;
  EntitiesReplaced: integer;
  FirstPos, LastPos, NextIndex, i: Integer;
  IsNumeric, IsHex, IsValid, MultiSel: boolean;
  AllowedChars: WideString;
  Entity: string;
  CodePoint: integer;
begin
  EntitiesReplaced := 0;
  Result := EntitiesReplaced;

  doc := npp.App.ActiveDocument;
  if doc.Language = L_XML then begin
    Entities := LoadEntities(npp, 'XML');
  end else begin
    Entities := LoadEntities(npp);
  end;

  if not Assigned(Entities) then
    Exit;

  Text := doc.Selection.Text;
  CharIndex := Pos('&', Text);

  // make sure the selection includes the semicolon
  if not (Pos(';', Text) > CharIndex) then
    Exit;

  MultiSel := (doc.SelectionMode = smStreamMulti);
  while CharIndex > 0 do begin
    FirstPos := CharIndex;
    LastPos := FirstPos;
    NextIndex := Length(Text) + 1;
    IsNumeric := False;
    IsHex := False;
    for i := 1 to Length(Text) - FirstPos do begin
      case i - 1 of
        0: begin
          AllowedChars := '#' + scLetters + ';';
        end;
        1: begin
          if Text[FirstPos + 1] = '#' then begin
            IsNumeric := True;
            AllowedChars := 'x' + scDigits;
          end else begin
            AllowedChars := scLetters;
          end;
        end;
        2: begin
          if IsNumeric then begin
            if Text[FirstPos + 2] = 'x' then begin
              IsHex := True;
              AllowedChars := scDigits + scHexLetters;
            end else begin
              AllowedChars := scDigits + ';';
            end;
          end else begin
            AllowedChars := scLetters + scDigits + ';';
          end;
        end;
        else begin
          if IsNumeric then begin
            if IsHex then begin
              AllowedChars := scDigits + scHexLetters + ';';
            end else begin
              AllowedChars := scDigits + ';';
            end;
          end else begin
            AllowedChars := scLetters + scDigits + ';';
          end;
        end;
      end;
      if Pos(Text[FirstPos + i], AllowedChars) = 0 then begin // stop! invalid char found
        LastPos := FirstPos + i - 1;
        NextIndex := FirstPos + i;
        Break;
      end else if Text[FirstPos + i] = ';' then begin // stop! end found
        LastPos := FirstPos + i - 1;
        NextIndex := FirstPos + i + 1;
        Break;
      end;
    end;

    if IsNumeric then begin
      if IsHex then begin
        IsValid := TryStrToInt(Format('$%s', [Copy(Text, FirstPos + 3, LastPos - FirstPos - 2)]), CodePoint);
      end else begin
        IsValid := TryStrToInt(Format('%s', [Copy(Text, FirstPos + 2, LastPos - FirstPos - 1)]), CodePoint);
      end;
    end else begin
      Entity := UTF8Encode(Copy(Text, FirstPos + 1, LastPos - FirstPos));
      EntityIndex := Entities.IndexOfName(Entity);
      if EntityIndex > -1 then begin
        CodePoint := StrToInt(Entities.ValueFromIndex[EntityIndex]);
        IsValid := True;
      end else begin
        CodePoint := 0;
        IsValid := False;
      end;
    end;

    if IsValid then begin
      if MultiSel then begin
        doc.SelectMultiple(doc.Selection.StartPos + FirstPos - 1, (LastPos - FirstPos) + 2);
        Text := WideChar(CodePoint);
      end else begin
        Text := Copy(Text, 1, FirstPos - 1)
                + WideChar(CodePoint)
                + Copy(Text, NextIndex);
        Dec(NextIndex, (LastPos - FirstPos + 1));
      end;
      Inc(EntitiesReplaced);
      if MultiSel then Break;
    end;

    CharIndex := PosEx('&', Text, NextIndex);
    if CharIndex = 0 then begin
      Break;
    end;
    CharIndex := CharIndex;
  end;

  if EntitiesReplaced > 0 then begin
    doc.ReplaceSelection(Text);
    doc.Selection.ClearSelection;
  end;

  Result := EntitiesReplaced;
end;

////////////////////////////////////////////////////////////////////////////////////////////////////
initialization

finalization
  if Assigned(EntityLists) then begin
    while EntityLists.Count > 0 do begin
      EntityLists.Objects[0].Free;
      EntityLists.Delete(0);
    end;
    FreeAndNil(EntityLists);
  end;

end.
