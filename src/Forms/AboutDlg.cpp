/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <windows.h>
#include <shellapi.h>
#include "VersionInfo.h"
#include "TextConv.h"
#include "HtmlTag.h"
#include "AboutDlg.h"

// Handle static text in default theme mode
#define WM_CTLCOLORSTATIC_LITE WM_CTLCOLORSTATIC

// Handle static text in dark theme mode
#define WM_CTLCOLORSTATIC_DARK WM_DRAWITEM

using namespace HtmlTag;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
struct DialogHyperlink {
	int id;
	WNDPROC defWndProc;
};

INT_PTR CALLBACK modalDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK linkCtrlWndProc(HWND hLink, UINT message, WPARAM wParam, LPARAM lParam);

Version pluginVersion;
HFONT hDefaultFont, hActiveLinkFont;
bool themeInitialized = false;

DialogHyperlink linkCtrls[] = {
	{ ID_RELEASE_NOTES_LINK, nullptr },
	{ ID_BUG_TRACKER_LINK, nullptr },
	{ ID_PLUGIN_REPO_LINK, nullptr },
	{ ID_ENTITIES_FILE_LINK, nullptr },
	{ ID_TRANSLATIONS_FILE_LINK, nullptr },
	{ ID_UNICODE_CONFIG_LINK, nullptr },
	{ ID_SIMPLEINI_LINK, nullptr },
	{ ID_TINYXML_LINK, nullptr },
};
constexpr size_t nbLinkCtrls = ARRAYSIZE(linkCtrls);
}

// --------------------------------------------------------------------------------------
// AboutDlg
// --------------------------------------------------------------------------------------
AboutDlg::AboutDlg(HINSTANCE hInst, NppData const &data) : StaticDialog() {
	pluginVersion = Version{ HTMLTAG_VERSION_WORDS };
	Window::init(hInst, data._nppHandle);
}
// --------------------------------------------------------------------------------------
void AboutDlg::show() {
	if (!isCreated())
		create(ID_ABOUT_HTML_TAG_DLG);

	goToCenter();
}
// --------------------------------------------------------------------------------------
void AboutDlg::toggleDarkMode(HWND hwnd, ULONG dmFlag) {
	if (!plugin.supportsDarkModeSubclassing())
		return;

	if (hwnd == _hSelf) {
		for (size_t i = 0; i < nbLinkCtrls; i++) {
			HWND hLink = ::GetDlgItem(hwnd, linkCtrls[i].id);
			LONG_PTR wstyle = static_cast<LONG_PTR>(::GetWindowLongPtrW(hLink, GWL_STYLE));
			wstyle = (plugin.isDarkModeEnabled() ? wstyle | SS_OWNERDRAW : wstyle & ~SS_OWNERDRAW);
			::SetWindowLongPtrW(hLink, GWL_STYLE, wstyle);
		}
	}
	plugin.sendNppMessage(NPPM_DARKMODESUBCLASSANDTHEME, dmFlag, reinterpret_cast<LPARAM>(hwnd));
}
// --------------------------------------------------------------------------------------
INT_PTR CALLBACK AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
	INT_PTR result = FALSE;
	switch (message) {
		case WM_INITDIALOG: {
			LOGFONT lf;
			hDefaultFont = reinterpret_cast<HFONT>(::SendMessageW(_hSelf, WM_GETFONT, 0, 0));
			::GetObjectW(hDefaultFont, sizeof(LOGFONT), &lf);
			lf.lfUnderline = TRUE;
			hActiveLinkFont = ::CreateFontIndirectW(&lf);

			for (size_t i = 0; i < nbLinkCtrls; i++) {
				HWND hCtrl = ::GetDlgItem(_hSelf, linkCtrls[i].id);
				linkCtrls[i].defWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
				    hCtrl, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(linkCtrlWndProc)));
				::SendMessageW(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hActiveLinkFont), 0);
			}

			std::wstringstream version;
			version << L"Version " << pluginVersion.str() << L" (" << sizeof(intptr_t) * 8 << L"-bit"
#ifdef _M_ARM
				<< L" ARM"
