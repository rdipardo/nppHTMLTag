/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef HTMLTAG_ENTITIES_H
#define HTMLTAG_ENTITIES_H

#include <map>
#include "HashedStringList.h"

namespace HtmlTag {
namespace Entities {
	typedef HashedStringList<> EntityList;
	typedef std::map<std::string, EntityList> EntityMap;

	enum EntityReplacementScope { ersSelection, ersDocument, ersAllDocuments };

	constexpr wchar_t scDigits[] = L"0123456789";
	constexpr wchar_t scHexLetters[] = L"ABCDEFabcdef";
	constexpr wchar_t scLetters[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	int decode();
	void encode(EntityReplacementScope scope = ersSelection, bool includeLineBreaks = false);
}
}
#endif // ~HTMLTAG_ENTITIES_H
