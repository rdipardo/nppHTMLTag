/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef HTMLTAG_RESOURCES_H
#define HTMLTAG_RESOURCES_H

#include <windows.h>

#define HTMLTAG_VERSION L"1.5.0.0\0"
#define HTMLTAG_VERSION_WORDS 1, 5, 0, 0

#define ID_ABOUT_HTML_TAG_DLG 0x1000
#define ID_UNICODE_FMT_CONFIG_DLG 0x2000

#define ID_PLUGIN_VERSION_TXT (ID_ABOUT_HTML_TAG_DLG + 0x1)

#define ID_RELEASE_NOTES_LINK (ID_ABOUT_HTML_TAG_DLG + 0x2)
#define RELEASE_NOTES_URL L"https://bitbucket.org/rdipardo/htmltag/src/HEAD/NEWS.textile"

#define ID_BUG_TRACKER_LINK (ID_ABOUT_HTML_TAG_DLG + 0x4)
#define BUG_TRACKER_URL L"https://github.com/rdipardo/nppHTMLTag/issues"

#define ID_PLUGIN_REPO_LINK (ID_ABOUT_HTML_TAG_DLG + 0x8)
#define PLUGIN_REPO_URL L"https://bitbucket.org/rdipardo/htmltag/downloads";

#define ID_PLUGIN_AUTHOR_TXT (ID_ABOUT_HTML_TAG_DLG + 0x10)
#define PLUGIN_AUTHOR_COPYRIGHT L"\251 2007-2020 Martijn Coppoolse (v0.1 - v1.1)"

#define ID_PLUGIN_MAINTAINER_TXT (ID_ABOUT_HTML_TAG_DLG + 0x20)
#define PLUGIN_MAINTAINER_COPYRIGHT L"\251 2022-2024 Robert Di Pardo (since v1.2)"

#define ID_PLUGIN_LICENSE_TXT (ID_ABOUT_HTML_TAG_DLG + 0x40)
#define PLUGIN_LICENSE L"Licensed under the MPL 2.0"

#define ID_SIMPLEINI_TXT (ID_ABOUT_HTML_TAG_DLG + 0x80)
#define ID_SIMPLEINI_LINK (ID_ABOUT_HTML_TAG_DLG + 0x100)
#define ID_SIMPLEINI_LICENSE_TXT (ID_ABOUT_HTML_TAG_DLG + 0x200)
#define SIMPLEINI_COPYRIGHT L", \251 Brodie Thiesfield, MIT License"
#define SIMPLEINI_URL L"https://github.com/brofield/simpleini#readme"

#define ID_TINYXML_TXT (ID_ABOUT_HTML_TAG_DLG + 0x81)
#define ID_TINYXML_LINK (ID_ABOUT_HTML_TAG_DLG + 0x101)
#define ID_TINYXML_LICENSE_TXT (ID_ABOUT_HTML_TAG_DLG + 0x201)
#define TINYXML_COPYRIGHT L", \251 Lee Thomason, zlib License"
#define TINYXML_URL L"https://github.com/leethomason/tinyxml2#readme"

#define ID_ENTITIES_FILE_LINK (ID_ABOUT_HTML_TAG_DLG + 0x400)
#define ID_TRANSLATIONS_FILE_LINK (ID_ABOUT_HTML_TAG_DLG + 0x800)

#define ID_UNICODE_FMT_LABEL_TXT (ID_ABOUT_HTML_TAG_DLG + 0x1000)
#define ID_UNICODE_USER_FMT_TXT (ID_ABOUT_HTML_TAG_DLG + 0x2000)
#define ID_UNICODE_CONFIG_LINK (ID_ABOUT_HTML_TAG_DLG + 0x4000)

#define ID_CONFIG_EDIT (ID_UNICODE_FMT_CONFIG_DLG + 0x1)
#define ID_CONFIG_LABEL_1 (ID_UNICODE_FMT_CONFIG_DLG + 0x2)
#define ID_CONFIG_LABEL_2 (ID_UNICODE_FMT_CONFIG_DLG + 0x4)

#define DEFAULT_WS (WS_VISIBLE | WS_CHILD)
#define DEFAULT_LINK_STYLE (DEFAULT_WS | SS_NOTIFY)
#define DEFAULT_BTTN_STYLE (DEFAULT_WS | BS_CENTER)

#define CL_LINK_DEFAULT RGB(0, 0, 238)
#define CL_LINK_DARK_MODE RGB(0, 191, 255)

#endif /* ~HTMLTAG_RESOURCES_H */
