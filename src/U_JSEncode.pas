unit U_JSEncode;

interface
uses
  Classes,
  NppSimpleObjects;

type
  TEntityReplacementScope = (ersSelection, ersDocument, ersAllDocuments);

procedure EncodeJS(Scope: TEntityReplacementScope = ersSelection);
function  DecodeJS(Scope: TEntityReplacementScope = ersSelection): Integer;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation
uses
  SysUtils,
//  L_DebugLogger,
  SciSupport;

{ ------------------------------------------------------------------------------------------------ }
type
  TConversionMethod = function(var Text: WideString): Integer;
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
      for DocIndex := 0 to npp.Documents.Count - 1 do begin
        doc := npp.Documents[DocIndex].Activate;
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
              + WideFormat('\u%s', [IntToHex(CharCode, 4)])
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
  Target, Match: TTextRange;
begin
  Result := 0;

  npp := GetApplication();
  doc := npp.ActiveDocument;

  Target := TTextRange.Create(doc, doc.Selection.StartPos, doc.Selection.EndPos);
  Match := TTextRange.Create(doc);
  try
    repeat
      doc.Find('\\u[0-9A-F][0-9A-F][0-9A-F][0-9A-F]', Match, SCFIND_REGEXP, Target.StartPos, Target.EndPos);
      if Match.Length <> 0 then begin
        // Adjust the target already
        Target.StartPos := Match.StartPos + 1;

        // replace this match's text by the appropriate Unicode character
        Match.Text := WideChar(StrToInt(Format('$%s', [Copy(Match.Text, 3, 4)])));

        Inc(Result);
      end;
    until Match.Length = 0;
  finally
    Target.Free;
    Match.Free;
  end;
//DebugWrite('DecodeJS', Format('Result: %d replacements', [Result]));
end{DecodeJS};


////////////////////////////////////////////////////////////////////////////////////////////////////
initialization

finalization

end.

