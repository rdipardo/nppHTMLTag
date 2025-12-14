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

	bool menuLocaleIsLatinSlavic() const noexcept {
		return std::find(latinSlavicLangs.begin(), latinSlavicLangs.end(), menuLocale()) !=
		       std::end(latinSlavicLangs);
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

	static inline const auto rtlLangs = { "arabic", "farsi", "hebrew", "kurdish", "urdu", "uyghur" };
	static inline const auto brahmicLangs = { "bengali", "georgian", "gujarati", "hindi", "kannada", "marathi",
		"nepali", "punjabi", "sinhala", "tamil", "telugu", "thai" };
	static inline const auto latinSlavicLangs = { "bosnian", "croatian", "czech", "latvian", "lithuanian", "polish",
		"romanian", "serbian", "slovak", "slovenian", "uzbek" };
	static inline const auto cyrillicLangs = { "abkhazian", "belarusian", "bulgarian", "kazakh", "kyrgyz",
		"macedonian", "mongolian", "russian", "serbianCyrillic", "tajikCyrillic", "tatar", "ukrainian",
		"uzbekCyrillic" };
	static inline const auto cjkLangs = { "chineseSimplified", "japanese", "hongKongCantonese", "korean",
		"taiwaneseMandarin" };
};
#endif // ~LOCALIZED_PLUGIN_H
