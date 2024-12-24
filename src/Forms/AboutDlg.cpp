/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <windows.h>
#include <shellapi.h>
#include "SimpleIni.h"
#include "VersionInfo.h"
#include "TextConv.h"
#include "HtmlTag.h"
#include "AboutDlg.h"
#include "dialogs.h"

// Handle static text in default theme mode
#define WM_CTLCOLORSTATIC_LITE WM_CTLCOLORSTATIC

// Handle static text in dark theme mode
#define WM_CTLCOLORSTATIC_DARK WM_DRAWITEM

using namespace HtmlTag;
using namespace TextConv;

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

constexpr LocalizedResource dialogLocales[] = {
	{ "arabic", AR_ABOUT_DLG, AR_UNICODE_DLG },
	{ "catalan", CA_ABOUT_DLG, CA_UNICODE_DLG },
	{ "chineseSimplified", ZH_ABOUT_DLG, ZH_UNICODE_DLG },
	{ "dutch", NL_ABOUT_DLG, NL_UNICODE_DLG },
	{ "farsi", FA_ABOUT_DLG, FA_UNICODE_DLG },
	{ "french", FR_ABOUT_DLG, FR_UNICODE_DLG },
	{ "german", DE_ABOUT_DLG, DE_UNICODE_DLG },
	{ "hebrew", HE_ABOUT_DLG, HE_UNICODE_DLG },
	{ "hindi", HI_ABOUT_DLG, HI_UNICODE_DLG },
	{ "italian", IT_ABOUT_DLG, IT_UNICODE_DLG },
	{ "japanese", JP_ABOUT_DLG, JP_UNICODE_DLG },
	{ "korean", KO_ABOUT_DLG, KO_UNICODE_DLG },
	{ "polish", PL_ABOUT_DLG, PL_UNICODE_DLG },
	{ "portuguese", PT_ABOUT_DLG, PT_UNICODE_DLG },
	{ "brazilian_portuguese", BR_PT_ABOUT_DLG, BR_PT_UNICODE_DLG },
	{ "romanian", RO_ABOUT_DLG, RO_UNICODE_DLG },
	{ "russian", RU_ABOUT_DLG, RU_UNICODE_DLG },
	{ "sinhala", SI_ABOUT_DLG, SI_UNICODE_DLG },
	{ "spanish", ES_ABOUT_DLG, ES_UNICODE_DLG },
	{ "spanish_ar", ES_ABOUT_DLG, ES_UNICODE_DLG },
	{ "tamil", TA_ABOUT_DLG, TA_UNICODE_DLG },
	{ "ukrainian", UK_ABOUT_DLG, UK_UNICODE_DLG },
};
constexpr size_t nbDialogLocales = ARRAYSIZE(dialogLocales);
}

