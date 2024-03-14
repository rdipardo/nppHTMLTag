unit ConfigForm;

{$IFDEF FPC}{$mode delphi}{$ENDIF}

{
  Copyright (c) 2023 Robert Di Pardo <dipardo.r@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public License,
  v. 2.0. If a copy of the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
}

interface

uses
  Classes,
  fpg_base,
  fpg_widget,
  fpg_form,
  fpg_label,
  fpg_memo,
  fpg_button;

const
  FrmHeight = 112;
  FrmWidth = 400;
  BtnWidth = 85;
  MaxPrefixLen = 6;

type
  TFrmConfig = class(TfpgForm)
    txtPrefix: TfpgMemo;
    lblFmt, lblDigits: TfpgLabel;
    btnOK, btnCancel, btnReset: TfpgButton;
    constructor Create(AOwner: TComponent); override;
    procedure AfterConstruction; override;
    procedure DoOnClose(var CloseAction: TCloseAction); override;
    procedure BtnOKClick({%H-}Sender: TObject);
    procedure BtnResetClick({%H-}Sender: TObject);
    procedure ValidatePrefix(Sender: TObject);
    procedure SetUpButton(var Btn: TfpgButton; Value: TfpgModalResult; Index: SmallInt; Icon: TfpgString);
  end;

implementation

uses U_Npp_HTMLTag;

constructor TFrmConfig.Create(AOwner: TComponent);
var
  BtnTop: TFpgCoord;
begin
  inherited Create(AOwner);
  WindowTitle := 'Change Unicode character prefix';
  WindowPosition := wpScreenCenter;
  Height := FrmHeight;
  Width := FrmWidth;
  Sizeable := False;
  BtnTop := (FrmHeight div 25)*17;
  btnOK := CreateButton(self, ((FrmWidth div 2) - ((BtnWidth div 2)*3) - 12), BtnTop, BtnWidth, 'OK', BtnOKClick);
  btnReset := CreateButton(self, (btnOK.Left + BtnWidth + 8), BtnTop, BtnWidth, 'Reset', BtnResetClick);
  btnCancel := CreateButton(self, (btnReset.Left + BtnWidth + 8), BtnTop, BtnWidth, 'Cancel', nil);
  lblFmt := CreateLabel(self, (FrmWidth div 4), (self.Height div 5), 'Format:', 64, 28);
  txtPrefix := CreateMemo(Self, (lblFmt.Left + lblFmt.Width + 2), (lblFmt.Top - 2), 68, 28);
  lblDigits := CreateLabel(self, (txtPrefix.Left + txtPrefix.Width + 2), lblFmt.Top, '0000', 64, 28);
  with txtPrefix do begin
    FontDesc := 'Consolas-8';
    TabOrder := 0;
    OnPaint := ValidatePrefix;
  end;
  lblDigits.FontDesc := txtPrefix.FontDesc + ':italic';
  SetUpButton(btnOK, mrOk, 1, 'stdimg.ok');
  SetUpButton(btnCancel, mrCancel, 2, 'stdimg.quit');
  SetUpButton(btnReset, mrNone, 3, 'stdimg.refresh');
  BtnResetClick(nil);
end;

procedure TFrmConfig.AfterConstruction;
var
  i: Cardinal;
begin
  inherited;
  if Npp.DarkModeEnabled then begin
    for i := 0 to Self.ComponentCount-1 do begin
      with TfpgWidget(Components[i]) do begin
        TextColor := clWhite;
        if InheritsFrom(TfpgLabel) then
          BackgroundColor := clBlack
        else
          BackgroundColor := fpgColor($3E, $3E, $40);
      end;
    end;
    Self.BackgroundColor := clBlack;
  end else begin
    for i := 0 to Self.ComponentCount-1 do begin
      with TfpgWidget(Components[i]) do begin
        TextColor := clBlack;
        if InheritsFrom(TfpgButton) then
          BackgroundColor := clButtonFace
        else
          BackgroundColor := clBoxColor;
      end;
    end;
    Self.BackgroundColor:= clBoxColor;
  end;
end;

procedure TFrmConfig.DoOnClose(var CloseAction: TCloseAction);
begin
  inherited;
  CloseAction := caHide;
end;

procedure TFrmConfig.BtnOKClick({%H-}Sender: TObject);
begin
  Npp.SetUnicodeFormatOptions(txtPrefix.Text);
end;

procedure TFrmConfig.BtnResetClick({%H-}Sender: TObject);
begin
  txtPrefix.Text := Npp.Options.UnicodePrefix;
  ValidatePrefix(txtPrefix);
end;

procedure TFrmConfig.ValidatePrefix(Sender: TObject);
var
  Input : TfpgString;
begin
  Input := TfpgMemo(Sender).Text;
  btnOK.Enabled := (Length(Input) > 0) and (Ord(Input[1]) > $20);
  if Length(Input) > MaxPrefixLen then
    TfpgMemo(Sender).Text:= Copy(Input, 1, MaxPrefixLen);
end;

procedure TFrmConfig.SetUpButton(var Btn: TfpgButton; Value: TfpgModalResult; Index: SmallInt; Icon: TfpgString);
begin
  with Btn do begin
    ModalResult := Value;
    TabOrder := Index;
    ImageName := Icon;
    ShowImage := (Icon <> '');
  end;
end;

end.
