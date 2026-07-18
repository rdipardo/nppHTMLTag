/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024,2025 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <windows.h>
#include <shellapi.h>
#include "SimpleIni.h"
#include "VersionInfo.h"
#include "TextConv.h"
#include "HtmlTag.h"
#include "AboutDlg.h"

// Handle static text in default theme mode
#define WM_CTLCOLORSTATIC_LITE WM_CTLCOLORSTATIC

// Handle static text in dark theme mode
#define WM_CTLCOLORSTATIC_DARK WM_DRAWITEM

using namespace HtmlTag;
using namespace TextConv;
using namespace NppDarkMode;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
struct DialogHyperlink {
	int id;
	WNDPROC defWndProc;
};

enum TextDirection { RTL = -1, NONE, LTR };

INT_PTR CALLBACK modalDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK linkCtrlWndProc(HWND hLink, UINT message, WPARAM wParam, LPARAM lParam);

Version pluginVersion;
HFONT hDefaultFont, hActiveLinkFont;
bool hasWin11Dims;

DialogHyperlink linkCtrls[] = {
	{ ID_RELEASE_NOTES_LINK, nullptr },
	{ ID_BUG_TRACKER_LINK, nullptr },
	{ ID_PLUGIN_REPO_LINK, nullptr },
	{ ID_ENTITIES_FILE_LINK, nullptr },
	{ ID_TRANSLATIONS_FILE_LINK, nullptr },
	{ ID_UNICODE_CONFIG_LINK, nullptr },
	{ ID_SIMPLEINI_LINK, nullptr },
};
constexpr size_t nbLinkCtrls = ARRAYSIZE(linkCtrls);
constexpr HWND nullDC = static_cast<HWND>(0ULL);
constexpr unsigned defaultFlags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE;

extern "C" NTSYSAPI VOID NTAPI RtlGetNtVersionNumbers(
    DWORD * /* NtMajorVersion */, DWORD * /* NtMinorVersion */, DWORD * /* NtBuildNumber */);

bool isAtLeastWindows11(DWORD &major, DWORD &build) noexcept {
	constexpr DWORD buildNrMask = ~0xF0000000;
	::RtlGetNtVersionNumbers(&major, nullptr, &build);
	build &= buildNrMask;
	return major > 10 || (major == 10 && build >= 22000);
}
}