#endif
				<< L")";
			::SetDlgItemTextW(_hSelf, ID_PLUGIN_VERSION_TXT, &(version.str())[0]);
			result = TRUE;
			break;
		}
		case WM_DESTROY: {
			for (size_t i = 0; i < nbLinkCtrls; i++) {
				::SetWindowLongPtrW(::GetDlgItem(_hSelf, linkCtrls[i].id), GWLP_WNDPROC,
				    reinterpret_cast<LONG_PTR>(linkCtrls[i].defWndProc));
			}
			::DeleteObject(hActiveLinkFont);
			break;
		}
		case WM_ACTIVATE: {
			if ((wParam & 0xffff) != WA_INACTIVE) {
				std::wstring prefixTxt(plugin.options.unicodePrefix.size() + 1, L'\0');
				TextConv::bytesToText(plugin.options.unicodePrefix.c_str(), prefixTxt, CP_ACP);
				prefixTxt.append(L"0000");
				::SetDlgItemTextW(_hSelf, ID_UNICODE_USER_FMT_TXT, &(prefixTxt)[0]);
			}
			toggleDarkMode(_hSelf, themeInitialized ? dmfHandleChange : dmfInit);
			themeInitialized = true;
			result = TRUE;
			break;
		}
		case WM_CTLCOLORSTATIC_LITE: {
			HDC hdc = reinterpret_cast<HDC>(wParam);
			int id = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			for (size_t i = 0; i < nbLinkCtrls; i++) {
				if (id == linkCtrls[i].id) {
					::SetTextColor(hdc, CL_LINK_DEFAULT);
					break;
				}
			}
			return reinterpret_cast<INT_PTR>(::GetSysColorBrush(::GetBkColor(hdc)));
		}
		case WM_CTLCOLORSTATIC_DARK: {
			LPDRAWITEMSTRUCT lpdi = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			if (lpdi->CtlType != ODT_STATIC)
				break;

			for (size_t i = 0; i < nbLinkCtrls; i++) {
				if (lpdi->CtlID == (UINT)linkCtrls[i].id) {
					wchar_t txtBuf[256]{};
					int txtLen = static_cast<int>(::SendMessageW(lpdi->hwndItem, WM_GETTEXT,
					    ARRAYSIZE(txtBuf), reinterpret_cast<LPARAM>(txtBuf)));
					::SetTextColor(lpdi->hDC, CL_LINK_DARK_MODE);
					::TextOutW(lpdi->hDC, lpdi->rcItem.left, lpdi->rcItem.top, txtBuf, txtLen);
					break;
				}
			}
			result = TRUE;
			break;
		}
		case WM_COMMAND: {
			std::wstring targetURL;
			bool hideOnReturn = true;
			switch (wParam & 0xffff) {
				case ID_UNICODE_CONFIG_LINK:
					::DialogBoxParamW(_hInst, MAKEINTRESOURCE(ID_UNICODE_FMT_CONFIG_DLG), _hSelf,
					    (DLGPROC)modalDlgProc, reinterpret_cast<LPARAM>(this));
					hideOnReturn = false;
					break;
				case ID_TRANSLATIONS_FILE_LINK:
					plugin.openFile((wchar_t *)&plugin.translations.native()[0]);
					break;
				case ID_ENTITIES_FILE_LINK:
					plugin.openFile((wchar_t *)&plugin.entities.native()[0]);
					break;
				case ID_RELEASE_NOTES_LINK: {
					std::wstring url = RELEASE_NOTES_URL;
					url = url.replace(url.find_first_of(L"HEAD"), url.size(),
					    L"v" + pluginVersion.str() + L"/NEWS.textile");
					targetURL = (url.find(pluginVersion.str()) != std::wstring::npos)
							? url
							: RELEASE_NOTES_URL;
					break;
				}
				case ID_BUG_TRACKER_LINK:
					targetURL = BUG_TRACKER_URL;
					break;
				case ID_PLUGIN_REPO_LINK:
					targetURL = PLUGIN_REPO_URL;
					break;
				case ID_SIMPLEINI_LINK:
					targetURL = SIMPLEINI_URL;
					break;
				case ID_TINYXML_LINK:
					targetURL = TINYXML_URL;
					break;
			}

			if (!targetURL.empty())
				::ShellExecuteW(0, L"open", &targetURL[0], nullptr, nullptr, SW_SHOWNORMAL);

			if (hideOnReturn)
				display(false);

			result = TRUE;
			break;
		}
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
INT_PTR CALLBACK modalDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	auto setEditText = [hwndDlg]() {
		HWND hEdit = ::GetDlgItem(hwndDlg, ID_CONFIG_EDIT);
		std::wstring editTxt(plugin.options.unicodePrefix.size() + 1, L'\0');
		TextConv::bytesToText(plugin.options.unicodePrefix.c_str(), editTxt, CP_ACP);
		::SetDlgItemTextW(hwndDlg, ID_CONFIG_EDIT, &editTxt[0]);
		::SetFocus(hEdit);
		::SendMessageW(hEdit, EM_SETSEL, 0, -1);
	};

	INT_PTR result = FALSE;
	switch (message) {
		case WM_INITDIALOG: {
			AboutDlg *aboutDlg = reinterpret_cast<AboutDlg *>(lParam);
			if (aboutDlg)
				aboutDlg->toggleDarkMode(hwndDlg);

			setEditText();
			result = TRUE;
			break;
		}
		case WM_COMMAND:
			switch (wParam & 0xffff) {
				case IDOK: {
					std::wstring editValue(64, L'\0');
					::GetDlgItemTextW(hwndDlg, ID_CONFIG_EDIT, &(editValue)[0], 63);
					TextConv::trim<std::wstring>(editValue);
					std::string newPrefix(sizeof(wchar_t) * editValue.size() + 1, 0);
					TextConv::textToBytes(editValue.c_str(), newPrefix);
					plugin.setUnicodeFormatOption(newPrefix);
					[[fallthrough]];
				}
				case IDCANCEL:
					::EndDialog(hwndDlg, wParam);
					break;
				case IDRETRY:
					setEditText();
					break;
			}
			result = TRUE;
			break;
	}
	return result;
}
// --------------------------------------------------------------------------------------
INT_PTR CALLBACK linkCtrlWndProc(HWND hCtrl, UINT message, WPARAM wParam, LPARAM lParam) {
	WNDPROC defWndProc = ::DefWindowProcW;
	for (size_t i = 0; i < nbLinkCtrls; i++) {
		if (linkCtrls[i].id == ::GetDlgCtrlID(hCtrl)) {
			defWndProc = linkCtrls[i].defWndProc;
			break;
		}
	}
	switch (message) {
		case WM_SETCURSOR: {
			HCURSOR hCursor = reinterpret_cast<HCURSOR>(
			    ::LoadImageW(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
			if (hCursor)
				::SetCursor(hCursor);

			return TRUE;
		}
	}
	return defWndProc(hCtrl, message, wParam, lParam);
}
}
