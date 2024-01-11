unit U_HTMLTagFinder;

{
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) Martijn Coppoolse <https://sourceforge.net/u/vor0nwe>
  Revisions copyright (c) 2022 Robert Di Pardo <dipardo.r@gmail.com>
}

interface
  uses
    NppSimpleObjects;

  type TSelectionOptions = set of (soNone, soTags, soContents);
  procedure FindMatchingTag(SelectionOptions: TSelectionOptions = [soNone]);

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation

uses
  SysUtils, Windows, Classes,
  nppplugin;

type
  TDirectionEnum = (dirBackward = -1, dirNone = 0, dirForward = 1, dirUnknown = 2);
  TTextRange = NppSimpleObjects.TTextRange;

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
begin
  ATagName := '';
  AOpening := True;
  AClosing := False;
  ClosureFound := False;
  Result := TTextRange.Create(AView);

  if (APosition < 0) then begin
    if (AView.CurrentPosition <= AView.Selection.Anchor) then begin
      APosition := AView.CurrentPosition + 1;
    end else begin
      APosition := AView.CurrentPosition;
    end;
  end;
  AView.Find('<', Result, 0, APosition, 0);
  if Result.Length = 0 then begin
    AView.Find('<', Result, 0, APosition);
    if Result.Length = 0 then begin
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

  TagEnd := TTextRange.Create(AView);
  AView.Find('>', TagEnd, 0, Result.EndPos + 1);
  if TagEnd.Length = 0 then begin
    ATagName := '';
    Exit;
  end else begin
    Result.EndPos := TagEnd.EndPos;
    FreeAndNil(TagEnd);
  end;

  // Determine the tag name, and whether it's an opening and/or closing tag
  StartIndex := 0;
  EndIndex := 0;
  ATagName := UTF8Encode(Result.Text);

  for i := 2 to Length(ATagName) - 1 do begin
    // Exit early when it's obviously a self-closing tag
    if (CompareStr(Copy(ATagName, i), '/>') = 0) then begin
      AOpening := True;
      AClosing := True;
      EndIndex := i - 1;
      break;
    end else if StartIndex = 0 then begin
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
      if not CharInSet(ATagName[i], ['0'..'9', 'A'..'Z', 'a'..'z', '-', '_', '.', ':', #0]) then begin
        EndIndex := i - 1;
        if AClosing = True then begin
          break;
        end;
      end;
    end else begin
      if ATagName[i] = '/' then begin
        ClosureFound := True;
      end else if ClosureFound and not CharInSet(ATagName[i], [' ', #9, #13, #10]) then begin
        ClosureFound := False;
      end;
    end;
  end;
  AClosing := AClosing or ClosureFound;
  if EndIndex = 0 then
    ATagName := Copy(ATagName, StartIndex, Length(ATagName) - StartIndex)
  else
    ATagName := Copy(ATagName, StartIndex, EndIndex - StartIndex + 1);
end {ExtractTagName};

{ ------------------------------------------------------------------------------------------------ }
procedure FindMatchingTag(SelectionOptions: TSelectionOptions);
var
  npp: TApplication;
  doc: TActiveDocument;

  Tags: TStringList;
  Tag, NextTag, MatchingTag, Target: TTextRange;
  TagName: string;
  TagOpens, TagCloses: boolean;
  InitPos: Sci_Position;
  Direction: TDirectionEnum;
  IsXML, ASelect, AContentsOnly, TagsOnly: boolean;
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
  procedure SelectTags(Tag, MatchingTag: TTextRange);
  var
    TagAttrPos: Integer;
  begin
    // Trim attributes from tag selection
    TagAttrPos := Pos(' ', Tag.Text);
    if TagAttrPos > Pos('<', Tag.Text) then
      Tag.EndPos := Tag.StartPos + TagAttrPos;
    // Trim '<' or '</' and '>' from selection
    if not Assigned(MatchingTag) then begin
      // Narrow selection for a self-closing tag
      Tag.StartPos := Tag.StartPos + Pos('<', Tag.Text);
      if Pos('/>', Tag.Text) > 0 then
        Tag.EndPos := Tag.EndPos - 1;
    end else
      Tag.StartPos := Tag.StartPos + (Pos('/', Tag.Text) shr 1) + 1;
    Doc.SendMessage(SCI_SETSELECTION, Tag.StartPos, Tag.EndPos - 1);
    if Assigned(MatchingTag) then begin
      TagAttrPos := Pos(' ', MatchingTag.Text);
      if TagAttrPos > Pos('<', MatchingTag.Text) then
        MatchingTag.EndPos := MatchingTag.StartPos + TagAttrPos;
      Doc.SendMessage(SCI_ADDSELECTION, MatchingTag.StartPos + (Pos('/', MatchingTag.Text) shr 1) + 1, MatchingTag.EndPos - 1);
    end;
  end;
  // ---------------------------------------------------------------------------------------------
begin
  npp := GetApplication();
  doc := npp.ActiveDocument;

  IsXML := (doc.Language = L_XML);
  ASelect := not (soNone in SelectionOptions);
  AContentsOnly := ASelect and ([soContents] = SelectionOptions);
  TagsOnly := ASelect and (not (soContents in SelectionOptions));
  Tags := TStringList.Create;
  MatchingTag := nil;
  NextTag := nil;
  Found := TTextRange.Create(doc);
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
        if Assigned(Tag) and (TagName <> '' ) then begin

          // If we're in HTML mode, check for any of the HTML 4 empty tags -- they're really self-closing
          if (not IsXML) and TagOpens and (not TagCloses) then begin
            for i := Low(scSelfClosingTags) to High(scSelfClosingTags) do begin
              if SameText(TagName, scSelfClosingTags[i]) then begin
                TagCloses := True;
                Break;
              end;
            end;
          end;

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
            Assert(False, 'This tag doesn''t open, and doesn''t close either!?! ' + TagName);
            MessageBeep(MB_ICONERROR);

          end{if TagOpens and/or TagCloses};

        end{if Assigned(Tag)};


        // Find the next tag in the search direction
        case Direction of
          dirForward: begin
            // look forward for corresponding closing tag
            NextTag := TTextRange.Create(doc);
            doc.Find('<[^%\?]', NextTag, SCFIND_REGEXP or SCFIND_POSIX, Tag.EndPos);
            if NextTag.Length <> 0 then
              NextTag.EndPos := NextTag.EndPos - 1
            else FreeAndNil(NextTag);
          end;
          dirBackward: begin
            // look backward for corresponding opening tag
            {--- 2015-01-19 MCO: backwards find using regular expressions has become too slow.
                                  See ticket http://fossil.2of4.net/npp_htmltag/tktview/3ded3902a9 ---}
            InitPos := Tag.StartPos;
            repeat
              if Assigned(NextTag) then
                NextTag.Free;
              NextTag := TTextRange.Create(doc);
              doc.Find('>', NextTag, 0, InitPos, 0);
              if NextTag.Length <> 0 then begin
                if NextTag.StartPos = 0 then begin
                  FreeAndNil(NextTag);
                  Break;
                end;
                NextTag.StartPos := NextTag.StartPos - 1;
                if CharInSet(NextTag.Text[1], ['%', '?']) then begin
                  InitPos := NextTag.StartPos;
                  Continue;
                end else begin
                  NextTag.StartPos := NextTag.StartPos + 1;
                  Break;
                end;
              end else FreeAndNil(NextTag);
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
      if Assigned(MatchingTag) then begin
        if Tags.Count = 2 then begin
          // Matching tag may be hidden by a fold
          doc.SendMessage(SCI_FOLDLINE, doc.SendMessage(SCI_LINEFROMPOSITION, MatchingTag.StartPos), SC_FOLDACTION_EXPAND);
          Tag := TTextRange(Tags.Objects[0]);
          if ASelect and not TagsOnly then begin
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
                doc.Find('[^ \r\n\t]', Found, SCFIND_REGEXP or SCFIND_POSIX, Target.StartPos, Target.EndPos);
                if Found.Length <> 0 then begin
                  try
                    Target.StartPos := Found.StartPos;
                  finally
                  end;
                end;
                // Also leave out whitespace at end
                doc.Find('[^ \r\n\t]', Found, SCFIND_REGEXP or SCFIND_POSIX, Target.EndPos, Target.StartPos);
                if Found.Length <> 0 then begin
                  try
                    Target.EndPos := Found.EndPos;
                  finally
                  end;
                end;
              end;
              Target.Select;
            finally
              Target.Free;
            end;
          end else if ASelect then begin
            SelectTags(Tag, MatchingTag);
          end else begin
            MatchingTag.Select;
          end;
        end else begin  // Self-closing tag
          if TagsOnly then begin
            SelectTags(MatchingTag, nil);
          end else begin
            MatchingTag.Select;
          end;
        end;
      end else if Tags.Count > 0 then begin
        MessageBeep(MB_ICONWARNING);
        Tag := TTextRange(Tags.Objects[0]);
        if ASelect then begin
          Tag.Select;
        end;
        Tag.Mark(STYLE_BRACEBAD, ncHighlightTimeout);
      end else begin
        MessageBeep(MB_ICONWARNING);
      end;

    except
      on E: Exception do begin
        MessageBox(0, PChar(E.Message), PChar(@E.ClassName()[1]), MB_ICONERROR);
      end;
    end;
  finally
    while Tags.Count > 0 do begin
      Tag := TTextRange(Tags.Objects[0]);
      Tags.Objects[0].Free;
      Tags.Delete(0);
    end;
    FreeAndNil(Tags);
    FreeAndNil(Found);
    FreeAndNil(NextTag);
  end;

end;

end.
