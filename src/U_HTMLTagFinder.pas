unit U_HTMLTagFinder;

interface
  uses
    NppSimpleObjects;

  procedure FindMatchingTag(ASelect: boolean = False; AContentsOnly: Boolean = False);

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation

uses
  SysUtils, Windows, Classes,
  SciSupport, nppplugin;

type
  TDirectionEnum = (dirBackward = -1, dirNone = 0, dirForward = 1, dirUnknown = 2);

const
  ncHighlightTimeout = 1000;
  scSelfClosingTags: array[0..12] of string = ('AREA', 'BASE', 'BASEFONT', 'BR', 'COL', 'FRAME',
                                                'HR', 'IMG', 'INPUT', 'ISINDEX', 'LINK', 'META',
                                                'PARAM');


{ ------------------------------------------------------------------------------------------------ }
function ExtractTagName(AView: TActiveDocument;
                        out ATagName: string;
                        out AOpening, AClosing: boolean;
                        APosition: Sci_Position = -1): TTextRange;
var
  {Tag, }TagEnd: TTextRange;
  i: Integer;
  StartIndex: integer;
  EndIndex: integer;
  {InnerLevel: integer;}
  ClosureFound: boolean;
  ExtraChar: AnsiChar;
begin
  ATagName := '';

  if (APosition < 0) then begin
    if (AView.CurrentPosition <= AView.Selection.Anchor) then begin
      APosition := AView.CurrentPosition + 1;
    end else begin
      APosition := AView.CurrentPosition;
    end;
  end;
  Result := AView.Find('<', 0, APosition, 0);
  if Result = nil then begin
