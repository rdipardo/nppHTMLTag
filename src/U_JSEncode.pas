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

{ ------------------------------------------------------------------------------------------------ }
procedure EncodeJS(Scope: TEntityReplacementScope = ersSelection);
var
  npp: TApplication;
  doc: TActiveDocument;
  DocIndex: Integer;
  Range: TTextRange;
  TargetText: WideString;
  MultiSel: Boolean;
  // ---------------------------------------------------------------------------------------------
  function DoEncode(var Text: WideString): Cardinal; overload;
  var
    CharPrefix: String;
    CharIndex, CharCode, EntitiesReplaced: Cardinal;
  begin
    EntitiesReplaced := 0;
    CharPrefix := U_Npp_HTMLTag.Npp.Options.UnicodePrefix;
    for CharIndex := Length(Text) downto 1 do begin
      CharCode := Ord(Text[CharIndex]);
      if CharCode > 127 then begin
        if MultiSel then begin
          doc.SelectMultiple(doc.Selection.StartPos + Pos(WideChar(CharCode), Text) - 1, doc.CharWidth);
          Text := WideFormat('%s%s', [CharPrefix, IntToHex(CharCode, 4)]);
        end else begin
          Text := Copy(Text, 1, CharIndex - 1)
                  + WideFormat('%s%s', [CharPrefix, IntToHex(CharCode, 4)])
                  + Copy(Text, CharIndex + 1);
        end;
        Inc(EntitiesReplaced);
      end;
    end;
    Result := EntitiesReplaced;
  end;
  function DoEncode: Cardinal; overload;
  begin
    TargetText := Range.Text;
    Result := DoEncode(TargetText);
    if Result > 0 then begin
      Range.Text := TargetText;
      Range.ClearSelection;
    end;
  end;
  // ---------------------------------------------------------------------------------------------
begin
  npp := GetApplication();

  case Scope of
    ersDocument: begin
      doc := npp.ActiveDocument;
      Range := doc.GetRange();
      try
        DoEncode;
      finally
        Range.Free;
      end;
    end;

    ersAllDocuments: begin
      for DocIndex := 0 to npp.Editors.Count - 1 do begin
        doc := npp.Editors[DocIndex];
        Range := doc.GetRange();
        try
          DoEncode;
        finally
          Range.Free;
        end;
      end;
    end;

    else begin // ersSelection
      doc := npp.ActiveDocument;
      TargetText := doc.Selection.Text;
      MultiSel := (doc.SelectionMode = smStreamMulti);
      if DoEncode(TargetText) > 0 then begin
        doc.ReplaceSelection(TargetText);
        doc.Selection.ClearSelection;
      end;
    end;
  end{case};
end{EncodeJS};

{ ------------------------------------------------------------------------------------------------ }
function DecodeJS(Scope: TEntityReplacementScope = ersSelection): Integer;
var
  npp: TApplication;
  doc: TActiveDocument;
  Target, Match, MatchNext: TTextRange;
  Pattern: WideString;
  ColumnSel, MultiSel: Boolean;
  LenPrefix: Sci_Position;
  HiByte, LoByte: Integer;
  EmojiChars: array [0..1] of WideChar;
begin
  Result := 0;

  npp := GetApplication();
  doc := npp.ActiveDocument;
  ColumnSel := (doc.SelectionMode = smColumn);
  MultiSel := (doc.SelectionMode = smStreamMulti);
  Target := TTextRange.Create(doc, doc.Selection.StartPos, doc.Selection.EndPos);
  Match := TTextRange.Create(doc);
  Pattern := UTF8Decode(U_Npp_HTMLTag.Npp.Options.UnicodeRE);
  LenPrefix := Length(U_Npp_HTMLTag.Npp.Options.UnicodePrefix);
  { NOTE: Carets don't line up correctly unless we search from the first match in the docuemnt(?) }
  if MultiSel then begin
    Target.StartPos := Pos(Copy(Target.Text, 0, 2*lenPrefix+8), doc.Text) - 1;
    Target.EndPos := doc.SendMessage(SCI_GETSELECTIONNEND, doc.SendMessage(SCI_GETSELECTIONS) - 1);
  end;
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
              EmojiChars[0] := WideChar(LoByte);
              EmojiChars[1] := WideChar(HiByte);
              if MultiSel then begin
                doc.SelectMultiple(Match.StartPos, Match.Length*2);
                doc.ReplaceSelection(WideCharToString(EmojiChars));
              end else begin
                MatchNext.Text := EmptyWideStr;
                Match.Text := WideCharToString(EmojiChars);
              end;

              if (Result < 1) then doc.Selection.StartPos := Match.StartPos;
            end;
          finally
            MatchNext.Free;
          end;
        end else
          if MultiSel then begin
            doc.SelectMultiple(Match.StartPos, Match.Length);
            doc.ReplaceSelection(WideChar(HiByte));
          end else
            Match.Text := WideChar(HiByte);

        if (Result < 1) then doc.Selection.StartPos := Match.StartPos;
        Inc(Result);
        if MultiSel then Break;
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

