/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef LOCALIZED_PLUGIN_H
#define LOCALIZED_PLUGIN_H

#include "PluginBase.h"

#ifndef NPPM_GETNATIVELANGFILENAME
#define NPPM_GETNATIVELANGFILENAME (NPPMSG + 116)
#endif

#ifndef NPPN_NATIVELANGCHANGED
#define NPPN_NATIVELANGCHANGED (NPPN_FIRST + 31)
#endif

class LocalizedPlugin : public PluginBase {

public:
	explicit LocalizedPlugin() noexcept : PluginBase(), _nativeLangId(defaultLangId) {}

	std::string const &menuLocale() const noexcept {
		return (_nativeLangId.find("english") != std::string::npos) ? defaultLangId : _nativeLangId;
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
};
#endif // ~LOCALIZED_PLUGIN_H
