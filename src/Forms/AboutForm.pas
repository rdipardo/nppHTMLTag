unit AboutForm;

{$IFDEF FPC}{$mode delphi}{$ENDIF}

{
  Copyright (c) 2022 Robert Di Pardo <dipardo.r@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public License,
  v. 2.0. If a copy of the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
}

interface

uses
  Classes,
  fpg_base,
  fpg_main,
  fpg_form,
  fpg_panel,
  fpg_button,
  VersionInfo;

const
  Repo = 'https://bitbucket.org/rdipardo/htmltag/downloads';
  Author = #$00A9' 2007-2020 %s (v0.1 - v1.1)';
  Maintainer = #$00A9' 2022 Robert Di Pardo (since v1.2)';
  FpgAuthors = #$00A9' Graeme Geldenhuys et al.';
  License = 'Licensed under the MPL 2.0';
  FpgLicense = 'Licensed under the LGPL 2.1 with static linking exception';
  BtnWidth = 85;
  InitFromHeight = 450;
  InitTextHeight = 18;
  NewLine = #13#10;

type
  TFrmAbout = class(TfpgForm)
    txtRelNotes: TfpgPanel;
    txtBugURL: TfpgPanel;
    txtDownloadSite: TfpgPanel;
    txtPluginVersion: TfpgPanel;
    txtAuthor: TfpgPanel;
    txtMaintainer: TfpgPanel;
    txtLicense: TfpgPanel;
    txtFpgAuthors: TfpgPanel;
    lblFpgLicense: TfpgPanel;
    txtFpgLicense: TfpgPanel;
    lblUnicodeFormat: TfpgPanel;
    txtUnicodeConfig: TfpgPanel;
    lblConfigDir: TfpgPanel;
    txtConfigDir: TfpgPanel;
    lblEntities: TfpgPanel;
    txtEntities: TfpgPanel;
    lblSpacer1: TfpgPanel;
    lblSpacer2: TfpgPanel;
    btnSpacer: TfpgPanel;
    btnClose: TfpgButton;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure AfterCreate; override;
    procedure DoOnShow({%H-}Sender: TObject);
  published
    procedure FormClose({%H-}Sender: TObject);
    procedure GoToChangelog({%H-}Sender: TObject);
    procedure GoToEntities({%H-}Sender: TObject);
    procedure ShowConfigForm({%H-}Sender: TObject);
    procedure FollowPath(Sender: TObject);
    procedure ShowLink(Sender: TObject);
    procedure RevertCursor(Sender: TObject);
  private
    FVersion: TFileVersionInfo;
    FDLLName: WideString;
    FEntities: WideString;
    FDidResize: boolean;
    FDidStyleChange: boolean;
    property DidResize: boolean read FDidResize write FDidResize default False;
    procedure FindEntities;
    procedure SetConfigFilePath(Path: TfpgPanel);
    procedure WrapFilePath(Path: TfpgPanel);
    procedure SetUrl(Lbl: TfpgPanel);
    function MakeText(const Txt: string; const Height: TfpgCoord = InitTextHeight): TfpgPanel;
    procedure SetDefaultStyles;
    procedure SetDarkStyles;
  end;

implementation

uses
  SysUtils,
  Windows,
  ModulePath,
  NppPlugin,
  ConfigForm,
  U_Npp_HTMLTag;

constructor TFrmAbout.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  try
    FDLLName := TModulePath.DLLFullName;
    FVersion := TFileVersionInfo.Create(FDLLName);
    Width := 575;
    Height := InitFromHeight;
    if Npp.HasNarrowDialogBorders then
      SetHeight(Height + GetSystemMetrics(SM_CXPADDEDBORDER)*3);
    BackgroundColor := clWhite;
    WindowAttributes := [waBorderless];
    WindowPosition := wpScreenCenter;
    Sizeable := False;
    OnShow := DoOnShow;
    WindowTitle := 'About';

    if Assigned(FVersion) then
      SetWindowTitle('  ' + UTF8Encode(FVersion.FileDescription));

    txtPluginVersion := MakeText(UTF8ToAnsi(UTF8Encode(Npp.Version)), 24);
    txtPluginVersion.FontDesc := 'Tahoma-9';

    txtRelNotes := MakeText('Release Notes', 24);
    SetUrl(txtRelNotes);
    txtRelNotes.OnClick := GoToChangelog;

    if Assigned(FVersion) then
    begin
      txtBugURL := MakeText('Bugs', 24);
      txtBugURL.Hint := UTF8Encode(FVersion.Comments);
      SetUrl(txtBugURL);
    end;

    txtDownloadSite := MakeText('Downloads', 24);
    txtDownloadSite.Hint := Repo;
    SetUrl(txtDownloadSite);
    lblSpacer1 := MakeText(' ', 8);

    if Assigned(FVersion) then
    begin
      txtAuthor := MakeText(UTF8Encode(WideFormat(Author, [FVersion.LegalCopyright])));
    end;

    txtMaintainer := MakeText(Maintainer);
    txtLicense := MakeText(License, 32);

    lblFpgLicense := MakeText('Using the fpGUI Toolkit');
    txtFpgAuthors := MakeText(FpgAuthors);
    txtFpgLicense := MakeText(FpgLicense);
    lblSpacer2 := MakeText(' ', 8);

    lblConfigDir := MakeText('Config location');
    txtConfigDir := MakeText(UTF8ToAnsi(UTF8Encode(Npp.PluginConfigDir)), 24);
    WrapFilePath(txtConfigDir);

    lblEntities := MakeText('HTML entities file');
    txtEntities := MakeText('', 24);

    lblUnicodeFormat := MakeText('Unicode character format: ' + Npp.Options.UnicodePrefix + '0000');
    txtUnicodeConfig := MakeText('Configure', 24);
    SetUrl(txtUnicodeConfig);
    txtUnicodeConfig.Onclick := ShowConfigForm;
    txtUnicodeConfig.FontDesc := FPG_DEFAULT_FONT_DESC;

    btnSpacer := MakeText(' ', 12);

    btnClose := CreateButton(self, 0, 0, BtnWidth, 'OK', FormClose);
    with btnClose do
    begin
      Align := AlClient;
      Flat := True;
      MaxHeight := (BtnWidth div 2);
      TabOrder := 0;
    end;

  except
    on E: Exception do
    begin
      MessageBoxW(Npp.App.WindowHandle, PWideChar(Utf8DEcode(E.Message)), PWideChar(Utf8DEcode(E.Message)),
        MB_ICONERROR);
    end;
  end;
