/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef LOCALIZED_PLUGIN_H
#define LOCALIZED_PLUGIN_H

#include "PluginBase.h"

class LocalizedPlugin : public PluginBase {

public:
	explicit LocalizedPlugin() noexcept : PluginBase(), _nativeLangId(defaultLangId) {}

	std::string const &menuLocale() const noexcept {
		return (_nativeLangId.find("english") != std::string::npos) ? defaultLangId : _nativeLangId;
	}

	bool menuLocaleIsRTL() const noexcept {
		return std::find(rtlLangs.begin(), rtlLangs.end(), menuLocale()) != std::end(rtlLangs);
	}

	bool menuLocaleIsBrahmic() const noexcept {
		return std::find(brahmicLangs.begin(), brahmicLangs.end(), menuLocale()) != std::end(brahmicLangs);
	}

	bool menuLocaleIsCyrillic() const noexcept {
		return std::find(cyrillicLangs.begin(), cyrillicLangs.end(), menuLocale()) != std::end(cyrillicLangs);
	}

	bool menuLocaleIsCJK() const noexcept {
		return std::find(cjkLangs.begin(), cjkLangs.end(), menuLocale()) != std::end(cjkLangs);
	}

	static inline std::string const &defaultLangId = "default";

protected:
	void setLanguage();
	virtual const wchar_t *getMessage(std::wstring const &) = 0;

private:
	std::string _nativeLangId;
	path_t getNativeLangFile() const;
	/// @brief @c true if N++ is v8.7 or later
	bool supportsLocalizedPluginMenus() const noexcept;

	/// TODO: Add all applicable Notepad++ localization identifiers to these lists
	static inline const auto rtlLangs = { "arabic", "farsi", "hebrew" };
	static inline const auto brahmicLangs = { "hindi", "sinhala", "tamil" };
	static inline const auto cyrillicLangs = { "russian", "serbianCyrillic", "ukrainian" };
	static inline const auto cjkLangs = { "chineseSimplified", "japanese", "korean" };
};
#endif // ~LOCALIZED_PLUGIN_H
