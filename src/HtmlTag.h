/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef HTML_TAG_H
#define HTML_TAG_H

#include "Entities.h"
#include "LocalizedPlugin.h"

using namespace HtmlTag::Entities;

namespace HtmlTag {
enum SelectionOptions { soNone = 0x1, soTags = 0x2, soContents = 0x4 };
DEFINE_ENUM_FLAG_OPERATORS(SelectionOptions)

struct PluginOptions {
	BOOL liveEntityDecoding;
	BOOL liveUnicodeDecoding;
	std::string unicodePrefix;
	std::string unicodeRE;
};

struct MenuTitles final : HashedStringList<std::wstring> {
	MenuTitles();
};

class HtmlTagPlugin final : public LocalizedPlugin {

public:
	explicit HtmlTagPlugin() noexcept : LocalizedPlugin() {
		_entityMap = { { "XML", EntityList{} }, { "HTML 5", EntityList{} } };
	}

	void initialize(HMODULE);
	void setInfo(const NppData *) override;
	void beNotified(SCNotification *) override;
	void finalize();
	void getEntities(EntityList &);
	const wchar_t *getMessage(std::wstring const &) override;
	void setUnicodeFormatOption(std::string const &);
	void toggleOption(BOOL *, const int);

	PluginOptions options;
	path_t optionsConfig, entities, translations;
	static constexpr wchar_t pluginMenuName[] = L"&HTML Tag";

private:
	EntityMap _entityMap;
	MenuTitles _menuTitles;
	std::wstring _pluginName, _pluginDLLName;
	void initMenu();
	void updateMenu();
	void loadTranslations();
	void loadOptions();
	void saveOptions();
};

/// Global instance of our plugin object
extern HtmlTagPlugin plugin;
}
#endif // ~HTML_TAG_H