end;

destructor TFrmAbout.Destroy;
begin
  if Assigned(FVersion) then
    FreeAndNil(FVersion);

  inherited;
end;

procedure TFrmAbout.AfterCreate;
begin
    inherited;
    FindEntities;
end;

procedure TFrmAbout.DoOnShow({%H-}Sender: TObject);
begin
  if Npp.DarkModeEnabled then
    SetDarkStyles
  else if FDidStyleChange then
    SetDefaultStyles;
  FindEntities;
  btnClose.Focused := True;
end;

procedure TFrmAbout.FormClose({%H-}Sender: TObject);
begin
  Close;
end;

procedure TFrmAbout.GoToChangelog({%H-}Sender: TObject);
var
  ChangeLog: WideString;
begin
  ChangeLog := 'https://bitbucket.org/rdipardo/htmltag/src/HEAD/NEWS.textile';

  if Assigned(FVersion) then
    ChangeLog := WideFormat(
      'https://bitbucket.org/rdipardo/htmltag/src/v%d.%d.%d/NEWS.textile',
      [FVersion.MajorVersion, FVersion.MinorVersion, FVersion.Revision]);

  Npp.ShellExecute(ChangeLog);
  Close;
end;

procedure TFrmAbout.GoToEntities({%H-}Sender: TObject);
begin
  Npp.DoOpen(FEntities);
  Close;
end;

procedure TFrmAbout.ShowConfigForm({%H-}Sender: TObject);
var
  Frm : TFrmConfig;
  Current: ShortString;
begin
  Current := Npp.Options.UnicodePrefix;
  try
    fpgApplication.CreateForm(TFrmConfig, Frm);
    if (Frm.ShowModal = mrOK) then
      lblUnicodeFormat.Text := LeftStr(lblUnicodeFormat.Text, 25) +
        StringReplace(Copy(lblUnicodeFormat.Text, 26, MaxInt), Current, Npp.Options.UnicodePrefix, []);
  finally
    FreeAndNil(Frm);
  end;
end;

procedure TFrmAbout.FollowPath(Sender: TObject);
begin
  Npp.ShellExecute(PChar(StringReplace(TfpgPanel(Sender).Hint, NewLine, '', [])));
  Close;
end;

procedure TFrmAbout.ShowLink(Sender: TObject);
begin
  TfpgPanel(Sender).MouseCursor := mcHand;
if Npp.DarkModeEnabled then
  TfpgPanel(Sender).TextColor := fpgColor($FF, $D7, $0)
else
  TfpgPanel(Sender).TextColor := clPeru;
  TfpgPanel(Sender).FontDesc := TfpgPanel(Sender).FontDesc + ':Underline';
end;

procedure TFrmAbout.RevertCursor(Sender: TObject);
begin
  TfpgPanel(Sender).MouseCursor := mcDefault;
if Npp.DarkModeEnabled then
  TfpgPanel(Sender).TextColor := fpgColor($0, $BF, $FF)
else
  TfpgPanel(Sender).TextColor := clHyperLink;
  TfpgPanel(Sender).FontDesc := StringReplace(TfpgPanel(Sender).FontDesc, ':Underline', '', []);
end;

function TFrmAbout.MakeText(const Txt: string; const Height: TfpgCoord): TfpgPanel;
begin
  Result := CreatePanel(self, 0, 0, self.Width, Height, txt, bsFlat, taCenter);
  Result.Align := alTop;
  Result.LineSpace := -1;
  Result.ParentBackgroundColor := True;
end;

