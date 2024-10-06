/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef TEXT_CONV_H
#define TEXT_CONV_H

#include <windows.h>
#include <string>
#include <algorithm>

/// String conversion utilities
namespace TextConv {
/// @brief Encodes a byte buffer according to @c cp and stores the result in a wide string.
void bytesToText(const char *src, std::wstring &dest, UINT cp = CP_UTF8);
/// @brief Encodes a wide string according to @c cp and stores the result in a character string.
void textToBytes(const wchar_t *src, std::string &dest, UINT cp = CP_ACP);
/// @brief @c true if both strings are the same, ignoring case.
bool sameText(std::string lhs, std::string rhs);
/// @copydoc TextConv::sameText(std::string, std::string)
bool sameText(std::wstring lhs, std::wstring rhs);
/// @brief Returns the 1-based index of a substring within a string, or 0 if not found.
/// @see https://www.freepascal.org/docs-html/rtl/system/pos.html
size_t pos(const char *subStr, std::string const &str, size_t offSet = 0ULL);
/// @copydoc TextConv::pos(const char *, const std::string, size_t)
size_t pos(const wchar_t *subStr, std::wstring const &wstr, size_t offSet = 0ULL);
/// @brief Removes all whitespace from a string.
/// @see https://stackoverflow.com/a/83538
template <typename Str_T = std::string>
void trim(Str_T &s) {
	s.erase(remove_if(s.begin(), s.end(), isspace), s.end());
}
}
#endif // ~TEXT_CONV_H