// --------------------------------------------------------------------------------------
// AboutDlg
// --------------------------------------------------------------------------------------
AboutDlg::AboutDlg(HINSTANCE hInst, NppData const &data) : StaticDialog() {
	pluginVersion = Version{ HTMLTAG_VERSION_WORDS };
	for (size_t i = 0; i < nbDialogLocales; i++) {
		if (plugin.menuLocale() == dialogLocales[i].locale) {
			_dialogResource = dialogLocales[i];
			break;
		}
	}
	Window::init(hInst, data._nppHandle);
}
// --------------------------------------------------------------------------------------
void AboutDlg::show() {
	if (!isCreated())
		create(_dialogResource.dialog);

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
void AboutDlg::localize(HWND hwnd) {
	if (!std::filesystem::exists(plugin.dlgTranslations)) {
		if (hwnd == _hSelf) { // Restore default text to the About dialog
			::SetDlgItemTextW(hwnd, ID_PLUGIN_VERSION_TXT, DEFAULT_VERSION_TXT);
			::SetDlgItemTextW(hwnd, ID_RELEASE_NOTES_LINK, DEFAULT_RELEASE_NOTES_TXT);
			::SetDlgItemTextW(hwnd, ID_BUG_TRACKER_LINK, DEFAULT_BUG_TRACKER_TXT);
			::SetDlgItemTextW(hwnd, ID_PLUGIN_REPO_LINK, DEFAULT_REPO_LINK_TXT);
			::SetDlgItemTextW(hwnd, ID_PLUGIN_LICENSE_TXT, PLUGIN_LICENSE);
			::SetDlgItemTextW(hwnd, ID_SIMPLEINI_TXT, DEFAULT_ABOUT_3RD_PARTY);
			::SetDlgItemTextW(hwnd, ID_TINYXML_TXT, DEFAULT_ABOUT_3RD_PARTY_ALSO);
			::SetDlgItemTextW(hwnd, ID_ENTITIES_FILE_LINK, DEFAULT_ENTITIES_FILE_TXT);
			::SetDlgItemTextW(hwnd, ID_TRANSLATIONS_FILE_LINK, DEFAULT_L10N_FILE_TXT);
			::SetDlgItemTextW(hwnd, ID_UNICODE_FMT_LABEL_TXT, DEFAULT_UNICODE_FORMAT_LABEL);
			::SetDlgItemTextW(hwnd, ID_UNICODE_CONFIG_LINK, DEFAULT_UNICODE_CONFIG_LABEL);
		} else { // Restore default text to the Unicode format modal dialog
			::SetDlgItemTextW(hwnd, ID_CONFIG_LABEL_1, DEFAULT_UNICODE_EDIT_LABEL);
			::SetDlgItemTextW(hwnd, IDOK, DEFAULT_UNICODE_BTN_OK_TXT);
			::SetDlgItemTextW(hwnd, IDCANCEL, DEFAULT_UNICODE_BTN_CANCEL_TXT);
			::SetDlgItemTextW(hwnd, IDRETRY, DEFAULT_UNICODE_BTN_RESET_TXT);
		}
		return;
	}

	CSimpleIniW config{ /* IsUtf8 */ true, /* MultiKey */ false, /* MultiLine */ true };
	config.SetQuotes(true);

	try {
		SI_Error err = config.LoadFile(plugin.dlgTranslations.c_str());
		if (err != SI_OK)
			return;

		std::wstring section(64, L'\0');
		TextConv::bytesToText(_dialogResource.locale, section, CP_ACP);
		std::list<CSimpleIniW::Entry> keys;
		if (!config.GetAllKeys(section.c_str(), keys)) // Unknown language
			return;

		for (auto &&msgId : keys) {
			if (hwnd == _hSelf) { // Localize the About dialog
				if (sameString(msgId.pItem, L"about_caption")) {
					::SetWindowTextW(
					    hwnd, config.GetValue(section.c_str(), msgId.pItem, DEFAULT_CAPTION));
				} else if (sameString(msgId.pItem, L"about_version")) {
					::SetDlgItemTextW(hwnd, ID_PLUGIN_VERSION_TXT,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_VERSION_TXT));
				} else if (sameString(msgId.pItem, L"about_rel_notes")) {
					::SetDlgItemTextW(hwnd, ID_RELEASE_NOTES_LINK,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_RELEASE_NOTES_TXT));
				} else if (sameString(msgId.pItem, L"about_bugs")) {
					::SetDlgItemTextW(hwnd, ID_BUG_TRACKER_LINK,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_BUG_TRACKER_TXT));
				} else if (sameString(msgId.pItem, L"about_downloads")) {
					::SetDlgItemTextW(hwnd, ID_PLUGIN_REPO_LINK,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_REPO_LINK_TXT));
				} else if (sameString(msgId.pItem, L"about_license")) {
					::SetDlgItemTextW(hwnd, ID_PLUGIN_LICENSE_TXT,
					    config.GetValue(section.c_str(), msgId.pItem, PLUGIN_LICENSE));
				} else if (sameString(msgId.pItem, L"about_3rd_party")) {
					::SetDlgItemTextW(hwnd, ID_SIMPLEINI_TXT,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_ABOUT_3RD_PARTY));
				} else if (sameString(msgId.pItem, L"about_3rd_party_also")) {
					::SetDlgItemTextW(hwnd, ID_TINYXML_TXT,
					    config.GetValue(
						section.c_str(), msgId.pItem, DEFAULT_ABOUT_3RD_PARTY_ALSO));
				} else if (sameString(msgId.pItem, L"about_entities_file")) {
					::SetDlgItemTextW(hwnd, ID_ENTITIES_FILE_LINK,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_ENTITIES_FILE_TXT));
				} else if (sameString(msgId.pItem, L"about_l10n_file")) {
					::SetDlgItemTextW(hwnd, ID_TRANSLATIONS_FILE_LINK,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_L10N_FILE_TXT));
				} else if (sameString(msgId.pItem, L"about_unicode_format")) {
					::SetDlgItemTextW(hwnd, ID_UNICODE_FMT_LABEL_TXT,
					    config.GetValue(
						section.c_str(), msgId.pItem, DEFAULT_UNICODE_FORMAT_LABEL));
				} else if (sameString(msgId.pItem, L"about_unicode_config")) {
					::SetDlgItemTextW(hwnd, ID_UNICODE_CONFIG_LINK,
					    config.GetValue(
						section.c_str(), msgId.pItem, DEFAULT_UNICODE_CONFIG_LABEL));
				}
			} else { // Localize the Unicode format modal dialog
				if (sameString(msgId.pItem, L"unicode_dlg_caption")) {
					::SetWindowTextW(hwnd, config.GetValue(section.c_str(), msgId.pItem,
								   DEFAULT_UNICODE_EDIT_CAPTION));
				} else if (sameString(msgId.pItem, L"unicode_dlg_format")) {
					::SetDlgItemTextW(hwnd, ID_CONFIG_LABEL_1,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_UNICODE_EDIT_LABEL));
				} else if (sameString(msgId.pItem, L"unicode_dlg_ok")) {
					::SetDlgItemTextW(hwnd, IDOK,
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_UNICODE_BTN_OK_TXT));
				} else if (sameString(msgId.pItem, L"unicode_dlg_cancel")) {
					::SetDlgItemTextW(hwnd, IDCANCEL,
					    config.GetValue(
						section.c_str(), msgId.pItem, DEFAULT_UNICODE_BTN_CANCEL_TXT));
				} else if (sameString(msgId.pItem, L"unicode_dlg_reset")) {
					::SetDlgItemTextW(hwnd, IDRETRY,
					    config.GetValue(
						section.c_str(), msgId.pItem, DEFAULT_UNICODE_BTN_RESET_TXT));
				}
			}
		}
	} catch (...) {
		config.~CSimpleIniTempl();
	}
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

			if (_dialogResource.locale != LocalizedPlugin::defaultLangId)
				localize(_hSelf);

			std::wstringstream version;
			wchar_t versionTxt[128]{ L'\0' };
			::GetDlgItemTextW(_hSelf, ID_PLUGIN_VERSION_TXT, versionTxt, 127);
			version << versionTxt << L" " << pluginVersion.str() << L" (" << sizeof(intptr_t) * 8 << L"-bit"
