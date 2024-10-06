/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef SCI_API_H
#define SCI_API_H

/// @defgroup SCI_API
/// @brief Scintilla API identifiers and deprecated symbols
/// @{
/// @def SCI_FINDTEXT
/// Provided for backward compatibility with Npp < v8.4.3
/// @see https://community.notepad-plus-plus.org/post/81355
#ifndef SCI_FINDTEXT
#define SCI_FINDTEXT 2150
#endif
/// @def SCI_GETTEXTRANGE
/// @copydoc #SCI_FINDTEXT
#ifndef SCI_GETTEXTRANGE
#define SCI_GETTEXTRANGE 2162
#endif

/// Scintilla API milestone versions
enum class SciApiLevel {
	/// Scintilla <= v4.4.6
	sciApi_LT_5,
	/// SCI_GETTEXT, SCI_GETSELTEXT and SCI_GETCURLINE return a string length that does @b not count the @c NULL
	/// @see https://groups.google.com/g/scintilla-interest/c/DoRE5t2vihE
	sciApi_GTE_515,
	/// SCI_GETTEXTRANGEFULL, SCI_FINDTEXTFULL and SCI_FORMATRANGEFULL introduced
	/// @see https://groups.google.com/g/scintilla-interest/c/mPLwYdC0-FE
	sciApi_GTE_523,
	/// SCI_REPLACETARGETMINIMAL introduced
	/// @see https://groups.google.com/g/scintilla-interest/c/9OG2VdnWJ5I
	sciApi_GTE_532,
	/// SCI_CHANGESELECTIONMODE, SCI_SETMOVEEXTENDSSELECTION and SCI_SELECTIONFROMPOINT introduced
	/// @see https://groups.google.com/g/scintilla-interest/c/gb8mmFg64vg
	sciApi_GTE_541
};
/// @}
#endif // ~SCI_API_H; vim: ft=cpp
