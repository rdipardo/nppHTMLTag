/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef HTMLTAG_XPM_H
#define HTMLTAG_XPM_H

#include "HtmlTag.h"

namespace HtmlTag {
namespace Entities {
	namespace XPM {
		constexpr int decorationID = 0x7f;
		constexpr int decorationDarkID = decorationID << 0x1;
		constexpr int gitHubID = decorationID << 0x2;
		constexpr int gitHubDarkID = decorationID << 0x3;
		constexpr const char *decoration[] = {
			/* columns rows colors chars-per-pixel */
			"16 16 2 1 ",
			"  c #262626",
			"z c None",
			/* pixels */
			"zzzzzzzzzzzzzzzz",
			"zzzzzzzzzzzzzzzz",
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
			"zzzzzzzzzzzzzzzz",
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
		constexpr const char *gitHubDecoration[] = {
			/* columns rows colors chars-per-pixel */
			"16 16 4 1 ",
			"  c None",
			". c #131112",
			"X c #171516",
			"o c #BCBCBC",
			/* pixels */
			"     XXXXXX     ",
			"   XXXXXXXXXX   ",
			"  XXXXXXXXXXXX  ",
			" XXX.XXXXXX.XXX ",
			" XXX        XXX ",
			"XXXX        XXXX",
			"XXX          XXX",
			"XXX          XXX",
			"XXX          XXX",
			"XXX          XXX",
			"XXXX        XXXX",
			" XXXXX    XXXXX ",
			" XX XX    XXXXX ",
			"  XX      XXXX  ",
			"   XXX    XXX   ",
			"     o    o     ",
		};
		constexpr const char *gitHubDecorationDark[] = {
			/* columns rows colors chars-per-pixel */
			"16 16 2 1 ",
			"  c None",
			". c #FEFEFE",
			/* pixels */
			"     ......     ",
			"   ..........   ",
			"  ............  ",
			" ... ...... ... ",
			" ...        ... ",
			"....        ....",
			"...          ...",
			"...          ...",
			"...          ...",
			"...          ...",
			"....        ....",
			" .....    ..... ",
			" ..  .    ..... ",
			"  ..      ....  ",
			"   ...    ...   ",
			"                ",
		};
		inline int getID() {
			return HtmlTag::plugin.isDarkModeEnabled() ? decorationDarkID : decorationID;
		}
		inline int getGitHubID() {
			return HtmlTag::plugin.isDarkModeEnabled() ? gitHubDarkID : gitHubID;
		}
		inline const char *const *getData() {
			return HtmlTag::plugin.isDarkModeEnabled() ? decorationDark : decoration;
		}
		inline const char *const *getGitHubData() {
			return HtmlTag::plugin.isDarkModeEnabled() ? gitHubDecorationDark : gitHubDecoration;
		}
	}
}
}
#endif // ~HTMLTAG_XPM_H
