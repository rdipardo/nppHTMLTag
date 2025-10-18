/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024,2025 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef ABOUT_DLG_H
#define ABOUT_DLG_H

#include "LocalizedPlugin.h"
#include "StaticDialog.h"
#include "resource.h"

using NppDarkMode::dmfInit;

class AboutDlg final : public StaticDialog {
public:
	explicit AboutDlg(HINSTANCE hInst, NppData const &data);
	void toggleDarkMode(HWND hwnd, ULONG dmFlag = dmfInit);
	void localize(HWND hwnd);
	void show();

private:
	bool _themeInitialized = false;
	bool _isNonLatin = false, _isRTL = false, _isCJK = false, _isBrahmic = false, _isCyrillic = false;
	void alignText(HWND hwndDlg, int id, std::wstring const &text, HDC const &hdc, RECT const &rc);
	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};
#endif // ~ABOUT_DLG_H
