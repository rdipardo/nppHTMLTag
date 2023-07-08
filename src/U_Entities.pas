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
  NppSimpleObjects,
  U_Npp_HTMLTag;

type
  TEntityReplacementScope = (ersSelection, ersDocument, ersAllDocuments);
  TEntityReplacementOption = (eroEncodeLineBreaks);
  TEntityReplacementOptions = set of TEntityReplacementOption;

procedure EncodeEntities(const Scope: TEntityReplacementScope = ersSelection; const Options: TEntityReplacementOptions = []);
function  DoEncodeEntities(var Text: WideString; const Entities: TStringList; const Options: TEntityReplacementOptions): Integer;

function DecodeEntities(const Scope: TEntityReplacementScope = ersSelection): Integer;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation
uses
  SysUtils, Windows, StrUtils,
  NppPlugin,
  L_SpecialFolders;

var
  EntityLists: TStringList;
  MaxEntityLength: integer;

{ ------------------------------------------------------------------------------------------------ }
function LoadEntities(ANpp: TNppPluginHTMLTag; ASet: string = 'HTML 5'): TStringList;
var
  IniFile: WideString;
  Lines: TStringList;
  Line, Value, Entity: string;
  i, Separation, CodePoint: integer;
  Reading: boolean;
  ErrMsg: WideString;
begin
  if not Assigned(EntityLists) then begin
    EntityLists := TStringList.Create;
    EntityLists.CaseSensitive := False;
  end;

  i := EntityLists.IndexOf(ASet);
  if i >= 0 then begin
    Result := TStringList(EntityLists.Objects[i]);
  end else begin
    IniFile := ANpp.Entities;
    ErrMsg := WideFormat('%s must be saved in'#13#10'%s', [ExtractFileName(IniFile), ExtractFileDir(IniFile)]);
    if not FileExists(IniFile) then
      IniFile := Npp.DefaultEntitiesPath;
      ErrMsg := Concat(ErrMsg, WideFormat(#13#10'or %s in'#13#10'%s', [ExtractFileName(IniFile), TSpecialFolders.DLL]));
    if not FileExists(IniFile) then begin
      MessageBoxW(ANpp.App.WindowHandle, PWideChar(ErrMsg), PWideChar('Missing Entities File'), MB_ICONERROR);
      FreeAndNil(EntityLists);
      Result := nil;
      Exit;
    end else begin
      Result := TStringList.Create;
      Result.NameValueSeparator := '=';
      Result.CaseSensitive := True;
      Result.Duplicates := dupIgnore;

      EntityLists.AddObject(ASet, Result);

      Lines := TStringList.Create;
      try
        Lines.CaseSensitive := True;
        Lines.Duplicates := dupAccept;
        Lines.LoadFromFile(UTF8Encode(IniFile));

        Reading := False;
        for i := 0 to Lines.Count - 1 do begin
          Line := Trim(Lines[i]);
          Separation := Pos(';', Line);
          if Separation > 0 then begin
            Line := TrimRight(Copy(Line, 1, Separation - 1));
          end;
          if (Length(Line) > 0) and (Line[1] = '[') and (Line[Length(Line)] = ']') then begin
            // New section
            Value := Trim(Copy(Line, 2, Length(Line) - 2));
            Reading := SameText(Value, ASet);

          end else if Reading then begin
            // New entity?
            Separation := Pos('=', Line);
            if Separation > 0 then begin
              Entity := Copy(Line, 1, Separation - 1);
              if Length(Entity) > MaxEntityLength then begin
                MaxEntityLength := Length(Entity);
              end;

              Value := Trim(Copy(Line, Separation + 1));
              if TryStrToInt(Value, CodePoint) then begin
                Result.AddObject(Lines[i], TObject(CodePoint));
              end;
            end;
          end;
        end;
      finally
        FreeAndNil(Lines);
      end;

    end;
  end;
end{LoadEntities};


{ ------------------------------------------------------------------------------------------------ }
procedure EncodeEntities(const Scope: TEntityReplacementScope; const Options: TEntityReplacementOptions);
var
  doc: TActiveDocument;
  DocIndex: Integer;
  Text: WideString;
  { - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - }
  function FetchEntities: TStringList;
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
      for DocIndex := 0 to npp.App.Documents.Count - 1 do begin
        doc := npp.App.Documents[DocIndex].Activate;
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
        doc.Selection.Text := Text;
        doc.Selection.ClearSelection;
      end;
    end;
  end{case};
end{EncodeEntities};

{ ------------------------------------------------------------------------------------------------ }
function DoEncodeEntities(var Text: WideString; const Entities: TStringList; const Options: TEntityReplacementOptions): Integer;
var
  CharIndex, EntityIndex: integer;
  ReplaceEntity: boolean;
  EncodedEntity: WideString;
  EntitiesReplaced: integer;
begin
  EntitiesReplaced := 0;
  if not Assigned(Entities) then begin
    Result := EntitiesReplaced;
    Exit;
  end;

  EncodedEntity := '';
  for CharIndex := Length(Text) downto 1 do begin
    EntityIndex := Entities.IndexOfObject(TObject(integer(Ord(Text[CharIndex]))));
    if EntityIndex > -1 then begin
      ReplaceEntity := True;
      EncodedEntity := UTF8Decode(Entities.Names[EntityIndex]);
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
      Text := Copy(Text, 1, CharIndex - 1)
              + '&' + EncodedEntity + ';'
              + Copy(Text, CharIndex + 1);
      Inc(EntitiesReplaced);
    end;
  end;
  Result := EntitiesReplaced;
end;

{ ------------------------------------------------------------------------------------------------ }
function DecodeEntities(const Scope: TEntityReplacementScope = ersSelection): Integer;
const
  scDigits = '0123456789';
  scHexLetters = 'ABCDEFabcdef';
  scLetters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
var
  doc: TActiveDocument;
  Entities: TStringList;
  Text: WideString;
  CharIndex, EntityIndex: integer;
  EntitiesReplaced: integer;
  FirstPos, LastPos, NextIndex, i: Integer;
  IsNumeric, IsHex, IsValid: boolean;
  AllowedChars: WideString;
  Entity: string;
  CodePoint: integer;
begin
  EntitiesReplaced := 0;

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
        CodePoint := integer(Entities.Objects[EntityIndex]);
        IsValid := True;
      end else begin
        CodePoint := 0;
        IsValid := False;
      end;
    end;

    if IsValid then begin
      Text := Copy(Text, 1, FirstPos - 1)
              + WideChar(CodePoint)
              + Copy(Text, NextIndex);
      Dec(NextIndex, (LastPos - FirstPos + 1));
      Inc(EntitiesReplaced);
    end;

    CharIndex := PosEx('&', Text, NextIndex);
    if CharIndex = 0 then begin
      Break;
    end;
    CharIndex := CharIndex;
  end;

  if EntitiesReplaced > 0 then begin
    doc.Selection.Text := Text;
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
