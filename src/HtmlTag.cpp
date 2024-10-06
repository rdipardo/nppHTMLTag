/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <regex>
#include <fstream>

#define SI_SUPPORT_IOSTREAMS /* CSimpleIniTempl<...>::LoadData(std::istream &) */

#include "SimpleIni.h"
#include "TextConv.h"
#include "TagFinder.h"
#include "Unicode.h"
#include "AboutDlg.h"
#include "HtmlTag.h"

using namespace HtmlTag;
using namespace TextConv;
namespace fs = std::filesystem;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
enum DecodeCmd { dcAuto = -1, dcEntity, dcUnicode };
enum CmdMenuPosition { cmpUnicode = 3, cmpEntities };

bool autoCompleteMatchingTag(const Sci_Position startPos, const char *tagName);
void findAndDecode(const int keyCode, DecodeCmd cmd = dcAuto);

constexpr char defaultUnicodePrefix[] = R"(\u)";
constexpr wchar_t menuItemSeparator[] = L"-";
std::unique_ptr<AboutDlg> aboutHtmlTag = nullptr;
}

#define CMDMENUPROC extern "C" void __cdecl

#ifdef _M_X64
#define CHECKCOMPATIBLE \
	if (!plugin.supportsBigFiles()) \
		return;
#else
#define CHECKCOMPATIBLE
#endif

