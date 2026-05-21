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
#include "XPM.h"

using namespace HtmlTag;
using namespace HtmlTag::Entities;
using namespace SciTextObjects;
using namespace TextConv;
namespace fs = std::filesystem;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
enum DecodeCmd { dcAuto = -1, dcEntity, dcUnicode };
enum CmdMenuPosition { cmpAcEntities = 3, cmpUnicode, cmpEntities };

bool isWebDocument() noexcept;
bool autoCompleteMatchingTag(const Sci_Position startPos, const char *tagName);
void autoCompleteEntity(bool preferEmoji);
void findAndDecode(const int keyCode, DecodeCmd cmd = dcAuto);

constexpr bool isStartOfEntity(const int &ch) noexcept {
	return ch == Entities::amp || ch == Entities::colon;
}
constexpr bool isEndOfEntity(const int &ch) noexcept {
	return ch == Entities::semi || ch == Entities::colon;
}
constexpr char acListKey[] = "_autocompletions";
constexpr char defaultUnicodePrefix[] = R"(\u)";
constexpr wchar_t menuItemSeparator[] = L"-";
constexpr wchar_t errorMessageDelimiter = L'|';
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
	if (!plugin.activeDocument().currentSelection())
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
	if (!plugin.activeDocument().currentSelection())
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
CMDMENUPROC toggleEntityAutoCompletion() {
	plugin.toggleOption(&plugin.options.entityAutoCompletion, CmdMenuPosition::cmpAcEntities);
}
// --------------------------------------------------------------------------------------
CMDMENUPROC commandAbout() {
	if (!aboutHtmlTag)
		aboutHtmlTag = std::make_unique<AboutDlg>(plugin.instance(), plugin.npp());
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
	path_t defaultMenuTranslations = installationPath / (_pluginName + L"-localizations.ini");
	path_t defaultDlgTranslations = installationPath / (_pluginName + L"-dialogs.ini");
	entities = configPath / L"entities.ini";
	menuTranslations = configPath / L"localizations.ini";
	dlgTranslations = configPath / L"dialogs.ini";
	optionsConfig = configPath / L"options.ini";
	std::error_code result;
	if (!fs::exists(configPath))
		fs::create_directory(configPath, result);
	if (result.value() == 0) {
		loadOptions();
		VersionChecksums checksums(installationPath / (_pluginName + L".md5sums"));
		for (path_t const &cfg : { entities, menuTranslations, dlgTranslations }) {
			if (!fs::exists(cfg))
				continue;
			std::string const &key = cfg.stem().string();
			std::string const &hash = checksums[key];
			if (options.currency[key] != hash) {
				options.currency.addPair(key, hash);
				std::wstringstream newName;
				auto const &mtime =
				    static_cast<int64_t>(fs::last_write_time(cfg, result).time_since_epoch().count());
				newName << cfg.stem().wstring() << L"-" << std::to_wstring(mtime) << L".ini";
				path_t const &backup = configPath / newName.view();
				unsigned long mvFlags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
				::MoveFileExW(cfg.c_str(), backup.c_str(), mvFlags);
				::SetLastError(0);
			}
		}
		::CopyFileW(defaultEntities.c_str(), entities.c_str(), TRUE);
		::CopyFileW(defaultMenuTranslations.c_str(), menuTranslations.c_str(), TRUE);
		::CopyFileW(defaultDlgTranslations.c_str(), dlgTranslations.c_str(), TRUE);
	}
	initMenu();
	size_t autoCompleteEntities = funcItems.count() - CmdMenuPosition::cmpAcEntities;
	size_t autoDecodeJs = funcItems.count() - CmdMenuPosition::cmpUnicode;
	size_t autoDecodeEntities = funcItems.count() - CmdMenuPosition::cmpEntities;
	funcItems[autoCompleteEntities]._init2Check = options.entityAutoCompletion;
	funcItems[autoDecodeJs]._init2Check = options.liveUnicodeDecoding;
	funcItems[autoDecodeEntities]._init2Check = options.liveEntityDecoding;
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::beNotified(SCNotification *scn) {
	static bool isWebDoc = false;
	if (scn->nmhdr.hwndFrom == plugin.editor().windowHandle()) {
		switch (scn->nmhdr.code) {
			case NPPN_READY:
#ifdef _M_X64
				if (!plugin.supportsBigFiles()) {
					std::wstringstream caption;
					unsigned long mbMask = MB_ICONERROR;
					if (menuLocaleIsRTL())
						mbMask |= MB_RTLREADING;
					caption << _pluginName << Version(HTMLTAG_VERSION_WORDS).str() << L" ("
						<< sizeof(intptr_t) * 8 << L"-bit)";
					::MessageBoxW(plugin.editor().windowHandle(), getMessage(L"err_compat"),
					    &caption.str()[0], mbMask);
				}
#endif
				break;
			case NPPN_BUFFERACTIVATED:
				isWebDoc = ::isWebDocument();
				break;
			case NPPN_FILESAVED: {
				path_t const &filePath = currentBufferPath(scn->nmhdr.idFrom);
				if (sameText(filePath, this->menuTranslations))
					updateMenu();
				else if (sameText(filePath, this->dlgTranslations))
					aboutHtmlTag.reset();
				else if (sameText(filePath, this->entities))
					_entityMap.clear();
				break;
			}
			case NPPN_NATIVELANGCHANGED:
				updateMenu();
				aboutHtmlTag.reset();
				break;
			case NPPN_SHUTDOWN:
				finalize();
				break;
		}
	} else {
		static bool isAutoCompletionCandidate = false;
		static intptr_t acInsertMode = SC_MULTIAUTOC_ONCE;
		switch (scn->nmhdr.code) {
			case SCN_AUTOCSELECTION:
				acInsertMode = activeDocument().sendMessage(SCI_AUTOCGETMULTI);
				if (isWebDoc && isAutoCompletionCandidate &&
				    autoCompleteMatchingTag(scn->position, scn->text)) {
					SciViewList views = editor().getViews();
					for (size_t i = 0; i < views.size; ++i)
						views[i].sendMessage(SCI_AUTOCSETMULTI, SC_MULTIAUTOC_EACH);
				}
				break;
			case SCN_AUTOCCOMPLETED: {
				SciViewList views = editor().getViews();
				for (size_t i = 0; i < views.size; ++i)
					views[i].sendMessage(SCI_AUTOCSETMULTI, acInsertMode);
				break;
			}
			case SCN_AUTOCSELECTIONCHANGE: // https://www.scintilla.org/ScintillaDoc.html#SCN_AUTOCSELECTIONCHANGE
				isAutoCompletionCandidate = (scn->listType == 0);
				break;
			case SCN_USERLISTSELECTION:
				isAutoCompletionCandidate = false;
				break;
			case SCN_AUTOCCHARDELETED:
				if (options.entityAutoCompletion && isWebDoc) {
					SciActiveDocument doc = activeDocument();
					Sci_Position pos = doc.currentPosition();
					int ch = static_cast<int>(doc.sendMessage(SCI_GETCHARAT, pos - 1));
					if (isStartOfEntity(ch)) {
						autoCompleteEntity(ch == Entities::colon);
					}
				}
				break;
			case SCN_CHARADDED:
				if ((scn->characterSource == SC_CHARACTERSOURCE_DIRECT_INPUT) &&
				    !activeDocument().currentSelection()) {
					findAndDecode(scn->ch);
				}
				if (options.entityAutoCompletion && isStartOfEntity(scn->ch) && isWebDoc) {
					autoCompleteEntity(scn->ch == Entities::colon);
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
void HtmlTagPlugin::getEntities(EntityList &list, bool preferEmoji) {
	const char *listName = documentLangType() == L_XML ? "XML" : (preferEmoji ? "Emoji" : "HTML 5");
	auto it = _entityMap.find(listName);
	if (it != _entityMap.end() && it->second) {
		list = it->second;
		return;
	}

	path_t iniFile = this->entities;

	if (!std::filesystem::exists(iniFile)) {
		size_t d1 = 0, d2 = 0;
		std::wstring msgText, msgs[3]{};
		std::wstringstream errMsg;
		msgText = getMessage(L"err_config_msg");
		d1 = msgText.find_first_of(errorMessageDelimiter);
		d2 = msgText.find_last_of(errorMessageDelimiter);
		msgs[0] = (d1 == std::wstring::npos) ? L"must be saved in folder" : msgText.substr(0, d1);
		errMsg << iniFile.filename() << L" " << msgs[0] << L":\r\n" << iniFile.parent_path().c_str();
		iniFile = pluginsHomeDir() / _pluginDLLName / (_pluginName + L"-entities.ini");
		if (!std::filesystem::exists(iniFile)) {
			unsigned long mbMask = MB_ICONERROR;
			if (menuLocaleIsRTL())
				mbMask |= MB_RTLREADING;
			msgs[1] = (d2 == std::wstring::npos || d1 >= d2) ? L"or" : msgText.substr(d1 + 1, d2 - d1 - 1);
			msgs[2] = (d2 == std::wstring::npos) ? L"in folder" : msgText.substr(d2 + 1);
			errMsg << L"\r\n"
			       << msgs[1] << L" " << iniFile.filename() << L" " << msgs[2] << L":\r\n"
			       << iniFile.parent_path().c_str();
			::MessageBoxW(editor().windowHandle(), &errMsg.str()[0], getMessage(L"err_config"), mbMask);
			return;
		}
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

		std::stringstream acListBuf;
		for (auto &&entity : charRefs) {
			std::string codePointStr = config.GetValue(listName, entity.pItem);
			int codePoint = std::stoi(codePointStr);
			if (codePoint > 0) {
				_entityMap[listName].addPair(entity.pItem, std::to_string(codePoint));
				acListBuf << entity.pItem;
				if (!preferEmoji) {
					_entityMap[listName].addPair(std::to_string(codePoint), entity.pItem);
					acListBuf << Entities::semi << '?' << XPM::getID();
				} else {
					acListBuf << Entities::colon << '?' << XPM::getGitHubID();
				}
				acListBuf << ' ';
			}
		}
		_entityMap[listName].addPair(&acListKey[0], acListBuf.str());
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
		reStr += R"(((00[7-9A-F][0-9A-F])|[0-9A-F]{6}|[1-9A-F]?[0-9A-F]{4}))";
		options.unicodePrefix = userPrefix;
		options.unicodeRE = reStr;
	} else if (options.unicodePrefix.empty()) {
		setUnicodeFormatOption(&defaultUnicodePrefix[0]);
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
	using pSk = std::shared_ptr<sk>;
	setLanguage();
	if (menuLocale() != LocalizedPlugin::defaultLangId)
		loadTranslations();
	funcItems.add(getMessage(L"menu_0"), commandFindMatchingTag, pSk(new sk{ false, true, false, 'T' }));
	funcItems.add(getMessage(L"menu_1"), commandSelectMatchingTags, pSk(new sk{ false, true, false, 113U }));
	funcItems.add(getMessage(L"menu_2"), commandSelectTagContents, pSk(new sk{ false, true, true, 'T' }));
	funcItems.add(getMessage(L"menu_3"), commandSelectTagContentsOnly, pSk(new sk{ true, true, false, 'T' }));
	funcItems.add(&menuItemSeparator[0]);
	funcItems.add(getMessage(L"menu_4"), commandEncodeEntities, pSk(new sk{ true, false, false, 'E' }));
	funcItems.add(
	    getMessage(L"menu_5"), commandEncodeEntitiesInclLineBreaks, pSk(new sk{ true, true, false, 'E' }));
	funcItems.add(getMessage(L"menu_6"), commandDecodeEntities, pSk(new sk{ true, false, true, 'E' }));
	funcItems.add(&menuItemSeparator[0]);
	funcItems.add(getMessage(L"menu_7"), commandEncodeJS, pSk(new sk{ false, true, false, 'J' }));
	funcItems.add(getMessage(L"menu_8"), commandDecodeJS, pSk(new sk{ false, true, true, 'J' }));
	funcItems.add(&menuItemSeparator[0]);
	funcItems.add(getMessage(L"menu_9"), toggleLiveEntityecoding);
	funcItems.add(getMessage(L"menu_10"), toggleLiveUnicodeDecoding);
	funcItems.add(getMessage(L"menu_12"), toggleEntityAutoCompletion);
	funcItems.add(&menuItemSeparator[0]);
	funcItems.add(getMessage(L"menu_11"), commandAbout);
}
// --------------------------------------------------------------------------------------
void HtmlTagPlugin::updateMenu() {
	if (!funcItems)
		return;

	setLanguage();
	loadTranslations();

	constexpr int formerMaxIndex = 11; //< Index of the "About..." menu title before v1.5.2
	HMENU hMenu = reinterpret_cast<HMENU>(sendNppMessage(NPPM_GETMENUHANDLE, NPPPLUGINMENU, nullptr));

	for (intptr_t i = 0, menuId = 0; i < funcItems.count(); i++, menuId++) {
		if (std::wcscmp(&(funcItems[i]._itemName)[0], &menuItemSeparator[0]) == 0) {
			menuId--;
			continue;
		} else if (menuId == formerMaxIndex) {
			std::wstring titleOld = L"menu_" + std::to_wstring(formerMaxIndex);
			std::wstring titleCurrent = L"menu_" + std::to_wstring(formerMaxIndex + 1);
			std::wstring tmp(_menuTitles[titleCurrent]);
			_menuTitles.addPair(titleCurrent, _menuTitles[titleOld]);
			_menuTitles.addPair(titleOld, tmp);
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
	if (!fs::exists(menuTranslations))
		return;

	CSimpleIniW config{ /* IsUtf8 */ true, /* MultiKey */ false, /* MultiLine */ true };
	config.SetQuotes(true);
	try {
		SI_Error err = config.LoadFile(menuTranslations.c_str());
		if (err != SI_OK)
			return;

		std::wstring section(64, L'\0');
		bytesToText(menuLocale().c_str(), section, CP_ACP);
		std::list<CSimpleIniW::Entry> keys;
		if (!config.GetAllKeys(section.c_str(), keys)) { // Unknown language
			_menuTitles = MenuTitles{};
			return;
		}

		MenuTitles defaultMsgs{};
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
			options.entityAutoCompletion = config.GetBoolValue("AUTO_COMPLETE", "ENTITIES", true);
			std::string userPrefix =
			    config.GetValue("FORMAT", "UNICODE_ESCAPE_PREFIX", &defaultUnicodePrefix[0]);
			setUnicodeFormatOption(userPrefix);
			for (auto &&[key, fname] : options.currency.keys)
				options.currency.addPair(fname, config.GetValue("VERSION_HASHES", key, ""));
		} catch (...) {
			config.~CSimpleIniTempl();
		}
		ifs.close();
	} else {
		setUnicodeFormatOption(&defaultUnicodePrefix[0]);
	}
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
		config.SetLongValue("AUTO_COMPLETE", "ENTITIES", options.entityAutoCompletion);
		config.SetValue("FORMAT", "UNICODE_ESCAPE_PREFIX", options.unicodePrefix.c_str());
		for (auto &&[key, fname] : options.currency.keys)
			config.SetValue("VERSION_HASHES", key, options.currency[{ fname }].c_str());
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
		// clang-format off
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
		L"menu_12=Auto-complete HTML entities",
		L"err_compat=The installed version of HTML Tag requires Notepad++ 8.3 or newer. Plugin commands have been disabled.",
		L"err_config=Missing Entities File",
		// clang-format on
	};
	addStrings(defaultMenuTitles);
};

// --------------------------------------------------------------------------------------
// HtmlTag::VersionChecksums
// --------------------------------------------------------------------------------------
VersionChecksums::VersionChecksums(path_t const &hashfile) : HashedStringList<std::string>() {
	nameValueSeparator = "\x20";
	std::ifstream ifs(hashfile, std::ios::in | std::ios::binary);
	if (ifs.is_open()) {
		std::string hash;
		while (std::getline(ifs, hash)) {
			size_t delim = hash.find_first_of(this->nameValueSeparator);
			size_t keyEnd = hash.find_first_of('.', delim);
			std::string const &key =
			    hash.substr(delim + 1, keyEnd != hash.npos ? --keyEnd - delim : keyEnd);
			std::string const &checksum = hash.substr(0, delim);
			size_t keyBegin = key.find_first_of('-');
			this->addPair(key.substr(keyBegin == key.npos ? 0 : keyBegin + 1), checksum);
		}
		ifs.close();
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
bool isWebDocument() noexcept {
	const auto webLangs = { L_HTML, L_XML, L_PHP, L_ASP, L_JSP };
	if (std::find(webLangs.begin(), webLangs.end(), plugin.documentLangType()) != std::end(webLangs))
		return true;

	const auto ftypes = {
		L".adoc",
		L".asciidoc",
		L".creole",
		L".markdown",
		L".md",
		L".mdoc",
		L".mdown",
		L".mdtext",
		L".mdtxt",
		L".mdwn",
		L".mediawiki",
		L".mkd",
		L".mkdn",
		L".org",
		L".pod",
		L".rdoc",
		L".rst",
		L".textile",
		L".wiki",
	};

	std::wstring docExt;
	wchar_t extBuf[MAX_PATH]{ L'\0' };
	if (!plugin.sendNppMessage(NPPM_GETEXTPART, MAX_PATH - 1, &extBuf[0]))
		return false;

	docExt.assign(&extBuf[0], std::wcslen(&extBuf[0]));

	return (std::find_if(ftypes.begin(), ftypes.end(),
		   [&docExt](const std::wstring &&ext) { return sameText(docExt, ext); })) != std::end(ftypes);
}
// --------------------------------------------------------------------------------------
void autoCompleteEntity(bool preferEmoji) {
	EntityList entities;
	plugin.getEntities(entities, preferEmoji);
	SciActiveDocument const &doc = plugin.activeDocument();
	int xpmId = preferEmoji ? XPM::getGitHubID() : XPM::getID();
	std::stringstream delimBuf;
	delimBuf << static_cast<char>(doc.sendMessage(SCI_AUTOCGETTYPESEPARATOR));
	delimBuf << xpmId;
	delimBuf << static_cast<char>(doc.sendMessage(SCI_AUTOCGETSEPARATOR));
	std::string acList = entities[{ &acListKey[0] }];
	if (acList.find(delimBuf.str()) == std::string::npos) {
		acList = std::regex_replace(acList, std::regex(R"(\?\d+ )"), delimBuf.str());
		entities.addPair(&acListKey[0], acList);
	}
	const LPARAM xmpData = reinterpret_cast<LPARAM>(preferEmoji ? XPM::getGitHubData() : XPM::getData());
	doc.sendMessage(SCI_REGISTERIMAGE, xpmId, xmpData);
	doc.sendMessage(SCI_AUTOCSHOW, UNUSEDW, &acList[0]);
}
// --------------------------------------------------------------------------------------
bool autoCompleteMatchingTag(const Sci_Position startPos, const char *tagName) {
	constexpr size_t maxTagLength = 72; // https://www.rfc-editor.org/rfc/rfc1866#section-3.2.3
	SciActiveDocument const &doc = plugin.activeDocument();

	if (doc.getSelectionMode() != smStreamMulti || strlen(tagName) > maxTagLength) {
		return false;
	}

	SciTextRange tagEnd{ doc };
	doc.find(LR"([/>\s])", tagEnd, SCFIND_REGEXP, startPos, startPos + maxTagLength + 1);

	return (tagEnd.length() != 0);
}
// --------------------------------------------------------------------------------------
void findAndDecode(const int keyCode, DecodeCmd cmd) {
	using Decoder = int (*)();
	SciActiveDocument const &doc = plugin.activeDocument();
	int ch = keyCode & 0xff;

	if ((cmd == dcAuto) && ((ch == 0x0D && doc.sendMessage(SCI_GETEOLMODE) == SC_EOL_CRLF) ||
				   !(plugin.options.liveEntityDecoding || plugin.options.liveUnicodeDecoding) ||
				   !((ch >= 0x09 && ch <= 0x0D) || ch == 0x20))) {
		return;
	}

	Sci_Position caret = doc.currentPosition(), charOffset = -1, anchor = 0, selStart = 0, nextCaretPos = 0;
	bool didReplace = false;
	bool skipEntities = false;

	auto replace = [doc](Decoder decoder, Sci_Position start, Sci_Position end) {
		int nDecoded = 0;
		doc.select(start, end - start);
		nDecoded = decoder();
		return (nDecoded > 0);
	};

	if (cmd == dcAuto)
		caret = doc.sendMessage(SCI_POSITIONBEFORE, doc.currentPosition());

	Sci_Position startPos = caret - 1;
	for (anchor = startPos; anchor >= 0; anchor--) {
		int chCurrent = static_cast<int>(doc.sendMessage(SCI_GETCHARAT, anchor));
		if (chCurrent >= 0 && chCurrent <= 0x20)
			break;
		if (plugin.options.liveEntityDecoding || cmd == dcEntity) {
			if (anchor == startPos)
				skipEntities =
				    // No adjacent entity here
				    !isEndOfEntity(chCurrent) ||
				    // Do nothing if current text is `:;`, `;:`, `;;` or `::`
				    isEndOfEntity(static_cast<int>(doc.sendMessage(SCI_GETCHARAT, anchor - 1)));
			else if (anchor < startPos && !skipEntities && isStartOfEntity(chCurrent)) { // Handle entities
				didReplace = replace(Entities::decode, anchor, caret);
				if (!(ch == 0x0A || ch == 0x0D))
					++charOffset;
				break;
			}
		}
		if (chCurrent == plugin.options.unicodePrefix[0] &&
		    (plugin.options.liveUnicodeDecoding || cmd == dcUnicode)) { // Handle Unicode
			Sci_Position lenPrefix = static_cast<Sci_Position>(plugin.options.unicodePrefix.size());
			Sci_Position lenCodePt = 4 + lenPrefix;
			selStart = anchor;
			// Backtrack to previous codepoint, in case it's part of a surrogate pair
			chCurrent = static_cast<int>(doc.sendMessage(SCI_GETCHARAT, anchor - lenCodePt));
			if (chCurrent == plugin.options.unicodePrefix[0]) {
				doc.select(anchor - lenCodePt, lenCodePt);
				const std::wstring &str = doc.currentSelection().text().substr(lenPrefix, 4);
				if (std::all_of(str.begin(), str.end(), [](wchar_t ch) { return std::isxdigit(ch); })) {
					int chValue = std::stoi(str, nullptr, 16);
					if (chValue >= 0xD800 && chValue <= 0xDBFF)
						selStart -= lenCodePt;
				}
			}
			didReplace = replace(Unicode::decode, selStart, caret);
			for (Sci_Position i = 1; i < lenPrefix; ++i) // Compensate for prefix length
				++charOffset;
			break;
		}
	}

	if (didReplace) {
		if (ch == 0x0A || ch == 0x0D) { // ENTER was pressed
			doc.currentPosition(doc.nextLineStartPosition());
		} else {
			nextCaretPos = doc.sendMessage(SCI_POSITIONAFTER, doc.currentPosition());
			if (nextCaretPos >= doc.nextLineStartPosition()) { // Stay in current line if at EOL
				if (cmd == dcAuto) // ...but also stay ahead of the inserted char
					doc.currentPosition(caret);
				return;
			}
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
