unit NppSimpleObjects;

{
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) Martijn Coppoolse <https://sourceforge.net/u/vor0nwe>
  Revisions copyright (c) 2022 Robert Di Pardo <dipardo.r@gmail.com>
}

interface
  uses
    Classes, Windows, NppPlugin;

{$I '..\Include\SciApi.inc'}

  const
    MULTISELECTION_MASK: 0..4 = $4;

  type
  //////////////////////////////////////////////////////////////////////////////////////////////////
    TActiveDocument = class;
    { -------------------------------------------------------------------------------------------- }
    TTextRange = class
      protected
        FEditor:    TActiveDocument;
        FStartPos:  Sci_Position;
        FEndPos:    Sci_Position;

        function  GetAnchor(): Sci_Position;
        procedure SetAnchor(const AValue: Sci_Position);
        function  GetStart(): Sci_Position; virtual;
        procedure SetStart(const AValue: Sci_Position); virtual;
        function  GetEnd(): Sci_Position; virtual;
        procedure SetEnd(const AValue: Sci_Position); virtual;
        function  GetLength(): Sci_Position; virtual;
        procedure SetLength(const AValue: Sci_Position); virtual;
        function  GetText(): WideString; virtual;
        procedure SetText(const AValue: WideString); virtual;
        function  GetFirstLine(): Sci_Position;
        function  GetStartCol(): Sci_Position;
        function  GetLastLine(): Sci_Position;
        function  GetEndCol(): Sci_Position;
        function  GetLineCount: Sci_Position;
        function  GetIndentLevel(): integer;
        procedure SetIndentLevel(const AValue: integer);
      public
        constructor Create(const AEditor: TActiveDocument; const AStartPos: Sci_Position = 0; AEndPos: Sci_Position = 0); overload;
        destructor  Destroy; override;

        procedure Select();
        procedure ClearSelection;
        procedure Indent(const Levels: integer = 1);
        procedure Mark(const Style: integer; const DurationInMs: cardinal = 0);

        property Document: TActiveDocument  read FEditor;
        property Anchor: Sci_Position       read GetAnchor        write SetAnchor;
        property StartPos: Sci_Position     read FStartPos        write SetStart;
        property EndPos: Sci_Position       read FEndPos          write SetEnd;
        property Length: Sci_Position       read GetLength        write SetLength;
        property Text: WideString           read GetText          write SetText;
        property FirstLine: Sci_Position    read GetFirstLine;
        property FirstColumn: Sci_Position  read GetStartCol;
        property LastLine: Sci_Position     read GetLastLine;
        property LastColumn: Sci_Position   read GetEndCol;
        property LineCount: Sci_Position    read GetLineCount;
        property IndentationLevel: integer  read GetIndentLevel   write SetIndentLevel;
    end;
    { -------------------------------------------------------------------------------------------- }
    TSelection = class(TTextRange)
      protected
        function  GetCurrentPos(): Sci_Position;
        procedure SetCurrentPos(const AValue: Sci_Position);
        function  GetStart(): Sci_Position; override;
        procedure SetStart(const AValue: Sci_Position); override;
        function  GetEnd(): Sci_Position; override;
        procedure SetEnd(const AValue: Sci_Position); override;
        function  GetLength(): Sci_Position; override;
        procedure SetLength(const AValue: Sci_Position); override;
        function  GetText(): WideString; override;
        procedure SetText(const AValue: WideString); override;
      public
        constructor Create(const AEditor: TActiveDocument);

        property Position: Sci_Position read GetCurrentPos    write SetCurrentPos;
        property StartPos: Sci_Position read GetStart         write SetStart;
        property EndPos: Sci_Position   read GetEnd           write SetEnd;
        property Length: Sci_Position   read GetLength        write SetLength;
        property Text: WideString       read GetText;
    end;
    { -------------------------------------------------------------------------------------------- }
    TTextRangeMark = class
      private
        FWindowHandle: THandle;
        FStartPos: Sci_Position;
        FEndPos: Sci_Position;
        FTimerID: integer;
      public
        constructor Create(const ARange: TTextRange; const ADurationInMS: cardinal); overload;
        destructor  Destroy; override;
    end;

    { -------------------------------------------------------------------------------------------- }
    TWindowedObject = class
      protected
        FWindowHandle: THandle;
        FIsNpp: boolean;
        FSciApiLevel: TSciApiLevel;
        procedure SetApiLevel(Api: TSciApiLevel); virtual;
      public
        constructor Create(AWindowHandle: THandle; ANppWindow: boolean = False);

        function SendMessage(const Message: UINT; wParam: WPARAM = 0; lParam: NativeUInt = 0): LRESULT; overload; virtual;
        function SendMessage(const Message: UINT; wParam: WPARAM; lParam: Pointer): LRESULT; overload; virtual;
        procedure PostMessage(const Message: UINT; wParam: WPARAM = 0; lParam: NativeUInt = 0); overload; virtual;
        procedure PostMessage(const Message: UINT; wParam: WPARAM; lParam: Pointer); overload; virtual;
        property ApiLevel: TSciApiLevel read FSciApiLevel write SetApiLevel;
    end;

    { -------------------------------------------------------------------------------------------- }
    TSelectionMode = (smStreamSingle = SC_SEL_STREAM, smColumn, smLines, smThin, smStreamMulti);
    TActiveDocument = class(TWindowedObject)
      private
        FSelection: TSelection;

        function  GetEditor(): TActiveDocument;
        function  GetModified(): boolean;
        procedure SetModified(const AValue: boolean);
        function  GetReadOnly(): boolean;
        procedure SetReadOnly(const AValue: boolean);
        function  GetText(): WideString;
        procedure SetText(const AValue: WideString);
        function  GetLength(): Sci_Position;
        function  GetLineCount(): Sci_Position;
        function  GetNextLineStart(): Sci_Position;
        function  GetLangType(): LangType;
        procedure SetLangType(const AValue: LangType);
        function  GetCurrentPos(): Sci_Position;
        procedure SetCurrentPos(const AValue: Sci_Position);
        function  GetSelection: TSelection;
        function  GetFirstVisibleLine: Sci_Position;
        function  GetLinesOnScreen: Sci_Position;
      public
        destructor Destroy(); override;
        function  Activate(): TActiveDocument;
        procedure Insert(const Text: WideString; const Position: Sci_Position = Sci_Position(High(Cardinal)));
        function  GetRange(const StartPosition: Sci_Position = 0; const LastPosition: Sci_Position = Sci_Position(High(Cardinal))): TTextRange;
        function  GetLines(const FirstLine: Sci_Position; const Count: Sci_Position = 1): TTextRange;
        function  CharWidth: Byte;
        function  SelectionMode: TSelectionMode;
        procedure Select(const Start: Sci_Position = 0; const Length: Sci_Position = Sci_Position(High(Cardinal)));
        procedure SelectMultiple(const Start: Sci_Position; const ALength: Sci_Position; MatchAll: Boolean = True);
        procedure ReplaceSelection(const AValue: WideString);
        procedure Find(const AText: WideString; var ATarget: TTextRange; const AOptions: integer = 0;
                        const AStartPos: Sci_Position = -1; const AEndPos: Sci_Position = -1); overload;
        procedure Find(const AText: WideString; var ATarget: TTextRange; const AOptions: integer); overload;

        property IsDirty: boolean     read GetModified  write SetModified;
        property IsReadOnly: boolean  read GetReadOnly  write SetReadOnly;
        property Text: WideString     read GetText      write SetText;
        property Length: Sci_Position read GetLength;
        property LineCount: Sci_Position read GetLineCount;
        property NextLineStartPosition: Sci_Position read GetNextLineStart;
        property Language: LangType   read GetLangType  write SetLangType;
        property CurrentPosition:Sci_Position read GetCurrentPos  write SetCurrentPos;
        property Selection: TSelection        read GetSelection;
        property TopLine: Sci_Position        read GetFirstVisibleLine;
        property VisibleLineCount: Sci_Position read GetLinesOnScreen;
    end;

    { -------------------------------------------------------------------------------------------- }
    TEditors = class
      private
        FList: TList;
      protected
        function  GetCount(): cardinal;
        function  GetItem(const AIndex: cardinal): TActiveDocument;
      public
        constructor Create(const ANPPData: PNppData);
        destructor  Destroy(); override;

        property Count: cardinal                                read GetCount;

        property Item[const Index: cardinal]: TActiveDocument   read GetItem; default;
    end;

    { -------------------------------------------------------------------------------------------- }
    TApplication = class(TWindowedObject)
      private
        FEditors: TEditors;

        function  GetDocument(): TActiveDocument;
      protected
        procedure SetApiLevel(Api: TSciApiLevel); override;
      public
        constructor Create(const ANppData: PNppData);
        destructor  Destroy(); override;


        property WindowHandle: THandle            read FWindowHandle;

        property Editors: TEditors                read FEditors;
        property ActiveDocument: TActiveDocument  read GetDocument;
    end;

    function GetApplication(const ANPPData: PNPPData = nil): TApplication; overload;
    function GetApplication(const ANPPData: PNPPData; Api: TSciApiLevel): TApplication; overload;