#ifdef _M_ARM
				<< L" ARM"
#endif
				<< L")";
			POINT pt{};
			HWND hwnd = ::GetDlgItem(_hSelf, ID_PLUGIN_VERSION_TXT);
			::MapWindowPoints(hwnd, _hSelf, &pt, 1);
			int offsetX =
#ifdef _M_ARM
			    18;
#else
			    0;
#endif
			if (pluginVersion.build > 0)
				offsetX += 4;
			::SetWindowPos(hwnd, 0, pt.x - offsetX, pt.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
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
			toggleDarkMode(_hSelf, _themeInitialized ? dmfHandleChange : dmfInit);
			_themeInitialized = true;
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
					::DialogBoxParamW(_hInst, MAKEINTRESOURCE(_dialogResource.modal), _hSelf,
					    (DLGPROC)modalDlgProc, reinterpret_cast<LPARAM>(this));
					hideOnReturn = false;
					break;
				case ID_TRANSLATIONS_FILE_LINK:
					plugin.openFile(&plugin.dlgTranslations.wstring()[0]);
					plugin.openFile(&plugin.menuTranslations.wstring()[0]);
					break;
				case ID_ENTITIES_FILE_LINK:
					plugin.openFile(&plugin.entities.wstring()[0]);
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
			if (aboutDlg) {
				aboutDlg->localize(hwndDlg);
				aboutDlg->toggleDarkMode(hwndDlg);
			}

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