//    DebugWrite('ExtractTagName', 'No start tag found before given position!');
    Result := AView.Find('<', 0, APosition);
    if Result = nil then begin
      ATagName := '';
      Exit;
    end;
  end;

  // Keep track of intermediate '<' and '>' levels, to accomodate <?PHP?> and <%ASP%> tags
  {InnerLevel := 0;}

  // TODO: search for '<' as well as '>';
  // - if '<' is before '>', then InnerLevel := InnerLevel + 1;
  // - else (if '>' is before '<', then)
  //   - if InnerLevel > 0 then InnerLevel := InnerLevel - 1;
  //   - else TagEnd has been found

  //DebugWrite('ExtractTagName', Format('Start of tag: (%d-%d): "%s"', [Tag.Start, Tag.&End, Tag.Text]));
  TagEnd := AView.Find('>', 0, Result.EndPos + 1);
  if TagEnd = nil then begin
    ATagName := '';
    Exit;
  end else begin
    //DebugWrite('ExtractTagName', Format('End of tag: (%d-%d): "%s"', [TagEnd.Start, TagEnd.&End, TagEnd.Text]));
    Result.EndPos := TagEnd.EndPos;
    FreeAndNil(TagEnd);
  end;

  // Determine the tag name, and whether it's an opening and/or closing tag
  AOpening := True;
  AClosing := False;
  ClosureFound := False;
  StartIndex := 0;
  EndIndex := 0;
  ATagName := Result.Text;
  ExtraChar := #0;
  for i := 2 to Length(ATagName) - 1 do begin
    if StartIndex = 0 then begin
      case ATagName[i] of
        '/': begin
          AOpening := False;
          AClosing := True;
        end;
        '0'..'9', 'A'..'Z', 'a'..'z', '-', '_', '.', ':': begin
          StartIndex := i;
        end;
      end;
    end else if EndIndex = 0 then begin
{$IFDEF UNICODE}
      if not CharInSet(ATagName[i], ['0'..'9', 'A'..'Z', 'a'..'z', '-', '_', '.', ':', ExtraChar]) then begin
{$ELSE}
      if not (ATagName[i] in ['0'..'9', 'A'..'Z', 'a'..'z', '-', '_', '.', ':', ExtraChar]) then begin
{$ENDIF}
        EndIndex := i - 1;
        if AClosing = True then begin
          break;
        end;
      end;
    end else begin
      if ATagName[i] = '/' then begin
        ClosureFound := True;
{$IFDEF UNICODE}
      end else if ClosureFound and not CharInSet(ATagName[i], [' ', #9, #13, #10]) then begin
{$ELSE}
      end else if ClosureFound and not (ATagName[i] in [' ', #9, #13, #10]) then begin
{$ENDIF}
        ClosureFound := False;
      end;
    end;
    //DebugWrite('ExtractTagName', Format('%d=%s; opens=%d,closes=%d; start=%d,end=%d', [i, ATagName[i], integer(AOpening), integer(AClosing or ClosureFound), StartIndex, EndIndex]));
  end;
  AClosing := AClosing or ClosureFound;
  if EndIndex = 0 then
    ATagName := Copy(ATagName, StartIndex, Length(ATagName) - StartIndex)
  else
    ATagName := Copy(ATagName, StartIndex, EndIndex - StartIndex + 1);
end {ExtractTagName};

{ ------------------------------------------------------------------------------------------------ }
procedure FindMatchingTag(ASelect: boolean = False; AContentsOnly: Boolean = False);
var
  npp: TApplication;
  doc: TActiveDocument;

  Tags: TStringList;
  Tag, NextTag, MatchingTag, Target: TTextRange;
  TagName: string;
  TagOpens, TagCloses: boolean;

  Direction: TDirectionEnum;
  IsXML: boolean;
  DisposeOfTag: boolean;
  i: integer;
  Found: TTextRange;

  // ---------------------------------------------------------------------------------------------
  procedure TagEncountered(ProcessDirection: TDirectionEnum; Prefix: Char);
  begin
    TagName := Prefix + TagName;

    if Tags.Count = 0 then begin
      Tags.AddObject(TagName, Tag);
      DisposeOfTag := False;
      Direction := ProcessDirection;
    end else if (IsXML and SameStr(Copy(TagName, 2), Copy(Tags.Strings[0], 2)))
                or ((not IsXML) and SameText(Copy(TagName, 2), Copy(Tags.Strings[0], 2))) then begin
      if Direction = ProcessDirection then begin
        Tags.AddObject(TagName, Tag);
        DisposeOfTag := False;
      end else begin
        if Tags.Count > 1 then begin
          Tags.Objects[Tags.Count - 1].Free;
          Tags.Delete(Tags.Count - 1);
        end else begin
          MatchingTag := Tag;
          Tags.AddObject(TagName, Tag);
          DisposeOfTag := False;
        end;
      end;
    end;
  end;
  // ---------------------------------------------------------------------------------------------
var
  InitPos: Sci_Position;
begin
  npp := GetApplication();
  doc := npp.ActiveDocument;

  IsXML := (doc.Language = L_XML);

  Tags := TStringList.Create;
  MatchingTag := nil;
  NextTag := nil;
  Direction := dirUnknown;
  try
    try
      repeat
        DisposeOfTag := True;
        if not Assigned(NextTag) then begin
          // The first time, begin at the document's current position
          Tag := ExtractTagName(doc, TagName, TagOpens, TagCloses);
        end else begin
          Tag := ExtractTagName(doc, TagName, TagOpens, TagCloses, NextTag.StartPos + 1);
          FreeAndNil(NextTag);
        end;
        if Assigned(Tag) then begin

          // If we're in HTML mode, check for any of the HTML 4 empty tags -- they're really self-closing
          if (not IsXML) and TagOpens and (not TagCloses) then begin
            for i := Low(scSelfClosingTags) to High(scSelfClosingTags) do begin
              if SameText(TagName, scSelfClosingTags[i]) then begin
                TagCloses := True;
                Break;
              end;
            end;
          end;

//          DebugWrite('FindMatchingTag', Format('Found TTextRange(%d, %d, "%s"): opens=%d, closes=%d', [Tag.StartPos, Tag.EndPos, Tag.Text, integer(TagOpens), integer(TagCloses)]));

          if TagOpens and TagCloses then begin // A self-closing tag
            TagName := '*' + TagName;

            if Tags.Count = 0 then begin
              MatchingTag := Tag;
              Tags.AddObject(TagName, Tag);
              DisposeOfTag := False;
              Direction := dirNone;
            end;

          end else if TagOpens then begin // An opening tag
            TagEncountered(dirForward, '+');

          end else if TagCloses then begin // A closing tag
            TagEncountered(dirBackward, '-');

          end else begin // A tag that doesn't open and doesn't close?!? This should never happen
            TagName := TagName + Format('[opening=%d,closing=%d]', [integer(TagOpens), integer(TagCloses)]);
//            DebugWrite('FindMatchingTag', Format('%s (%d-%d): "%s"', [TagName, Tag.StartPos, Tag.EndPos, Tag.Text]));
            Assert(False, 'This tag doesn''t open, and doesn''t close either!?! ' + TagName);
            MessageBeep(MB_ICONERROR);

          end{if TagOpens and/or TagCloses};

//          DebugWrite('FindMatchingTag', Format('Processed TTextRange(%d, %d, "%s")', [Tag.StartPos, Tag.EndPos, Tag.Text]));

        end{if Assigned(Tag)};


        // Find the next tag in the search direction
        case Direction of
          dirForward: begin
            // look forward for corresponding closing tag
            NextTag := doc.Find('<[^%\?]', SCFIND_REGEXP or SCFIND_POSIX, Tag.EndPos);
            if Assigned(NextTag) then
              NextTag.EndPos := NextTag.EndPos - 1;
          end;
          dirBackward: begin
            // look backward for corresponding opening tag
            {--- 2015-01-19 MCO: backwards find using regular expressions has become too slow.
                                  See ticket http://fossil.2of4.net/npp_htmltag/tktview/3ded3902a9 ---}
            InitPos := Tag.StartPos;
            repeat
              if Assigned(NextTag) then
                NextTag.Free;
              NextTag := doc.Find('>', 0, InitPos, 0);
              if Assigned(NextTag) then begin
                if NextTag.StartPos = 0 then begin
                  FreeAndNil(NextTag);
                  Break;
                end;
                NextTag.StartPos := NextTag.StartPos - 1;
                if CharInSet(NextTag.Text[1], ['%', '?']) then begin
                  InitPos := NextTag.StartPos;
                  Continue;
                end else begin
//                  OutputDebugString(PChar(Format('npp_htmltag:Found(%d-%d, "%s")', [NextTag.StartPos, NextTag.EndPos, NextTag.Text])));
                  NextTag.StartPos := NextTag.StartPos + 1;
                  Break;
                end;
              end;
            until not Assigned(NextTag);

          end;
          else begin
            //dirUnknown: ;
            //dirNone: ;
            NextTag := nil;
          end;
        end;

        if DisposeOfTag then begin
          FreeAndNil(Tag);
        end;
      until (NextTag = nil) or (MatchingTag <> nil);

      Tags.LineBreak := #9;
//      DebugWrite('FindMatchingTag:Done looking', Format('Tags.Count = %d (%s)', [Tags.Count, Tags.Text]));
      if Assigned(MatchingTag) then begin
//        DebugWrite('FindMatchingTag:Marking', Format('MatchingTag = TTextRange(%d, %d, "%s")', [MatchingTag.StartPos, MatchingTag.EndPos, MatchingTag.Text]));
        if Tags.Count = 2 then begin
          Tag := TTextRange(Tags.Objects[0]);
          if ASelect then begin
            if Tag.StartPos < MatchingTag.StartPos then begin
              if AContentsOnly then begin
                Target := doc.GetRange(Tag.EndPos, MatchingTag.StartPos);
              end else begin
                Target := doc.GetRange(Tag.StartPos, MatchingTag.EndPos);
              end;
            end else begin
              if AContentsOnly then begin
                Target := doc.GetRange(MatchingTag.EndPos, Tag.StartPos);
              end else begin
                Target := doc.GetRange(MatchingTag.StartPos, Tag.EndPos);
              end;
            end;
            try
              if AContentsOnly and True then begin // TODO: make optional, read setting from .ini ([MatchTag] SkipWhitespace=1)
                // Leave out whitespace at begin
                Found := doc.Find('[^ \r\n\t]', SCFIND_REGEXP or SCFIND_POSIX, Target.StartPos, Target.EndPos);
                if Assigned(Found) then begin
                  try
                    Target.StartPos := Found.StartPos;
                  finally
                    Found.Free;
                  end;
                end;
                // Also leave out whitespace at end
                Found := doc.Find('[^ \r\n\t]', SCFIND_REGEXP or SCFIND_POSIX, Target.EndPos, Target.StartPos);
                if Assigned(Found) then begin
                  try
                    Target.EndPos := Found.EndPos;
                  finally
                    Found.Free;
                  end;
                end;
              end;
              Target.Select;
            finally
              Target.Free;
            end;
          end else begin
//            DebugWrite('FindMatchingTag:Marking', Format('CurrentTag = TTextRange(%d, %d, "%s")', [Tag.StartPos, Tag.EndPos, Tag.Text]));
            MatchingTag.Select;
            {$IFNDEF NPPUNICODE} // NPP Unicode has always done this itself
            if HIWORD(npp.SendMessage(NPPM_GETNPPVERSION)) < 5 then begin
              Tag.Mark(STYLE_BRACELIGHT, 255, ncHighlightTimeout);
              MatchingTag.Mark(STYLE_BRACELIGHT, 255, ncHighlightTimeout);
            end;
            {$ENDIF}
          end;
        end else begin
          if ASelect then begin
            MatchingTag.Select;
          end else begin
            MatchingTag.Select;
            {$IFNDEF NPPUNICODE} // NPP Unicode has always done this itself
            if HIWORD(npp.SendMessage(NPPM_GETNPPVERSION)) < 5 then
              MatchingTag.Mark(STYLE_BRACELIGHT, 255, ncHighlightTimeout);
            {$ENDIF}
          end;
        end;
      end else if Tags.Count > 0 then begin
        MessageBeep(MB_ICONWARNING);
        Tag := TTextRange(Tags.Objects[0]);
        if ASelect then begin
          Tag.Select;
        end;
        Tag.Mark(STYLE_BRACEBAD, 255, ncHighlightTimeout);
      end else begin
        MessageBeep(MB_ICONWARNING);
      end;

    except
      on E: Exception do begin
        MessageBeep(MB_ICONERROR);
//        DebugWrite('FindMatchingTag:Exception', Format('%s: "%s"', [E.ClassName, E.Message]));
      end;
    end;
  finally
    while Tags.Count > 0 do begin
      Tag := TTextRange(Tags.Objects[0]);
//      DebugWrite('FindMatchingTag:Cleanup', Format('Tags["%s"] = TTextRange(%d, %d, "%s")', [Tags.Strings[0], Tag.StartPos, Tag.EndPos, Tag.Text]));
      Tags.Objects[0].Free;
      Tags.Delete(0);
    end;
    FreeAndNil(Tags);
  end;

  //MessageBox(npp.WindowHandle, PChar('Current tag: ' + TagName), scPTitle, MB_ICONINFORMATION);
end;

end.
