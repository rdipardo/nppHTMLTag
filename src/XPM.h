/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
namespace HtmlTag {
namespace Entities {
	namespace XPM {
		constexpr int decorationID = 0x7f;
		constexpr int decorationDarkID = decorationID << 0x1;
		constexpr const char *decoration[] = {
			/* columns rows colors chars-per-pixel */
			"16 16 2 1 ",
			"  c #262626",
			"z c None",
			/* pixels */
			"zzzzzzzzzzzzzzzz",
			"zzzzzz z zzzzzzz",
			"zzzzz      zzzzz",
			"zzzz   z   zzzzz",
			"zzzz   zz  zzzzz",
			"zzzz   z   zzzzz",
			"zzzzz     zzzzzz",
			"zzzzz    zzzzzzz",
			"zzz       z  zzz",
			"zzz  zz      zzz",
			"zz   zzz     zzz",
			"zz   zzzz    zzz",
			"zzz   zz      zz",
			"zzzz      zz  zz",
			"zzzzzzzzzzzz zzz",
			"zzzzzzzzzzzzzzzz",
		};
		constexpr const char *decorationDark[] = {
			/* columns rows colors chars-per-pixel */
			"16 16 2 1 ",
			"  c #A0A0A0",
			"z c None",
			/* pixels */
			"zzzzzzzzzzzzzzzz",
			"zzzzzz z zzzzzzz",
			"zzzzz      zzzzz",
			"zzzz   z   zzzzz",
			"zzzz   zz  zzzzz",
			"zzzz   z   zzzzz",
			"zzzzz     zzzzzz",
			"zzzzz    zzzzzzz",
			"zzz       z  zzz",
			"zzz  zz      zzz",
			"zz   zzz     zzz",
			"zz   zzzz    zzz",
			"zzz   zz      zz",
			"zzzz      zz  zz",
			"zzzzzzzzzzzz zzz",
			"zzzzzzzzzzzzzzzz",
		};
		inline int getID() {
			return HtmlTag::plugin.isDarkModeEnabled() ? decorationDarkID : decorationID;
		}
		inline const char *const *getData() {
			return HtmlTag::plugin.isDarkModeEnabled() ? decorationDark : decoration;
		}
	}
}
}