////////////////////////////////////////////////////////////////////////////////////////////////////
implementation

uses
  {$IFDEF CPUx64}Math,{$ENDIF}
  SysUtils;

var
  Application: TApplication;
  TimedTextRangeMarks: TList;
  TRTimerID: Cardinal;

{ ------------------------------------------------------------------------------------------------ }

function GetApplication(const ANPPData: PNPPData = nil): TApplication;
begin
  if not Assigned(Application) and Assigned(ANPPData) then
    Application := TApplication.Create(ANPPData);
  Result := Application;
end;

function GetApplication(const ANPPData: PNPPData; Api: TSciApiLevel): TApplication;
begin
  Application := GetApplication(ANPPData);
  Application.ApiLevel := Api;
  Result := Application;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TextRangeUnmarkTimer(AWindowHandle: THandle; AMessage: Cardinal; AEventID: Cardinal; ATime: Cardinal); stdcall;
var
  i: integer;
  trm: TTextRangeMark;
begin
  KillTimer(0, AEventID);
  if Assigned(TimedTextRangeMarks) then begin
    for i := 0 to TimedTextRangeMarks.Count - 1 do begin
      trm := TTextRangeMark(TimedTextRangeMarks.Items[i]);
      if trm.FTimerID = integer(AEventID) then begin
        TimedTextRangeMarks.Delete(i);
        trm.Free;
      end;
    end;
  end;
