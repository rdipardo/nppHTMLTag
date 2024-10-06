/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef ABOUT_DLG_H
#define ABOUT_DLG_H

#include "PluginInterface.h"
#include "StaticDialog.h"
#include "resource.h"

using namespace NppDarkMode;

class AboutDlg final : public StaticDialog {
public:
	explicit AboutDlg(HINSTANCE hInst, NppData const &data);
	void toggleDarkMode(HWND hwnd, ULONG dmFlag = dmfInit);
	void show();

private:
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};
#endif // ~ABOUT_DLG_H
