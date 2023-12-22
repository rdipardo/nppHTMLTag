unit U_JSEncode;

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
  U_Entities,
  NppSimpleObjects;

type
  TEntityReplacementScope = U_Entities.TEntityReplacementScope;

procedure EncodeJS(Scope: TEntityReplacementScope = ersSelection);
function  DecodeJS(Scope: TEntityReplacementScope = ersSelection): Integer;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation
uses
  SysUtils,
  U_Npp_HTMLTag,
  NppPlugin;

{ ------------------------------------------------------------------------------------------------ }
type
  TTextRange = NppSimpleObjects.TTextRange;
  TRangeConversionMethod = function(const TextRange: TTextRange): Integer;

{ ------------------------------------------------------------------------------------------------ }
function PerformConversion(Conversion: TRangeConversionMethod; Scope: TEntityReplacementScope = ersSelection): Integer;
var
  npp: TApplication;
  doc: TActiveDocument;
  DocIndex: Integer;
  Range: TTextRange;
begin
  npp := GetApplication();

  Result := 0;
  case Scope of
    ersDocument: begin
      doc := npp.ActiveDocument;
      Range := doc.GetRange();
      try
        Result := Conversion(Range);
      finally
        Range.Free;
      end;
    end;

    ersAllDocuments: begin
      for DocIndex := 0 to npp.Editors.Count - 1 do begin
        doc := npp.Editors[DocIndex];
        Range := doc.GetRange();
        Result := Conversion(Range);
      end;
    end;

    else begin // ersSelection
      doc := npp.ActiveDocument;
      Range := doc.Selection;
      Result := Conversion(Range);
    end;
  end{case};
end {PerformConversion};

{ ------------------------------------------------------------------------------------------------ }
function DoEncodeJS(var Text: WideString): Integer; overload;
var
  CharIndex: Cardinal;
  CharCode: Cardinal;
  EntitiesReplaced: integer;
begin
  EntitiesReplaced := 0;

  for CharIndex := Length(Text) downto 1 do begin
    CharCode := Ord(Text[CharIndex]);
    if CharCode > 127 then begin
      Text := Copy(Text, 1, CharIndex - 1)
              + WideFormat('%s%s', [U_Npp_HTMLTag.Npp.Options.UnicodePrefix, IntToHex(CharCode, 4)])
              + Copy(Text, CharIndex + 1);
      Inc(EntitiesReplaced);
    end;
  end;
  Result := EntitiesReplaced;
end {DoEncodeJS};

{ ------------------------------------------------------------------------------------------------ }
function DoEncodeJS(const Range: TTextRange): Integer; overload;
var
  Text: WideString;
begin
  Text := Range.Text;
  Result := DoEncodeJS(Text);
  if Result > 0 then begin
    Range.Text := Text;
    Range.ClearSelection;
  end;
end{DoEncodeJS};

{ ------------------------------------------------------------------------------------------------ }
procedure EncodeJS(Scope: TEntityReplacementScope = ersSelection);
begin
  PerformConversion(DoEncodeJS, Scope);
end{EncodeJS};

{ ------------------------------------------------------------------------------------------------ }
function DecodeJS(Scope: TEntityReplacementScope = ersSelection): Integer;
var
  npp: TApplication;
  doc: TActiveDocument;
  Target, Match, MatchNext: TTextRange;
  Pattern: WideString;
  ColumnSel: Boolean;
  LenPrefix: Sci_Position;
  HiByte, LoByte: Integer;
  EmojiChars: array [0..1] of WideChar;
begin
  Result := 0;

  npp := GetApplication();
  doc := npp.ActiveDocument;
  ColumnSel := (doc.SendMessage(SCI_GETSELECTIONMODE) <> SC_SEL_STREAM);
  Target := TTextRange.Create(doc, doc.Selection.StartPos, doc.Selection.EndPos);
  Match := TTextRange.Create(doc);
  Pattern := UTF8Decode(U_Npp_HTMLTag.Npp.Options.UnicodeRE);
  LenPrefix := Length(U_Npp_HTMLTag.Npp.Options.UnicodePrefix);
  try
    repeat
      doc.Find(Pattern, Match, SCFIND_REGEXP, Target.StartPos, Target.EndPos);
      if Match.Length <> 0 then begin
        // Adjust the target already
        Target.StartPos := Match.StartPos + 1;

        // check if code point belongs to a multi-byte glyph
        if TryStrToInt(Format('$%s', [Copy(Match.Text, lenPrefix+1, 4)]), HiByte) and
           (HiByte >= $D800) and (HiByte <= $DBFF) then
        begin
          try
            MatchNext := TTextRange.Create(doc);
            doc.Find(Pattern, MatchNext, SCFIND_REGEXP, Match.EndPos-lenPrefix, Target.EndPos);
            if (MatchNext.Length <> 0) and TryStrToInt(Format('$%s', [Copy(MatchNext.Text, lenPrefix+1, 4)]), LoByte) then
            begin
              // erase tail character
              MatchNext.Text := EmptyWideStr;
              EmojiChars[0] := WideChar(LoByte);
              EmojiChars[1] := WideChar(HiByte);
              Match.Text := WideCharToString(EmojiChars);
              if (Result < 1) then doc.Selection.StartPos := Match.StartPos;
            end;
          finally
            MatchNext.Free;
          end;
        end else
          Match.Text := WideChar(HiByte);

        if (Result < 1) then doc.Selection.StartPos := Match.StartPos;
        Inc(Result);
      end;
    until (Match.Length = 0) or (ColumnSel and (Result = doc.SendMessage(SCI_GETSELECTIONS)));

    if Result > 0 then doc.Selection.ClearSelection;
  finally
    Target.Free;
    Match.Free;
  end;
end{DecodeJS};


////////////////////////////////////////////////////////////////////////////////////////////////////
initialization

finalization

end.

