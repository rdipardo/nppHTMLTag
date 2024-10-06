/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <locale>
#include "TextConv.h"

/////////////////////////////////////////////////////////////////////////////////////////
#ifndef WC_ERR_INVALID_CHARS
#include "HtmlTag.h"
namespace {
// Must be 0 for WinXP, or 128 if WINVER >= 0x0600
unsigned WC_ERR_INVALID_CHARS =
    (HtmlTag::plugin.sendNppMessage(NPPM_GETWINDOWSVERSION) < winVer::WV_VISTA) ? 0x00000000 : 0x00000080;
}
#endif

// --------------------------------------------------------------------------------------
// TextConv
// --------------------------------------------------------------------------------------
void TextConv::bytesToText(const char *src, std::wstring &dest, UINT cp) {
	if (!src)
		return;
	// https://learn.microsoft.com/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
	unsigned dwFlags = (cp == CP_UTF8 || cp == 54936) ? MB_ERR_INVALID_CHARS : 0;
	int len = ::MultiByteToWideChar(cp, dwFlags, src, -1, nullptr, 0);
	if (len > 0) {
		if ((size_t)len > dest.capacity())
			dest.resize(len);
		::MultiByteToWideChar(cp, dwFlags, src, -1, &dest[0], len);
		dest = std::wstring(&dest[0]);
	}
}
// --------------------------------------------------------------------------------------
void TextConv::textToBytes(const wchar_t *src, std::string &dest, UINT cp) {
	if (!src)
		return;
	// https://learn.microsoft.com/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
	unsigned dwFlags = (cp == CP_UTF8 || cp == 54936) ? WC_ERR_INVALID_CHARS : 0;
	int len = ::WideCharToMultiByte(cp, dwFlags, src, -1, nullptr, 0, nullptr, nullptr);
	if (len > 0) {
		if ((size_t)len > dest.capacity())
			dest.resize(len);
		::WideCharToMultiByte(cp, dwFlags, src, -1, &dest[0], len, nullptr, nullptr);
		dest = std::string(&dest[0]);
	}
}
// --------------------------------------------------------------------------------------
bool TextConv::sameText(std::string lhs, std::string rhs) {
	auto tolower_l = [](uint8_t c) { return std::tolower(c, std::locale()); };
	std::transform(lhs.begin(), lhs.end(), lhs.begin(), tolower_l);
	std::transform(rhs.begin(), rhs.end(), rhs.begin(), tolower_l);
	return lhs == rhs;
}
// --------------------------------------------------------------------------------------
bool TextConv::sameText(std::wstring lhs, std::wstring rhs) {
	auto towlower_l = [](std::wint_t wc) { return std::tolower(wc, std::locale()); };
	std::transform(lhs.begin(), lhs.end(), lhs.begin(), towlower_l);
	std::transform(rhs.begin(), rhs.end(), rhs.begin(), towlower_l);
	return lhs == rhs;
}
// --------------------------------------------------------------------------------------
size_t TextConv::pos(const char *subStr, std::string const &str, size_t offSet) {
	const size_t result = str.find(subStr, offSet);
	return (result == std::string::npos) ? 0 : result + 1;
}
// --------------------------------------------------------------------------------------
size_t TextConv::pos(const wchar_t *subStr, std::wstring const &wstr, size_t offSet) {
	const size_t result = wstr.find(subStr, offSet);
	return (result == std::wstring::npos) ? 0 : result + 1;
}