// --------------------------------------------------------------------------------------
CMDMENUPROC commandFindMatchingTag() {
	CHECKCOMPATIBLE
	TagFinder::findMatchingTag();
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandSelectMatchingTags() {
	CHECKCOMPATIBLE
	TagFinder::findMatchingTag(soTags);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandSelectTagContents() {
	CHECKCOMPATIBLE
	TagFinder::findMatchingTag(soTags | soContents);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandSelectTagContentsOnly() {
	CHECKCOMPATIBLE
	TagFinder::findMatchingTag(soContents);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandEncodeEntities() {
	CHECKCOMPATIBLE
	Entities::encode();
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandEncodeEntitiesInclLineBreaks() {
	CHECKCOMPATIBLE
	Entities::encode(EntityReplacementScope::ersSelection, true);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandDecodeEntities() {
	CHECKCOMPATIBLE
	if (!plugin.editor().activeDocument().currentSelection())
		findAndDecode(0, dcEntity);
	else
		Entities::decode();
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandEncodeJS() {
	CHECKCOMPATIBLE
	Unicode::encode();
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandDecodeJS() {
	CHECKCOMPATIBLE
	if (!plugin.editor().activeDocument().currentSelection())
		findAndDecode(0, dcUnicode);
	else
		Unicode::decode();
}
// --------------------------------------------------------------------------------------
CMDMENUPROC toggleLiveEntityecoding() {
	plugin.toggleOption(&plugin.options.liveEntityDecoding, CmdMenuPosition::cmpEntities);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC toggleLiveUnicodeDecoding() {
	plugin.toggleOption(&plugin.options.liveUnicodeDecoding, CmdMenuPosition::cmpUnicode);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandAbout() {
	aboutHtmlTag->show();
}

// --------------------------------------------------------------------------------------
// HtmlTag::HtmlTagPlugin
// --------------------------------------------------------------------------------------
HtmlTagPlugin HtmlTag::plugin;

void HtmlTagPlugin::initialize(HMODULE hModule) {
	_pluginDLLName = pluginNameFromModule(hModule);
	_pluginName = _pluginDLLName.substr(0, _pluginDLLName.find_last_not_of(L"_unicode") + 1);
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::setInfo(const NppData *data) {
	PluginBase::setInfo(data);
	path_t configPath = pluginsConfigDir() / _pluginName;
	path_t installationPath = pluginsHomeDir() / _pluginDLLName;
	path_t defaultEntities = installationPath / (_pluginName + L"-entities.ini");
	path_t defaulttranslations = installationPath / (_pluginName + L"-translations.ini");
	entities = configPath / L"entities.ini";
	translations = configPath / L"localizations.ini";
	optionsConfig = configPath / L"options.ini";
	std::error_code result;
	if (!fs::exists(configPath))
		fs::create_directory(configPath, result);
	if (result.value() == 0 && !fs::exists(entities))
		::CopyFileW(defaultEntities.c_str(), entities.c_str(), TRUE);
	if (result.value() == 0 && !fs::exists(translations))
		::CopyFileW(defaulttranslations.c_str(), translations.c_str(), TRUE);

	initMenu();
	loadOptions();
	aboutHtmlTag = std::make_unique<AboutDlg>(this->instance(), *data);
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::beNotified(SCNotification *scn) {
	if (scn->nmhdr.hwndFrom == plugin.editor().windowHandle()) {
		switch (scn->nmhdr.code) {
			case NPPN_READY:
#ifdef _M_X64
				if (!plugin.supportsBigFiles()) {
					std::wstringstream caption;
					caption << _pluginName << Version(HTMLTAG_VERSION_WORDS).str() << L" ("
						<< sizeof(intptr_t) * 8 << L"-bit)";
					::MessageBoxW(plugin.editor().windowHandle(), getMessage(L"err_compat"),
					    &caption.str()[0], MB_ICONWARNING);
				}
#endif
				break;
			case NPPN_NATIVELANGCHANGED:
				updateMenu();
				break;
			case NPPN_SHUTDOWN:
				finalize();
				break;
		}
	} else {
		static bool isAutoCompletionCandidate = false;
		switch (scn->nmhdr.code) {
			case SCN_AUTOCSELECTION:
				if (isAutoCompletionCandidate && autoCompleteMatchingTag(scn->position, scn->text))
					plugin.editor().activeDocument().sendMessage(SCI_AUTOCCANCEL);
				break;
			case SCN_AUTOCSELECTIONCHANGE: // https://www.scintilla.org/ScintillaDoc.html#SCN_AUTOCSELECTIONCHANGE
				isAutoCompletionCandidate = (scn->listType == 0);
				break;
			case SCN_USERLISTSELECTION:
				isAutoCompletionCandidate = false;
				break;
			case SCN_CHARADDED:
				if ((scn->characterSource == SC_CHARACTERSOURCE_DIRECT_INPUT) &&
				    !plugin.editor().activeDocument().currentSelection()) {
					findAndDecode(scn->ch);
				}
				break;
		}
	}
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::finalize() {
	saveOptions();
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::getEntities(EntityList &list) {
	const char *listName = documentLangType() == L_XML ? "XML" : "HTML 5";
	auto it = _entityMap.find(listName);
	if (it != _entityMap.end() && it->second) {
		list = it->second;
		return;
	}

	std::wstringstream errMsg;
	path_t iniFile = this->entities;
	errMsg << iniFile.filename() << L" must be saved in folder:\r\n" << iniFile.parent_path().c_str();

	if (!std::filesystem::exists(iniFile)) {
		iniFile = pluginsHomeDir() / _pluginDLLName / (_pluginName + L"-entities.ini");
		errMsg << L"\r\nor " << iniFile.filename() << L" in folder:\r\n" << iniFile.parent_path().c_str();
	}
	if (!std::filesystem::exists(iniFile)) {
		::MessageBoxW(editor().windowHandle(), &errMsg.str()[0], getMessage(L"err_config"), MB_ICONERROR);
		return;
	}

	CSimpleIniCaseA config;
	std::ifstream ifs(iniFile.c_str(), std::ios::in | std::ios::binary);
	std::istream &stream = ifs;

	try {
		SI_Error err = config.LoadData(stream);
		if (err != SI_OK)
			return;

		std::list<CSimpleIniCaseA::Entry> charRefs;
		if (!config.GetAllKeys(listName, charRefs))
			return;

		for (auto &&entity : charRefs) {
			std::string codePointStr = config.GetValue(listName, entity.pItem);
			int codePoint = std::stoi(codePointStr);
			if (codePoint > 0) {
				_entityMap[listName].addPair(entity.pItem, std::to_string(codePoint));
				_entityMap[listName].addPair(std::to_string(codePoint), entity.pItem);
			}
		}
		list = _entityMap[listName];
	} catch (...) {
		config.~CSimpleIniTempl();
	}
	ifs.close();
}
// --------------------------------------------------------------------------------------
const wchar_t *HtmlTagPlugin::getMessage(std::wstring const &key) {
	return _menuTitles[key].c_str();
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::setUnicodeFormatOption(std::string const &userPrefix) {
	if (!userPrefix.empty()) {
		std::string reStr = std::regex_replace(userPrefix, std::regex(R"([\.*+?^${}()[\]|])"), R"(\$&)");
		reStr = std::regex_replace(reStr, std::regex(R"(\\[[:alpha:]])"), R"(\\$&)");
		reStr += R"([0-9A-F]{4,6})";
		options.unicodePrefix = userPrefix;
		options.unicodeRE = reStr;
	} else if (options.unicodePrefix.empty()) {
		setUnicodeFormatOption(defaultUnicodePrefix);
	}
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::toggleOption(BOOL *pOption, const int menuPos) {
	*pOption = !*pOption;
	size_t cmdIdx = funcItems.count() - menuPos;
	sendNppMessage(NPPM_SETMENUITEMCHECK, funcItems.getItemCmdId(cmdIdx), *pOption);
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::initMenu() {
	using sk = ShortcutKey;
	setLanguage();
	if (menuLocale() != LocalizedPlugin::defaultLangId)
		loadTranslations();
	funcItems.add(getMessage(L"menu_0"), commandFindMatchingTag, new sk{ false, true, false, 'T' });
	funcItems.add(getMessage(L"menu_1"), commandSelectMatchingTags, new sk{ false, true, false, 113U });
	funcItems.add(getMessage(L"menu_2"), commandSelectTagContents, new sk{ false, true, true, 'T' });
	funcItems.add(getMessage(L"menu_3"), commandSelectTagContentsOnly, new sk{ true, true, false, 'T' });
	funcItems.add(menuItemSeparator);
	funcItems.add(getMessage(L"menu_4"), commandEncodeEntities, new sk{ true, false, false, 'E' });
	funcItems.add(getMessage(L"menu_5"), commandEncodeEntitiesInclLineBreaks, new sk{ true, true, false, 'E' });
	funcItems.add(getMessage(L"menu_6"), commandDecodeEntities, new sk{ true, false, true, 'E' });
	funcItems.add(menuItemSeparator);
	funcItems.add(getMessage(L"menu_7"), commandEncodeJS, new sk{ false, true, false, 'J' });
	funcItems.add(getMessage(L"menu_8"), commandDecodeJS, new sk{ false, true, true, 'J' });
	funcItems.add(menuItemSeparator);
	funcItems.add(getMessage(L"menu_9"), toggleLiveEntityecoding);
	funcItems.add(getMessage(L"menu_10"), toggleLiveUnicodeDecoding);
	funcItems.add(menuItemSeparator);
	funcItems.add(getMessage(L"menu_11"), commandAbout);
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::updateMenu() {
	if (!funcItems)
		return;

	setLanguage();
	loadTranslations();
	HMENU hMenu = reinterpret_cast<HMENU>(sendNppMessage(NPPM_GETMENUHANDLE, NPPPLUGINMENU, nullptr));

	for (intptr_t i = 0, menuId = 0; i < funcItems.count(); i++, menuId++) {
		if (std::wcscmp(funcItems[i]._itemName, menuItemSeparator) == 0) {
			menuId--;
			continue;
		}

		MENUITEMINFOW mii;
		mii.cbSize = sizeof(MENUITEMINFOW);
		mii.fMask = (MIIM_STRING | MIIM_STATE);
		mii.dwTypeData = nullptr;
		int mId = funcItems[i]._cmdID;

		if (::GetMenuItemInfoW(hMenu, mId, 0, &mii)) {
			std::wstring menubuf(++mii.cch, L'\0');
			mii.dwTypeData = &menubuf[0];
			::GetMenuItemInfoW(hMenu, mId, 0, &mii);
			std::wstring newMenuTitle = getMessage(L"menu_" + std::to_wstring(menuId));
			size_t shortcutPos = menubuf.find_last_of(0x9);
			if (shortcutPos != std::wstring::npos)
				newMenuTitle += menubuf.substr(shortcutPos);
			menubuf = newMenuTitle;
			mii.dwTypeData = &menubuf[0];
			::SetMenuItemInfoW(hMenu, mId, 0, &mii);
		}
	}
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::loadTranslations() {
	if (!fs::exists(translations))
		return;

	CSimpleIniW config{ /* IsUtf8 */ true, /* MultiKey */ false, /* MultiLine */ true };
	config.SetQuotes(true);
	try {
		SI_Error err = config.LoadFile(translations.c_str());
		if (err != SI_OK)
			return;

		std::wstring section(64, L'\0');
		bytesToText(menuLocale().c_str(), section, CP_ACP);
		std::list<CSimpleIniW::Entry> keys;
		if (!config.GetAllKeys(section.c_str(), keys)) { // Unknown language
			_menuTitles = MenuTitles{};
			return;
		}

		MenuTitles defaultMsgs = MenuTitles{};
		for (auto &&msgId : keys) {
			const wchar_t *defMsg = defaultMsgs[{ msgId.pItem }].c_str();
			_menuTitles.addPair(msgId.pItem, config.GetValue(section.c_str(), msgId.pItem, defMsg));
		}
	} catch (...) {
		config.~CSimpleIniTempl();
	}
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::loadOptions() {
	if (fs::exists(optionsConfig)) {
		CSimpleIniA config;
		std::ifstream ifs(optionsConfig.c_str(), std::ios::in | std::ios::binary);
		std::istream &stream = ifs;
		try {
			SI_Error err = config.LoadData(stream);
			if (err != SI_OK)
				return;
			options.liveEntityDecoding = config.GetBoolValue("AUTO_DECODE", "ENTITIES", false);
			options.liveUnicodeDecoding = config.GetBoolValue("AUTO_DECODE", "UNICODE_ESCAPE_CHARS", false);
			std::string userPrefix =
			    config.GetValue("FORMAT", "UNICODE_ESCAPE_PREFIX", defaultUnicodePrefix);
			setUnicodeFormatOption(userPrefix);
		} catch (...) {
			config.~CSimpleIniTempl();
		}
		ifs.close();
	} else {
		setUnicodeFormatOption(defaultUnicodePrefix);
	}

	size_t autoDecodeJs = funcItems.count() - CmdMenuPosition::cmpUnicode;
	size_t autoDecodeEntities = funcItems.count() - CmdMenuPosition::cmpEntities;
	funcItems[autoDecodeJs]._init2Check = options.liveUnicodeDecoding;
	funcItems[autoDecodeEntities]._init2Check = options.liveEntityDecoding;
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::saveOptions() {
	if (!fs::exists(optionsConfig.parent_path()))
		return;

	CSimpleIniA config;
	config.SetSpaces(false);
	std::ofstream ofs(optionsConfig.c_str(), std::ios::out | std::ios::binary);
	try {
		config.SetLongValue("AUTO_DECODE", "ENTITIES", options.liveEntityDecoding);
		config.SetLongValue("AUTO_DECODE", "UNICODE_ESCAPE_CHARS", options.liveUnicodeDecoding);
		config.SetValue("FORMAT", "UNICODE_ESCAPE_PREFIX", options.unicodePrefix.c_str());
		config.Save(ofs);
	} catch (...) {
		config.~CSimpleIniTempl();
	}
	ofs.close();
}

// --------------------------------------------------------------------------------------
// HtmlTag::MenuTitles
// --------------------------------------------------------------------------------------
MenuTitles::MenuTitles() : HashedStringList<std::wstring>() {
	std::initializer_list<std::wstring> defaultMenuTitles = {
		L"menu_0=&Find matching tag",
		L"menu_1=Select &matching tags",
		L"menu_2=&Select tag and contents",
		L"menu_3=Select tag &contents only",
		L"menu_4=&Encode entities",
		L"menu_5=Encode entities (incl. line &breaks)",
		L"menu_6=&Decode entities",
		L"menu_7=Encode &Unicode characters",
		L"menu_8=Dec&ode Unicode characters",
		L"menu_9=Automatically decode entities",
		L"menu_10=Automatically decode Unicode characters",
		L"menu_11=&About...",
		L"err_compat=The installed version of HTML Tag requires Notepad++ 8.3 or newer. Plugin commands have "
		L"been disabled.",
		L"err_config=Missing Entities File",
	};
	addStrings(defaultMenuTitles);
};

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
bool autoCompleteMatchingTag(const Sci_Position startPos, const char *tagName) {
	const size_t maxTagLength = 72; // https://www.rfc-editor.org/rfc/rfc1866#section-3.2.3
	const auto webLangs = { L_HTML, L_XML, L_PHP, L_ASP, L_JSP };
	std::wstring newTag;
	bytesToText(tagName, newTag, CP_ACP);

	if (std::find(webLangs.begin(), webLangs.end(), plugin.documentLangType()) != std::end(webLangs) ||
	    plugin.editor().activeDocument().getSelectionMode() != smStreamMulti || newTag.length() > maxTagLength) {
		return false;
	}

	SciActiveDocument doc = plugin.editor().activeDocument();
	SciTextRange tagEnd{ doc };
	doc.find(LR"([/>\s])", tagEnd, SCFIND_REGEXP, startPos, startPos + maxTagLength + 1);

	if (tagEnd.length() != 0) {
		commandSelectMatchingTags();
		doc.currentSelection() = newTag;
		return true;
	}

	return false;
}
// --------------------------------------------------------------------------------------
void findAndDecode(const int keyCode, DecodeCmd cmd) {
	using Decoder = int (*)();
	int ch = keyCode & 0xff;

	if ((cmd == dcAuto) && (!(plugin.options.liveEntityDecoding || plugin.options.liveUnicodeDecoding) ||
				   !((ch >= 0x09 && ch <= 0x0D) || ch == 0x20))) {
		return;
	}

	SciActiveDocument doc = plugin.editor().activeDocument();
	Sci_Position caret = doc.currentPosition(), charOffset = -1, anchor, selStart, nextCaretPos;
	bool didReplace = false;

	auto replace = [doc](Decoder decoder, Sci_Position start, Sci_Position end) {
		int nDecoded = 0;
		doc.select(start, end - start);
		nDecoded = decoder();
		return (nDecoded > 0);
	};

	if (cmd == dcAuto)
		caret = doc.sendMessage(SCI_POSITIONBEFORE, doc.currentPosition());

	for (anchor = caret - 1; anchor >= 0; anchor--) {
		int chCurrent = static_cast<int>(doc.sendMessage(SCI_GETCHARAT, anchor));
		if (chCurrent >= 0 && chCurrent <= 0x20)
			break;
		if (chCurrent == '&' && (plugin.options.liveEntityDecoding || cmd == dcEntity)) { // Handle entities
			didReplace = replace(Entities::decode, anchor, caret);
			if (!(ch == 0x0A || ch == 0x0D))
				++charOffset;
			break;
		} else if (chCurrent == plugin.options.unicodePrefix[0] &&
			   (plugin.options.liveUnicodeDecoding || cmd == dcUnicode)) { // Handle Unicode
			size_t lenPrefix = plugin.options.unicodePrefix.size();
			size_t lenCodePt = 4 + lenPrefix;
			selStart = anchor;
			// Backtrack to previous codepoint, in case it's part of a surrogate pair
			chCurrent = static_cast<int>(doc.sendMessage(SCI_GETCHARAT, anchor - lenCodePt));
			if (chCurrent == plugin.options.unicodePrefix[0]) {
				doc.select(anchor - lenCodePt, lenCodePt);
				int chValue =
				    std::stoi(doc.currentSelection().text().substr(lenPrefix, 4), nullptr, 16);
				if (chValue >= 0xD800 && chValue <= 0xDBFF) {
					selStart -= lenCodePt;
				}
			}
			didReplace = replace(Unicode::decode, selStart, caret);
			for (size_t i = 0; i < lenPrefix; i++) // Compensate for prefix length
				++charOffset;
			break;
		}
	}

	if (didReplace) {
		if (ch == 0x0A || ch == 0x0D) { // ENTER was pressed
			doc.currentPosition(doc.nextLineStartPosition());
		} else {
			nextCaretPos = doc.sendMessage(SCI_POSITIONAFTER, doc.currentPosition());
			if (nextCaretPos >= doc.nextLineStartPosition()) // Stay in current line if at EOL
				return;

			if (cmd > dcAuto) // No inserted char, nothing to offset
				charOffset = -1;

			doc.currentPosition(nextCaretPos + charOffset);
		}
	} else {
		if (cmd == dcAuto) { // Place caret after inserted char
			++caret;
			if (ch == 0x0A && doc.sendMessage(SCI_GETEOLMODE) == SC_EOL_CRLF)
				++caret;
		}
		doc.currentSelection().clearSelection();
		doc.currentPosition(caret);
	}
}
}