procedure TFrmAbout.SetUrl(Lbl: TfpgPanel);
begin
  with Lbl do
  begin
    OnMouseEnter := ShowLink;
    OnMouseExit := RevertCursor;
    OnClick := FollowPath;
  if Npp.DarkModeEnabled then
    TextColor := fpgColor($0, $BF, $FF)
  else
    TextColor := clHyperLink;
    FontDesc := 'Tahoma-9';
  end;
end;

procedure TFrmAbout.SetConfigFilePath(Path: TfpgPanel);
begin
  with Path do
  begin
    if FileExists(FEntities) then
    begin
      SetUrl(Path);
      Path.OnClick := GoToEntities;
      FontDesc := FPG_DEFAULT_FONT_DESC;
    end
    else
    begin
      Text := 'Not found';
      FontDesc := StringReplace(FontDesc, ':Underline', '', []);
  if Npp.DarkModeEnabled then
      TextColor := fpgColor($FF, $63, $47)
  else
      TextColor := clCrimson;
      OnMouseEnter := nil;
      OnMouseExit := nil;
      OnClick := nil;
    end;
  end;
  WrapFilePath(Path);
end;

procedure TFrmAbout.FindEntities;
begin
  FEntities := Npp.Entities;

  if not FileExists(FEntities) then
    FEntities := Npp.DefaultEntitiesPath;

  txtEntities.Text := UTF8Encode(FEntities);

  SetConfigFilePath(txtEntities);
  DidResize := ((self.Height > InitFromHeight) or (txtEntities.Height > InitTextHeight));
end;

procedure TFrmAbout.WrapFilePath(Path: TfpgPanel);
var
  Txt: string;
  Bump, WrapAt: TfpgCoord;
  OS: TWinVer;
  i, LineSpc: integer;
begin
  with Path do
  begin
    Txt := Text;
    OS := TWinVer(Npp.App.SendMessage(NPPM_GETWINDOWSVERSION));

    if (OS <= WV_WIN7) then
    begin
      WrapAt := (self.Width div 5);
      Bump := (Length(Text) div 16);
    end
    else
    begin
      WrapAt := (BtnWidth div 10)*9;
      if (OS > WV_WIN10) then
        LineSpc := 8
      else
        LineSpc := 4;
      Bump := (Length(Text) div LineSpc);
    end;

    if Length(Txt) > WrapAt then
    begin
      // break long path names at directory separator
      for i := 1 to Length(Txt) do
      begin
        if (Txt[i] = PathDelim) and (i >= (Length(Text) div 2)) then
        begin
          Text := Concat(LeftStr(Txt, i), NewLine,
            RightStr(Txt, Length(Txt) - i));
          break;
        end;
      end;
      if (not self.DidResize) then
      begin
        Height := Height + Bump;
        self.Height := self.Height + Bump;
      end;
    end;
  end;
end;

procedure TFrmAbout.SetDefaultStyles;
begin
  txtPluginVersion.TextColor := clBlack;
  txtRelNotes.TextColor := clHyperLink;
  txtBugURL.TextColor := clHyperLink;
  txtDownloadSite.TextColor := clHyperLink;
  txtAuthor.TextColor := clBlack;
  txtMaintainer.TextColor := clBlack;
  txtLicense.TextColor := clBlack;
  lblFpgLicense.TextColor := clBlack;
  txtFpgAuthors.TextColor := clBlack;
  txtFpgLicense.TextColor := clBlack;
  lblUnicodeFormat.TextColor := clBlack;
  txtUnicodeConfig.TextColor := clHyperLink;
  lblConfigDir.TextColor := clBlack;
  txtConfigDir.TextColor := clBlack;
  lblEntities.TextColor := clBlack;
  txtEntities.TextColor := clHyperLink;
  btnClose.BackgroundColor := clButtonFace;
  btnClose.TextColor := clBlack;
  Self.BackgroundColor := clWhite;
  Self.TextColor := clBlack;
  FDidStyleChange := False;
end;

procedure TFrmAbout.SetDarkStyles;
begin
  txtPluginVersion.TextColor := clWhite;
  txtRelNotes.TextColor := fpgColor($0, $BF, $FF);
  txtBugURL.TextColor := fpgColor($0, $BF, $FF);
  txtDownloadSite.TextColor := fpgColor($0, $BF, $FF);
  txtAuthor.TextColor := clWhite;
  txtMaintainer.TextColor := clWhite;
  txtLicense.TextColor := clWhite;
  lblFpgLicense.TextColor := clWhite;
  txtFpgAuthors.TextColor := clWhite;
  txtFpgLicense.TextColor := clWhite;
  lblUnicodeFormat.TextColor := clWhite;
  txtUnicodeConfig.TextColor := fpgColor($0, $BF, $FF);
  lblConfigDir.TextColor := clWhite;
  txtConfigDir.TextColor := clWhite;
  lblEntities.TextColor := clWhite;
  txtEntities.TextColor := fpgColor($0, $BF, $FF);
  btnClose.BackgroundColor := fpgColor($48, $48, $4E);
  btnClose.TextColor := clWhite;
  Self.BackgroundColor := clBlack;
  Self.TextColor := clWhite;
  FDidStyleChange := True;
end;

end.
