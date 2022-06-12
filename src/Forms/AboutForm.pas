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
  L_VersionInfoW;

const
  Repo = 'https://bitbucket.org/rdipardo/htmltag/downloads';
  Author = #$00A9' 2011-2020 %s (v0.1 - v1.1)';
  Maintainer = #$00A9' 2022 Robert Di Pardo (since v1.2)';
  FpgAuthors = #$00A9' Graeme Geldenhuys et al.';
  License = 'Licensed under the MPL 2.0';
  FpgLicense = 'Licensed under the LGPL 2.1 with static linking exception';
  EntitiesConf = 'HTMLTag-entities.ini';
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
    lblHomeDir: TfpgPanel;
    txtHomeDir: TfpgPanel;
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
  published
    procedure DoOnShow({%H-}Sender: TObject);
    procedure FormClose({%H-}Sender: TObject);
    procedure GoToChangelog({%H-}Sender: TObject);
    procedure FollowPath(Sender: TObject);
    procedure ShowLink(Sender: TObject);
    procedure RevertCursor(Sender: TObject);
  private
    FVersion: TFileVersionInfo;
    FDLLName: string;
    FDidResize: boolean;
    property DidResize: boolean read FDidResize write FDidResize default False;
    procedure FindEntities;
    procedure SetConfigFilePath(Path: TfpgPanel);
    procedure WrapFilePath(Path: TfpgPanel);
    procedure SetUrl(Lbl: TfpgPanel);
    function MakeText(const Txt: string; const Height: TfpgCoord = InitTextHeight): TfpgPanel;
  end;

implementation

uses
  SysUtils,
  StrUtils,
  Windows,
  L_SpecialFolders,
  NppPlugin,
  U_Npp_HTMLTag;

constructor TFrmAbout.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  try
    FDLLName := TSpecialFolders.DLLFullName;
    FVersion := TFileVersionInfo.Create(FDLLName);
    Width := 575;
    Height := InitFromHeight;
    BackgroundColor := clWhite;
    WindowAttributes := [waBorderless];
    WindowPosition := wpScreenCenter;
    Sizeable := False;
    OnShow := DoOnShow;
    WindowTitle := 'About';

    if Assigned(FVersion) then
      SetWindowTitle('  ' + FVersion.FileDescription);

    txtPluginVersion := MakeText(UTF8ToAnsi(UTF8Encode(Npp.Version)), 24);
    txtPluginVersion.FontDesc := 'Tahoma-9';

    txtRelNotes := MakeText('Release Notes', 24);
    SetUrl(txtRelNotes);
    txtRelNotes.OnClick := GoToChangelog;

    if Assigned(FVersion) then
    begin
      txtBugURL := MakeText('Bugs', 24);
      txtBugURL.Hint := FVersion.Comments;
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

    lblHomeDir := MakeText('Plugin location');
    txtHomeDir := MakeText(ExtractFileDir(FDLLName), 24);
    WrapFilePath(txtHomeDir);

    lblConfigDir := MakeText('Config location');
    txtConfigDir := MakeText(UTF8ToAnsi(UTF8Encode(Npp.App.ConfigFolder)), 24);
    WrapFilePath(txtConfigDir);

    lblEntities := MakeText('HTML entities file');
    txtEntities := MakeText('', 24);

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
      MessageBox(Npp.App.WindowHandle, PChar(E.Message), PChar(E.Message),
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

procedure TFrmAbout.DoOnShow({%H-}Sender: TObject);
begin
  FindEntities;
  btnClose.Focused := True;
end;

procedure TFrmAbout.FormClose({%H-}Sender: TObject);
begin
  Close;
end;

procedure TFrmAbout.GoToChangelog({%H-}Sender: TObject);
var
  ChangeLog: string;
begin
  ChangeLog := 'https://bitbucket.org/rdipardo/htmltag/src/HEAD/NEWS.textile';

  if Assigned(FVersion) then
    ChangeLog := Format(
      'https://bitbucket.org/rdipardo/htmltag/src/v%d.%d.%d/NEWS.textile',
      [FVersion.MajorVersion, FVersion.MinorVersion, FVersion.Revision]);

  Npp.ShellExecute(ChangeLog);
  Close;
end;

procedure TFrmAbout.FollowPath(Sender: TObject);
begin
  Npp.ShellExecute(PChar(ReplaceStr(TfpgPanel(Sender).Hint, NewLine, '')));
  Close;
end;

procedure TFrmAbout.ShowLink(Sender: TObject);
begin
  TfpgPanel(Sender).MouseCursor := mcHand;
  TfpgPanel(Sender).TextColor := clPeru;
  TfpgPanel(Sender).FontDesc := TfpgPanel(Sender).FontDesc + ':Underline';
end;

procedure TFrmAbout.RevertCursor(Sender: TObject);
begin
  TfpgPanel(Sender).MouseCursor := mcDefault;
  TfpgPanel(Sender).TextColor := clHyperLink;
  TfpgPanel(Sender).FontDesc := ReplaceStr(TfpgPanel(Sender).FontDesc, ':Underline', '');
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
    TextColor := clHyperLink;
    FontDesc := 'Tahoma-9';
  end;
end;

procedure TFrmAbout.SetConfigFilePath(Path: TfpgPanel);
begin
  with Path do
  begin
    if FileExists(Text) then
    begin
      Hint := Text;
      SetUrl(Path);
      FontDesc := FPG_DEFAULT_FONT_DESC;
    end
    else
    begin
      Text := 'Not found';
      TextColor := clCrimson;
      OnMouseEnter := nil;
      OnMouseExit := nil;
      OnClick := nil;
    end;
  end;
  WrapFilePath(Path);
end;

procedure TFrmAbout.FindEntities;
var
  Entities: string;
begin
  Entities :=
    IncludeTrailingPathDelimiter(ReplaceStr(txtConfigDir.Text, NewLine, '')) +
    EntitiesConf;

  if not FileExists(Entities) then
    txtEntities.Text :=
      IncludeTrailingPathDelimiter(ReplaceStr(txtHomeDir.Text, NewLine, '')) +
      EntitiesConf
  else
    txtEntities.Text := Entities;

  SetConfigFilePath(txtEntities);
  DidResize := ((self.Height > InitFromHeight) or (txtEntities.Height > InitTextHeight));
end;

procedure TFrmAbout.WrapFilePath(Path: TfpgPanel);
var
  Txt: string;
  Bump, WrapAt: TfpgCoord;
  OS: TWinVer;
  i: integer;
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
      WrapAt := Round(BtnWidth * 0.8);
      Bump := (Length(Text) div 4);
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

end.