// --------------------------------------------------------------------------------------
// AboutDlg
// --------------------------------------------------------------------------------------
AboutDlg::AboutDlg(HINSTANCE hInst, NppData const &data) : StaticDialog() {
	DWORD ntMajorVer = 0, ntBuildNr = 0;
	hasWin11Dims = isAtLeastWindows11(ntMajorVer, ntBuildNr);
	pluginVersion = Version{ HTMLTAG_VERSION_WORDS };
	Window::init(hInst, data._nppHandle);
}
// --------------------------------------------------------------------------------------
void AboutDlg::show() {
	if (!isCreated()) {
		_isRTL = plugin.menuLocaleIsRTL();
		_isCJK = plugin.menuLocaleIsCJK();
		_isBrahmic = plugin.menuLocaleIsBrahmic();
		_isCyrillic = plugin.menuLocaleIsCyrillic();
		_isLatinSlavic = plugin.menuLocaleIsLatinSlavic();
		_isNonLatin = (_isCJK || _isBrahmic || _isCyrillic);
		int dlgRes = ID_ABOUT_HTML_TAG_DLG;
		if (_isRTL)
			dlgRes = ID_ABOUT_HTML_TAG_DLG_RTL;
		else if (_isCJK)
			dlgRes = ID_ABOUT_HTML_TAG_DLG_CJK;
		create(dlgRes);
	}
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
			::SetWindowTextW(hwnd, DEFAULT_CAPTION);
			::SetDlgItemTextW(hwnd, ID_PLUGIN_VERSION_TXT, DEFAULT_VERSION_TXT);
			::SetDlgItemTextW(hwnd, ID_RELEASE_NOTES_LINK, DEFAULT_RELEASE_NOTES_TXT);
			::SetDlgItemTextW(hwnd, ID_BUG_TRACKER_LINK, DEFAULT_BUG_TRACKER_TXT);
			::SetDlgItemTextW(hwnd, ID_PLUGIN_REPO_LINK, DEFAULT_REPO_LINK_TXT);
			::SetDlgItemTextW(hwnd, ID_PLUGIN_LICENSE_TXT, PLUGIN_LICENSE);
			::SetDlgItemTextW(hwnd, ID_SIMPLEINI_TXT, DEFAULT_ABOUT_3RD_PARTY);
			::SetDlgItemTextW(hwnd, ID_ENTITIES_FILE_LINK, DEFAULT_ENTITIES_FILE_TXT);
			::SetDlgItemTextW(hwnd, ID_TRANSLATIONS_FILE_LINK, DEFAULT_L10N_FILE_TXT);
			::SetDlgItemTextW(hwnd, ID_UNICODE_FMT_LABEL_TXT, DEFAULT_UNICODE_FORMAT_LABEL);
			::SetDlgItemTextW(hwnd, ID_UNICODE_CONFIG_LINK, DEFAULT_UNICODE_CONFIG_LABEL);
		} else { // Restore default text to the Unicode format modal dialog
			::SetWindowTextW(hwnd, DEFAULT_UNICODE_EDIT_CAPTION);
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
		std::wstring relNotes, bugs, repo, simpleIni, entities, l10ns, fmtLbl, cfgLbl, cfgDlgLbl;
		TextConv::bytesToText(plugin.menuLocale().c_str(), section, CP_ACP);
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
					relNotes =
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_RELEASE_NOTES_TXT);
					::SetDlgItemTextW(hwnd, ID_RELEASE_NOTES_LINK, relNotes.c_str());
				} else if (sameString(msgId.pItem, L"about_bugs")) {
					bugs = config.GetValue(section.c_str(), msgId.pItem, DEFAULT_BUG_TRACKER_TXT);
					::SetDlgItemTextW(hwnd, ID_BUG_TRACKER_LINK, bugs.c_str());
				} else if (sameString(msgId.pItem, L"about_downloads")) {
					repo = config.GetValue(section.c_str(), msgId.pItem, DEFAULT_REPO_LINK_TXT);
					::SetDlgItemTextW(hwnd, ID_PLUGIN_REPO_LINK, repo.c_str());
				} else if (sameString(msgId.pItem, L"about_license")) {
					::SetDlgItemTextW(hwnd, ID_PLUGIN_LICENSE_TXT,
					    config.GetValue(section.c_str(), msgId.pItem, PLUGIN_LICENSE));
				} else if (sameString(msgId.pItem, L"about_3rd_party")) {
					simpleIni =
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_ABOUT_3RD_PARTY);
					::SetDlgItemTextW(hwnd, ID_SIMPLEINI_TXT, simpleIni.c_str());
				} else if (sameString(msgId.pItem, L"about_entities_file")) {
					entities =
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_ENTITIES_FILE_TXT);
					::SetDlgItemTextW(hwnd, ID_ENTITIES_FILE_LINK, entities.c_str());
				} else if (sameString(msgId.pItem, L"about_l10n_file")) {
					l10ns = config.GetValue(section.c_str(), msgId.pItem, DEFAULT_L10N_FILE_TXT);
					::SetDlgItemTextW(hwnd, ID_TRANSLATIONS_FILE_LINK, l10ns.c_str());
				} else if (sameString(msgId.pItem, L"about_unicode_format")) {
					fmtLbl =
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_UNICODE_FORMAT_LABEL);
					::SetDlgItemTextW(hwnd, ID_UNICODE_FMT_LABEL_TXT, fmtLbl.c_str());
				} else if (sameString(msgId.pItem, L"about_unicode_config")) {
					cfgLbl =
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_UNICODE_CONFIG_LABEL);
					::SetDlgItemTextW(hwnd, ID_UNICODE_CONFIG_LINK, cfgLbl.c_str());
				}
			} else { // Localize the Unicode format modal dialog
				if (sameString(msgId.pItem, L"unicode_dlg_caption")) {
					::SetWindowTextW(hwnd, config.GetValue(section.c_str(), msgId.pItem,
								   DEFAULT_UNICODE_EDIT_CAPTION));
				} else if (sameString(msgId.pItem, L"unicode_dlg_format")) {
					cfgDlgLbl =
					    config.GetValue(section.c_str(), msgId.pItem, DEFAULT_UNICODE_EDIT_LABEL);
					::SetDlgItemTextW(hwnd, ID_CONFIG_LABEL_1, cfgDlgLbl.c_str());
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

		/* Customize the position and alignment of localized text items */
		RECT bounds;
		HDC hHDC = ::GetDC(nullDC);
		::GetWindowRect(hwnd, &bounds);
		if (hwnd == _hSelf) {
			alignText(hwnd, ID_RELEASE_NOTES_LINK, relNotes, hHDC, bounds);
			alignText(hwnd, ID_BUG_TRACKER_LINK, bugs, hHDC, bounds);
			alignText(hwnd, ID_PLUGIN_REPO_LINK, repo, hHDC, bounds);

			HWND dlgItem = ::GetDlgItem(hwnd, ID_PLUGIN_LICENSE_TXT);
			LONG_PTR wstyle = static_cast<LONG_PTR>(::GetWindowLongPtrW(dlgItem, GWL_STYLE));
			::SetWindowLongPtr(dlgItem, GWL_STYLE, wstyle | SS_CENTER);

			alignText(hwnd, ID_SIMPLEINI_TXT, simpleIni, hHDC, bounds);
			alignText(hwnd, ID_ENTITIES_FILE_LINK, entities, hHDC, bounds);
			alignText(hwnd, ID_TRANSLATIONS_FILE_LINK, l10ns, hHDC, bounds);
			alignText(hwnd, ID_UNICODE_FMT_LABEL_TXT, fmtLbl, hHDC, bounds);
			alignText(hwnd, ID_UNICODE_CONFIG_LINK, cfgLbl, hHDC, bounds);
		} else {
			alignText(hwnd, ID_CONFIG_LABEL_1, cfgDlgLbl, hHDC, bounds);
		}
		::ReleaseDC(nullDC, hHDC);
	} catch (...) {
		config.~CSimpleIniTempl();
	}
}
// --------------------------------------------------------------------------------------
void AboutDlg::alignText(HWND hwndDlg, int id, std::wstring const &text, HDC const &hHDC, RECT const &rc) {
	POINT pt{};
	SIZE sz{};
	TextDirection dir = (_isRTL ? TextDirection::RTL : TextDirection::LTR);
	bool isFarsi = plugin.menuLocale() == "farsi";
	bool isHebrew = plugin.menuLocale() == "hebrew";
	bool isHindi = plugin.menuLocale() == "hindi";
	bool isHungarian = plugin.menuLocale() == "hungarian";
	bool isKorean = plugin.menuLocale() == "korean";
	bool isSerbCyrl = plugin.menuLocale() == "serbianCyrillic";
	bool isSinhala = plugin.menuLocale() == "sinhala";
	bool isUkr = plugin.menuLocale() == "ukrainian";

	int textLen = static_cast<int>(std::wcslen(text.c_str()));
	::GetTextExtentPoint32W(hHDC, text.c_str(), textLen, &sz);

	int charWidth = sz.cx / textLen;
	double charSpacing = 1.25;
	if ((hasWin11Dims && isHindi) || _isRTL)
		charWidth *= 2 * dir;
	if (_isBrahmic || (_isCyrillic))
		charSpacing = 1.67;

	HWND item = ::GetDlgItem(hwndDlg, id);
	int scale = ::MapWindowPoints(item, hwndDlg, &pt, 1);
	int offsetY = (scale >> 16) & 0xffff;
	int cx = static_cast<int>(std::ceil(sz.cx * charSpacing));
	int bias = static_cast<int>(std::ceil(charSpacing * charWidth));
	double percentage = 1.0;

	switch (id) {
		case ID_PLUGIN_VERSION_TXT: {
#ifdef _M_ARM
			if (!(_isBrahmic || _isCyrillic))
				charSpacing = 1;
#else
			if (_isBrahmic && hasWin11Dims)
				charSpacing = 1.34;
#endif
			dir = TextDirection::LTR;
			cx = static_cast<int>(std::ceil(sz.cx * charSpacing));

			if (hasWin11Dims) {
				if (_isBrahmic) {
					percentage =
#ifdef _M_ARM
					    0.112;
#else
					    0.167;
#endif
					bias = static_cast<int>(std::ceil(cx * percentage));
				} else if (isSerbCyrl || plugin.menuLocale() == "japanese") {
					/* do nothing */
				} else if (!(plugin.menuLocale() == LocalizedPlugin::defaultLangId || _isNonLatin)) {
					bias = static_cast<int>(std::ceil(cx * 0.0315));
				} else {
					percentage = (isUkr ? 0.05 : 0.0834);
#ifdef _M_ARM
					if (_isCyrillic)
						percentage = 0.05;
#endif
					bias = static_cast<int>(std::floor(cx * percentage));
				}
			} else {
				if (_isBrahmic) {
					percentage = ((isHindi || isSinhala) ? 0.136 : 0.04175);
#ifdef _M_ARM
					if (isHindi || isSinhala)
						percentage = 0.0834;
#endif
					bias = static_cast<int>(std::ceil(cx * percentage));
				} else if (!(plugin.menuLocale() == LocalizedPlugin::defaultLangId || _isNonLatin)) {
					bias = static_cast<int>(std::ceil(cx * (isHebrew ? -0.04691 : -0.10425)));
				} else {
					percentage =
#ifdef _M_ARM
					    _isCyrillic ? (plugin.menuLocale() == "russian" ? 0 : -0.01752) : -0.045625;
#else
					    _isCyrillic ? 0.0209 : -0.0209;
#endif
					bias = static_cast<int>(std::floor(cx * percentage));
				}
			}
			break;
		}
		case ID_RELEASE_NOTES_LINK:
			if (isSerbCyrl || (hasWin11Dims && isFarsi))
				percentage = 0.015625;
			else if (_isBrahmic || _isCyrillic) {
				if (hasWin11Dims) {
					if (isUkr)
						percentage = 0.0417;
					else if (_isBrahmic)
						percentage = isHindi ? 0.03125 : (isSinhala ? 0.05 : 0.07);
					else
						percentage = 0.06255;
				} else
					percentage = 0.07143;
			}
			if (static_cast<int>(percentage) < 1)
				bias += static_cast<int>(std::floor((rc.right - rc.left) * percentage));
			break;
		case ID_BUG_TRACKER_LINK:
			[[fallthrough]];
		case ID_PLUGIN_REPO_LINK:
			if (hasWin11Dims && plugin.menuLocale() == "romanian") {
				percentage = (id == ID_BUG_TRACKER_LINK) ? 0.0208 : 0.0125;
				bias += static_cast<int>(std::floor((rc.right - rc.left) * percentage));
				break;
			}
			[[fallthrough]];
		case ID_ENTITIES_FILE_LINK:
			[[fallthrough]];
		case ID_TRANSLATIONS_FILE_LINK:
			[[fallthrough]];
		case ID_UNICODE_CONFIG_LINK:
			if (id == ID_BUG_TRACKER_LINK && (hasWin11Dims && !(_isNonLatin || _isRTL))) {
				bias += static_cast<int>(std::ceil(cx * 0.0625));
				break;
			} else if (id == ID_PLUGIN_REPO_LINK && _isCyrillic) {
				percentage = (hasWin11Dims ? 0.03125 : 0.05);
				bias += static_cast<int>(std::floor((rc.right - rc.left) * percentage));
				break;
			}

			if (id != ID_BUG_TRACKER_LINK || !isSerbCyrl) {
				if (_isBrahmic) {
					if (isHindi && !hasWin11Dims)
						bias += (rc.right - rc.left) >> 4;
					else if (hasWin11Dims) {
						switch (id) {
							case ID_BUG_TRACKER_LINK:
								if (!isHindi) {
									percentage = isSinhala ? 0.02381 : 0.0417;
									break;
								}
								[[fallthrough]];
							case ID_PLUGIN_REPO_LINK:
								if (!isHindi) {
									percentage = isSinhala ? 0.0417 : 0.0625;
									break;
								}
								[[fallthrough]];
							case ID_ENTITIES_FILE_LINK:
								if (isHindi)
									percentage = 0.02381;
								else if (!isHungarian)
									percentage = isSinhala ? 0.05 : 0.0417;
								break;
							case ID_TRANSLATIONS_FILE_LINK:
								if (!isHindi && !isHungarian) {
									percentage = isSinhala ? 0.05 : 0.0625;
									break;
								}
								[[fallthrough]];
							case ID_UNICODE_CONFIG_LINK:
								percentage = 0.0417;
								break;
						}
					}
				} else if (_isCyrillic) {
					percentage = (hasWin11Dims ? 0.0417 : 0.0625);
					if (isUkr)
						percentage *= 0.7;
				}
			}
			if (static_cast<int>(percentage) < 1)
				bias += static_cast<int>(std::floor((rc.right - rc.left) * percentage));
			break;
		case ID_UNICODE_FMT_LABEL_TXT:
			bias += static_cast<int>(std::ceil((rc.right - rc.left) * (_isBrahmic ? 0.225 : 0.125)));
			if (hasWin11Dims && !_isCyrillic)
				bias -= static_cast<int>(std::floor(bias * 0.33));
			break;
		case ID_CONFIG_LABEL_1: {
			RECT rc2;
			::GetClientRect(::GetDlgItem(hwndDlg, id), &rc2);
			bias += static_cast<int>(std::ceil(((cx >> 1) + rc2.right - rc2.left) * (_isCJK ? 0.4 : 0.7)));
			break;
		}
		case ID_SIMPLEINI_TXT: {
			SIZE sz2;
			int linkId = (id == ID_SIMPLEINI_TXT ? ID_SIMPLEINI_LINK : ID_TINYXML_LINK);
			int textId = (id == ID_SIMPLEINI_TXT ? ID_SIMPLEINI_LICENSE_TXT : ID_TINYXML_LICENSE_TXT);
			HWND hLink = ::GetDlgItem(_hSelf, linkId);
			HWND hText = ::GetDlgItem(_hSelf, textId);
			std::wstring dlgText(0x200, 0);
			::GetDlgItemTextW(_hSelf, linkId, dlgText.data(), static_cast<int>(dlgText.size() - 1ULL));
			int dlgTextLen = static_cast<int>(std::wcslen(dlgText.c_str()));
			::GetTextExtentPoint32W(hHDC, dlgText.c_str(), dlgTextLen, &sz2);

			int fraction = 3;
			if (_isCyrillic)
				fraction = hasWin11Dims ? 2 : 1;
			else if (_isBrahmic)
				fraction = (!hasWin11Dims && isHindi) ? 0 : 1;

			bias += static_cast<int>(std::ceil(1.125 * ((this->getWidth() >> 2) + (sz.cx >> fraction))));
			int startPos = (this->getWidth() >> 1) - (sz.cx >> 1) - bias + charWidth;
			if (_isBrahmic || _isCyrillic)
				startPos += charWidth;

			double displacement =
			    isHungarian ? 0.75 : (_isLatinSlavic ? 0.8667 : (hasWin11Dims ? 0.85 : 0.78));
			if (!(hasWin11Dims || isSerbCyrl) && _isCyrillic)
				displacement = 0.7;
			else if (_isCJK || (hasWin11Dims && _isCyrillic))
				displacement = (hasWin11Dims || isKorean) ? 0.75 : 0.65;
			else if (_isBrahmic || _isCyrillic)
				displacement = (!hasWin11Dims && isHindi) ? 1.0625 : 1;

			if (id == ID_SIMPLEINI_TXT) {
				if (isFarsi)
					displacement = (hasWin11Dims ? 1.34 : 0.85);
				else if (!hasWin11Dims && _isRTL)
					displacement = (isHebrew ? 1.34 : 1.25);
				else if (_isRTL)
					displacement = 1.8;
			}

			int linkStart = (dir * startPos) + static_cast<int>(std::ceil(cx * displacement));
			int linkStartAfter = (hasWin11Dims ? sz2.cx : static_cast<int>(std::ceil(sz2.cx * 0.75)));
			if (!(hasWin11Dims || isSerbCyrl) && _isCyrillic)
				linkStart += (linkStart >> 2);
			int textStart = linkStart + linkStartAfter;
			::SetWindowPos(hLink, HWND_TOP, linkStart, offsetY, sz2.cx, sz2.cy, defaultFlags);
			::SetWindowPos(hText, HWND_TOP, textStart, offsetY, 0, 0, defaultFlags | SWP_NOSIZE);
			break;
		}
		default:
			break;
	}
	int offsetX = ((rc.right - rc.left) >> 1) - (sz.cx >> 1) - (dir * bias);
	if ((id == ID_TRANSLATIONS_FILE_LINK || id == ID_UNICODE_CONFIG_LINK) && isHungarian)
		offsetX -= static_cast<int>(std::ceil(cx * (hasWin11Dims ? 0.03125 : -0.1)));
	else if (!(id == ID_PLUGIN_VERSION_TXT || hasWin11Dims || _isCyrillic || _isBrahmic))
		offsetX += static_cast<int>(std::ceil(cx * 0.125));
	::SetWindowPos(item, HWND_TOP, offsetX, offsetY, cx, sz.cy, defaultFlags);
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

			if (plugin.menuLocale() != LocalizedPlugin::defaultLangId)
				localize(_hSelf);

			std::wstringstream version;
			wchar_t buffer[128]{ L'\0' };
			wchar_t *versionTxt = &buffer[0];
			::GetDlgItemTextW(_hSelf, ID_PLUGIN_VERSION_TXT, versionTxt, 127);
			version << versionTxt << L" " << pluginVersion.str() << L" (" << sizeof(intptr_t) * 8 << L"-bit"
#ifdef _M_ARM
				<< L" ARM"
#endif
				<< L")";
			RECT rc;
			HDC hHDC = ::GetDC(nullDC);
			this->getClientRect(rc);
			::SetDlgItemTextW(_hSelf, ID_PLUGIN_VERSION_TXT, version.str().c_str());
			alignText(_hSelf, ID_PLUGIN_VERSION_TXT, version.str(), hHDC, rc);
			::ReleaseDC(nullDC, hHDC);
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
				::SetDlgItemTextW(_hSelf, ID_UNICODE_USER_FMT_TXT, prefixTxt.c_str());
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
			return reinterpret_cast<INT_PTR>(::GetSysColorBrush(static_cast<int>(::GetBkColor(hdc))));
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
					::TextOutW(lpdi->hDC, lpdi->rcItem.left, lpdi->rcItem.top, &txtBuf[0], txtLen);
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
				case ID_UNICODE_CONFIG_LINK: {
					int dlgRes = ID_UNICODE_FMT_CONFIG_DLG;
					if (_isRTL)
						dlgRes = ID_UNICODE_FMT_CONFIG_DLG_RTL;
					else if (_isCJK)
						dlgRes = ID_UNICODE_FMT_CONFIG_DLG_CJK;
					::DialogBoxParamW(_hInst, MAKEINTRESOURCE(dlgRes), _hSelf,
					    (DLGPROC)modalDlgProc, reinterpret_cast<LPARAM>(this));
					hideOnReturn = false;
					break;
				}
				case ID_TRANSLATIONS_FILE_LINK:
					plugin.openFile(plugin.dlgTranslations);
					plugin.openFile(plugin.menuTranslations);
					break;
				case ID_ENTITIES_FILE_LINK:
					plugin.openFile(plugin.entities);
					break;
				case ID_RELEASE_NOTES_LINK: {
					std::wstring url = RELEASE_NOTES_URL;
					url = url.replace(url.find(L"HEAD"), url.size(),
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
			}

			if (!targetURL.empty())
				::ShellExecuteW(0, L"open", targetURL.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

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
		::SetDlgItemTextW(hwndDlg, ID_CONFIG_EDIT, editTxt.c_str());
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
					::EndDialog(hwndDlg, static_cast<INT_PTR>(wParam));
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