end;

{ ================================================================================================ }
{ TTextRange }

constructor TTextRange.Create(const AEditor: TActiveDocument; const AStartPos: Sci_Position = 0; AEndPos: Sci_Position = 0);
begin
  FEditor := AEditor;
  SetStart(AStartPos);
  SetEnd(AEndPos);
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TTextRange.Destroy;
begin

  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }
function TTextRange.GetAnchor: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_GETANCHOR);
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetAnchor(const AValue: Sci_Position);
begin
  FEditor.SendMessage(SCI_SETANCHOR, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetStart: Sci_Position;
begin
  Result := FStartPos;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetStart(const AValue: Sci_Position);
begin
  if AValue <= INVALID_POSITION then
    FStartPos := 0
  else
    FStartPos := AValue;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetEnd: Sci_Position;
begin
  if FEndPos <= INVALID_POSITION then
    Result := FEditor.SendMessage(SCI_GETLENGTH)
  else
    Result := FEndPos;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetEnd(const AValue: Sci_Position);
begin
  if AValue <= INVALID_POSITION then
    FEndPos := FEditor.SendMessage(SCI_GETLENGTH)
  else
    FEndPos := AValue;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetLength: Sci_Position;
begin
  Result := Abs(FEndPos - FStartPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetLineCount: Sci_Position;
begin
  Result := GetLastLine - GetFirstLine + 1;
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetLength(const AValue: Sci_Position);
begin
  Self.EndPos := FStartPos + AValue;
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetText: WideString;
var
  SciMsg: UINT;
  tr: RSciTextRange;
  Chars: AnsiString;
begin
  SciMsg := SCI_GETTEXTRANGEFULL;
  if FEditor.ApiLevel < sciApi_GTE_523 then
      SciMsg := SCI_GETTEXTRANGE;
  Chars := AnsiString(StringOfChar(#0, GetLength + 1));
  tr.chrg.cpMin := FStartPos;
  tr.chrg.cpMax := FEndPos;
  tr.lpstrText := PAnsiChar(Chars);
  System.SetLength(Chars, FEditor.SendMessage(SciMsg, 0, @tr));
  Result := WideString(Chars);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetText(const AValue: WideString);
var
  SciMsg: UINT;
  Chars: AnsiString;
  TxtRng: Integer;
begin
  SciMsg := SCI_REPLACETARGETMINIMAL;
  if FEditor.ApiLevel < sciApi_GTE_532 then
    SciMsg := SCI_REPLACETARGET;
  case FEditor.SendMessage(SCI_GETCODEPAGE) of
  SC_CP_UTF8:
    Chars := UTF8Encode(AValue)
  else
    Chars := RawByteString(AValue);
  end;
  TxtRng := System.Length(Chars);
  FEditor.SendMessage(SCI_SETTARGETSTART, FStartPos);
  FEditor.SendMessage(SCI_SETTARGETEND, FEndPos);
  Inc(FStartPos, Sci_Position(FEditor.SendMessage(SciMsg, TxtRng, PAnsiChar(Chars))));
end;


{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetStartCol: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_GETCOLUMN, FStartPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetFirstLine: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_LINEFROMPOSITION, FStartPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetIndentLevel: integer;
begin
  Result := FEditor.SendMessage(SCI_GETLINEINDENTATION, GetFirstLine);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.SetIndentLevel(const AValue: integer);
begin
  FEditor.SendMessage(SCI_SETLINEINDENTATION, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetEndCol: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_GETCOLUMN, FEndPos);
end;

{ ------------------------------------------------------------------------------------------------ }

function TTextRange.GetLastLine: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_LINEFROMPOSITION, FEndPos);
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TTextRange.Indent(const Levels: integer);
var
  i: Sci_Position;
begin
  for i := GetFirstLine to GetLastLine do begin
    FEditor.SendMessage(SCI_SETLINEINDENTATION, i, FEditor.SendMessage(SCI_GETLINEINDENTATION, i) + LRESULT(Levels));
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TTextRange.Mark(const Style: integer; const DurationInMs: cardinal);
var
  CurrentStyleEnd: Sci_Position;
begin
  CurrentStyleEnd := FEditor.SendMessage(SCI_GETENDSTYLED);
  FEditor.SendMessage(SCI_STARTSTYLING, FStartPos, 0);
  FEditor.SendMessage(SCI_SETSTYLING, GetLength, Style);
  FEditor.SendMessage(SCI_STARTSTYLING, CurrentStyleEnd, 0);

  if DurationInMs > 0 then
    TimedTextRangeMarks.Add(TTextRangeMark.Create(Self, DurationInMs));
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.Select;
begin
  FEditor.SendMessage(SCI_SETSELECTION, FEndPos, FStartPos);
  FEditor.SendMessage(SCI_SCROLLCARET);
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TTextRange.ClearSelection();
begin
  Self.Anchor := FEditor.CurrentPosition;
end;

{ ================================================================================================ }
{ TSelection }

constructor TSelection.Create(const AEditor: TActiveDocument);
begin
  FEditor := AEditor;
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetCurrentPos: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_GETCURRENTPOS);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetCurrentPos(const AValue: Sci_Position);
begin
  FEditor.SendMessage(SCI_SETCURRENTPOS, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetEnd: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_GETSELECTIONEND);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetEnd(const AValue: Sci_Position);
begin
  FEditor.SendMessage(SCI_SETSELECTIONEND, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetLength: Sci_Position;
begin
  Result := Abs(GetEnd - GetStart);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetLength(const AValue: Sci_Position);
begin
  FEditor.SendMessage(SCI_SETSELECTIONEND, GetStart + AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetStart: Sci_Position;
begin
  Result := FEditor.SendMessage(SCI_GETSELECTIONSTART);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetStart(const AValue: Sci_Position);
begin
  FEditor.SendMessage(SCI_SETSELECTIONSTART, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TSelection.GetText: WideString;
var
  Chars: AnsiString;
  LenSel: Sci_Position;
begin
  LenSel := FEditor.SendMessage(SCI_GETSELTEXT, 0, Nil);
  if FEditor.ApiLevel >= sciApi_GTE_515 then Inc(LenSel);
  Chars := StringOfChar(#0, LenSel);
  FEditor.SendMessage(SCI_GETSELTEXT, 0, PAnsiChar(Chars));
  case FEditor.SendMessage(SCI_GETCODEPAGE) of
    SC_CP_UTF8:
      Result := UTF8Decode(Chars)
    else
      Result := WideString(UTF8Encode(Chars))
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TSelection.SetText(const AValue: WideString);
var
  Chars: AnsiString;
  NewLength, EndPos: Sci_Position;
  Reversed: boolean;
begin
  case FEditor.SendMessage(SCI_GETCODEPAGE) of
  SC_CP_UTF8:
    Chars := UTF8Encode(AValue)
  else
    Chars := RawByteString(AValue);
  end;
  if (FEditor.SendMessage(SCI_GETSELECTIONS) > 1) then begin
    FEditor.SendMessage(SCI_SETSEL, Self.Anchor, Self.GetCurrentPos);
    Chars := TrimRight(Chars);
  end;
  NewLength := System.Length(Chars) - 1;
  Reversed := (Self.Anchor > Self.GetCurrentPos);
  FEditor.SendMessage(SCI_REPLACESEL, 0, PAnsiChar(Chars));
  EndPos := GetCurrentPos;
  if Reversed then begin
    FEditor.SendMessage(SCI_SETSEL, EndPos, EndPos - NewLength);
  end else begin
    FEditor.SendMessage(SCI_SETSEL, EndPos - NewLength, EndPos);
  end;
end;

{ ================================================================================================ }
{ TWindowedObject }

constructor TWindowedObject.Create(AWindowHandle: THandle; ANppWindow: boolean);
begin
  FWindowHandle := AWindowHandle;
  FIsNpp := ANppWindow;
  FSciApiLevel := Default(TSciApiLevel);
end;

{ ------------------------------------------------------------------------------------------------ }
function TWindowedObject.SendMessage(const Message: UINT; wParam: WPARAM; lParam: NativeUInt): LRESULT;
begin
  try
    if SendMessageTimeout(FWindowHandle, Message, wParam, lParam, SMTO_NORMAL, 5000, @Result) = 0 then
      RaiseLastOSError;
  except
    on E: EOSError do begin
      raise;
    end;
    on E: Exception do begin
      raise;
    end;
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
function TWindowedObject.SendMessage(const Message: UINT; wParam: WPARAM; lParam: Pointer): LRESULT;
begin
  try
    if SendMessageTimeout(FWindowHandle, Message, wParam, Windows.LPARAM(lParam), SMTO_NORMAL, 5000, @Result) = 0 then
      RaiseLastOSError;
  except
    on E: EOSError do begin
      raise;
    end;
    on E: Exception do begin
      raise;
    end;
  end;
end;

{ ------------------------------------------------------------------------------------------------ }
procedure TWindowedObject.PostMessage(const Message: UINT; wParam: WPARAM; lParam: NativeUInt);
begin
  try
    if Windows.PostMessage(FWindowHandle, Message, wParam, Windows.LPARAM(lParam)) = False then
      RaiseLastOSError;
  except
    on E: EOSError do begin
      raise;
    end;
    on E: Exception do begin
      raise;
    end;
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TWindowedObject.PostMessage(const Message: UINT; wParam: WPARAM; lParam: Pointer);
begin
  try
    if Windows.PostMessage(FWindowHandle, Message, wParam, Windows.LPARAM(lParam)) = False then
      RaiseLastOSError;
  except
    on E: EOSError do begin
      raise;
    end;
    on E: Exception do begin
      raise;
    end;
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TWindowedObject.SetApiLevel(Api: TSciApiLevel);
begin
    FSciApiLevel := Api;
end;

{ ================================================================================================ }
{ TActiveDocument }

destructor TActiveDocument.Destroy;
begin
  if Assigned(FSelection) then
    FreeAndNil(FSelection);

  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.Find(const AText: WideString; var ATarget: TTextRange; const AOptions: integer);
begin
  if Assigned(ATarget) then
  begin
    if AOptions <> 0 then
      Find(AText, ATarget, AOptions, ATarget.StartPos, ATarget.EndPos)
    else
      Find(AText, ATarget);
  end;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.Find(const AText: WideString; var ATarget: TTextRange; const AOptions: integer; const AStartPos, AEndPos: Sci_Position);
var
  SciMsg: UINT;
  TTF: RSciTextToFind;
  StartPos: LRESULT;
begin
  SciMsg := SCI_FINDTEXTFULL;
  if Self.ApiLevel < sciApi_GTE_523 then
      SciMsg := SCI_FINDTEXT;
  TTF := Default(RSciTextToFind);
  if AStartPos < 0 then
    TTF.chrg.cpMin := 0
  else
    TTF.chrg.cpMin := AStartPos;
  if AEndPos < 0 then
    TTF.chrg.cpMax := High(TTF.chrg.cpMax)
  else
    TTF.chrg.cpMax := AEndPos;
  TTF.lpstrText := PAnsiChar(UTF8Encode(AText));
  TTF.chrgText := TTF.chrg;
  StartPos := SendMessage(SciMsg, AOptions, @TTF);
  if StartPos = -1 then begin
    ATarget.SetStart(0);
    ATarget.SetEnd(0);
  end else begin
    ATarget.SetStart(TTF.chrgText.cpMin);
    ATarget.SetEnd(TTF.chrgText.cpMax);
  end;
end;

{ ------------------------------------------------------------------------------------------------ }
function TActiveDocument.SelectionMode: TSelectionMode;
begin
  Result := TSelectionMode(Self.SendMessage(SCI_GETSELECTIONMODE));
  if (Ord(Result) = SC_SEL_STREAM) and (Self.SendMessage(SCI_GETSELECTIONS) > 1) then
    Result := smStreamMulti;
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetCurrentPos: Sci_Position;
begin
  Result := SendMessage(SCI_GETCURRENTPOS);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.SetCurrentPos(const AValue: Sci_Position);
begin
  SendMessage(SCI_SETANCHOR, AValue);
  SendMessage(SCI_SETCURRENTPOS, AValue);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetFirstVisibleLine: Sci_Position;
begin
  Result := SendMessage(SCI_GETFIRSTVISIBLELINE);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLinesOnScreen: Sci_Position;
begin
  Result := SendMessage(SCI_LINESONSCREEN);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.CharWidth: Byte;
begin
  Result := SizeOf(AnsiChar);
  if (SendMessage(SCI_GETCODEPAGE) = SC_CP_UTF8) then
    Result := Sizeof(Widechar);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLangType: LangType;
var
  LT: integer;
begin
  Application.SendMessage(NPPM_GETCURRENTLANGTYPE, 0, @LT);
  Result := LangType(LT);
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.SetLangType(const AValue: LangType);
begin
  SendMessage(NPPM_SETCURRENTLANGTYPE, 0, @integer(AValue));
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetSelection: TSelection;
begin
  if not Assigned(FSelection) then
    FSelection := TSelection.Create(Self);
  Result := FSelection;
end;
{ ------------------------------------------------------------------------------------------------ }
procedure TActiveDocument.Select(const Start, Length: Sci_Position);
var
  SelMode: TSelectionMode;
begin
  SelMode := TSelectionMode(Ord(SelectionMode) and (not MULTISELECTION_MASK));
  if SelMode <> smStreamSingle then
    SendMessage(SCI_SETSELECTIONMODE, SC_SEL_STREAM);
  SendMessage(SCI_SETSEL, Start, Start + Length);
  if SelMode <> smStreamSingle then
    SendMessage(SCI_SETSELECTIONMODE, Ord(SelMode))
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SelectMultiple(const Start: Sci_Position; const ALength: Sci_Position; MatchAll: Boolean);
var
  SciMsg : cardinal;
begin
  SendMessage(SCI_CANCEL);
  FSelection.StartPos := SendMessage(SCI_POSITIONAFTER, (Start - (CharWidth shr 1)));
  FSelection.EndPos := FSelection.StartPos + ALength;
  SciMsg := SCI_MULTIPLESELECTADDEACH;
  if not MatchAll then
    SciMsg := SCI_MULTIPLESELECTADDNEXT;
  SendMessage(SciMsg);
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.ReplaceSelection(const AValue: WideString);
var
  Chars: AnsiString;
  MultiPasteMode: Cardinal;
begin
  case Self.SendMessage(SCI_GETCODEPAGE) of
  SC_CP_UTF8:
    Chars := UTF8Encode(AValue)
  else
    Chars := RawByteString(AValue);
  end;
  if (SelectionMode = smStreamMulti) then begin
    MultiPasteMode := SendMessage(SCI_GETMULTIPASTE);
    SendMessage(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
    SendMessage(SCI_COPYTEXT, System.Length(Chars), PAnsiChar(Chars));
    SendMessage(SCI_PASTE);
    SendMessage(SCI_SETMULTIPASTE, MultiPasteMode);
  end else
    Selection.SetText(AValue);
end;

{ ================================================================================================ }
{ TEditors }

constructor TEditors.Create(const ANPPData: PNppData);
begin
  FList := TList.Create;
  FList.Add(TActiveDocument.Create(ANPPData.nppScintillaMainHandle));
  FList.Add(TActiveDocument.Create(ANPPData.nppScintillaSecondHandle));
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TEditors.Destroy;
begin
  TActiveDocument(FList.Items[1]).Free;
  FList.Delete(1);
  TActiveDocument(FList.Items[0]).Free;
  FList.Delete(0);
  FreeAndNil(FList);
  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }

function TEditors.GetCount: cardinal;
begin
  Result := 2;
end;

{ ------------------------------------------------------------------------------------------------ }

function TEditors.GetItem(const AIndex: cardinal): TActiveDocument;
begin
  Result := TActiveDocument(FList.Items[AIndex]);
end;

{ ================================================================================================ }
{ TActiveDocument }

function TActiveDocument.Activate: TActiveDocument;
begin
{$MESSAGE HINT 'Figure out where this document is, and switch to it'}
  Result := nil;
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetEditor: TActiveDocument;
begin
  Result := Self; // TODO
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLength: Sci_Position;
begin
  Result := SendMessage(SCI_GETLENGTH);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLineCount: Sci_Position;
begin
  Result := SendMessage(SCI_GETLINECOUNT);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetNextLineStart(): Sci_Position;
var
  LineEndCurrent: Sci_Position;
begin
  LineEndCurrent := SendMessage(SCI_GETLINEENDPOSITION, SendMessage(SCI_LINEFROMPOSITION, CurrentPosition));
  Result := SendMessage(SCI_POSITIONAFTER, LineEndCurrent);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetLines(const FirstLine: Sci_Position; const Count: Sci_Position): TTextRange;
var
  LineCount: Sci_Position;
begin
  LineCount := GetLineCount;
  if FirstLine + Count > LineCount then begin
    Result := TTextRange.Create(Self, SendMessage(SCI_POSITIONFROMLINE, FirstLine), GetLength);
  end else begin
    Result := TTextRange.Create(Self, SendMessage(SCI_POSITIONFROMLINE, FirstLine), SendMessage(SCI_GETLINEENDPOSITION, FirstLine + Count));
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetModified: boolean;
begin
  Result := Boolean(SendMessage(SCI_GETMODIFY));
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetRange(const StartPosition, LastPosition: Sci_Position): TTextRange;
begin
  Result := TTextRange.Create(Self, StartPosition, LastPosition);
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetReadOnly: boolean;
begin
  Result := boolean(SendMessage(SCI_GETREADONLY));
end;

{ ------------------------------------------------------------------------------------------------ }

function TActiveDocument.GetText: WideString;
{
  Per https://www.scintilla.org/ScintillaHistory.html § 5.1.5

  "When calling SCI_GETTEXT, SCI_GETSELTEXT, and SCI_GETCURLINE with a NULL
  buffer argument to discover the length that should be allocated, do not
  include the terminating NUL in the returned value. The value returned is 1
  less than previous versions of Scintilla. Applications should allocate a
  buffer 1 more than this to accommodate the NUL. The wParam (length)
  argument to SCI_GETTEXT and SCI_GETCURLINE also omits the NUL."
}
var
  Chars: AnsiString;
  Len: Sci_PositionU;
  {$IF DEFINED(FPC) AND DEFINED(CPUx64)}
  SafeLen, SafeDocLen: Extended;
  {$ENDIF}
begin
  Len := SendMessage(SCI_GETTEXT, WPARAM(High(Sci_PositionU)) - 1, nil);
  if Self.ApiLevel >= sciApi_GTE_515 then
  begin
{$IFNDEF FPC}
  Len := Round(MinValue([Len + 1, SendMessage(SCI_GETLENGTH)]));
{$ELSE}
{$IFDEF CPUx64}
  SafeLen := Len + 1.0;
  SafeDocLen := SendMessage(SCI_GETLENGTH) * 1.0;
  Len := Round(MinValue([SafeLen, SafeDocLen]));
{$ELSE}
  Inc(Len);
{$ENDIF}
{$ENDIF}
  end;
  Chars := EmptyStr;
  SetLength(Chars, Len);
  SendMessage(SCI_GETTEXT, Len, PAnsiChar(Chars));
  case SendMessage(SCI_GETCODEPAGE) of
    SC_CP_UTF8:
      Result := UTF8Decode(Chars)
    else
      Result := WideString(UTF8Encode(Chars))
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.Insert(const Text: WideString; const Position: Sci_Position);
begin
  SendMessage(SCI_INSERTTEXT, Position, PAnsiChar(UTF8Encode(Text)));
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SetModified(const AValue: boolean);
begin
  if AValue then begin
    SendMessage(SCI_SETSAVEPOINT);
  end else begin
    Application.SendMessage(NPPM_MAKECURRENTBUFFERDIRTY); // TODO: kunnen we niet op een andere manier bij de TApplication komen?
  end;
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SetReadOnly(const AValue: boolean);
begin
  SendMessage(SCI_SETREADONLY, cardinal(AValue));
end;

{ ------------------------------------------------------------------------------------------------ }

procedure TActiveDocument.SetText(const AValue: WideString);
var
  Chars: AnsiString;
begin
  case Self.SendMessage(SCI_GETCODEPAGE) of
  SC_CP_UTF8:
    Chars := UTF8Encode(AValue)
  else
    Chars := RawByteString(AValue);
  end;
  SendMessage(SCI_SETTEXT, 0, PAnsiChar(Chars));
end;

{ ================================================================================================ }
{ TApplication }

constructor TApplication.Create(const ANppData: PNppData);
begin
  inherited Create(ANppData.nppHandle, True);

  FEditors := TEditors.Create(ANppData);
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TApplication.Destroy;
begin
  if Assigned(FEditors) then
    FreeAndNil(FEditors);

  inherited;
end;

{ ------------------------------------------------------------------------------------------------ }

function TApplication.GetDocument: TActiveDocument;
var
  Index: Cardinal;
begin
  Self.SendMessage(NPPM_GETCURRENTSCINTILLA, 0, @Index);
  Result := FEditors.Item[Index];
end;


{ ------------------------------------------------------------------------------------------------ }

procedure TApplication.SetApiLevel(Api: TSciApiLevel);
var i: Cardinal;
begin
  for i := 0 to FEditors.Count - 1 do begin
    FEditors[i].FSciApiLevel := Api
  end;

  inherited SetApiLevel(Api);
end;

{ ------------------------------------------------------------------------------------------------ }
{ TTextRangeMark }

constructor TTextRangeMark.Create(const ARange: TTextRange; const ADurationInMS: cardinal);
begin
  FWindowHandle := ARange.FEditor.FWindowHandle;
  FStartPos := ARange.FStartPos;
  FEndPos := ARange.FEndPos;

  FTimerID := SetTimer(0, 0, ADurationInMs, @TextRangeUnmarkTimer);
end;
{ ------------------------------------------------------------------------------------------------ }
destructor TTextRangeMark.Destroy;
var
  Editor: TActiveDocument;
  CurrentStyleEnd: Sci_Position;
begin
  Editor := TActiveDocument.Create(FWindowHandle);
  try
    try
      KillTimer(0, FTimerID);
      CurrentStyleEnd := Editor.SendMessage(SCI_GETENDSTYLED);
      if FStartPos < CurrentStyleEnd then
        CurrentStyleEnd := FStartPos;
      Editor.SendMessage(SCI_STARTSTYLING, CurrentStyleEnd, 0);
    except
      on E: Exception do begin
        // ignore
      end;
    end;
  finally
    FreeAndNil(Editor);
  end;

  inherited;
end;

////////////////////////////////////////////////////////////////////////////////////////////////////
initialization
  TimedTextRangeMarks := TList.Create;

finalization
  if Assigned(TimedTextRangeMarks) then begin
    while TimedTextRangeMarks.Count > 0 do begin
      TRTimerID := TTextRangeMark(TimedTextRangeMarks.Items[0]).FTimerID;
      if TRTimerID <> 0 then
        KillTimer(0, TRTimerID);
      TTextRangeMark(TimedTextRangeMarks.Items[0]).Free;
      TimedTextRangeMarks.Delete(0);
    end;
    FreeAndNil(TimedTextRangeMarks);
  end;

  if Assigned(Application) then begin
    FreeAndNil(Application);
  end;

end.
