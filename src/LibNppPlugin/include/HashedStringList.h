/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef HASHED_STRING_LIST_H
#define HASHED_STRING_LIST_H

#include <string>
#include <initializer_list>
#include <unordered_map>

/// Quick and dirty C++ adaptation of Free Pascal's @c THashedStringList
/// @see https://www.freepascal.org/docs-html/fcl/inifiles/thashedstringlist.html
template <typename Str_T = std::string>
struct HashedStringList {
	explicit HashedStringList() noexcept { nameValueSeparator = "="; }

	void addStrings(std::initializer_list<Str_T> source, bool clearFirst = false) {
		if (clearFirst)
			_strings.clear();
		for (Str_T const &str : source) {
			size_t delim = str.find_first_of(nameValueSeparator);
			addPair(str.substr(0, delim), str.substr(delim + 1));
		}
	}

	HashedStringList operator=(HashedStringList<Str_T> const &source) {
		if (source)
			_strings.clear();
		for (auto &&pair : source._strings)
			addPair(pair.first, pair.second);
		return *this;
	}

	Str_T const &operator[](Str_T const &key) const {
		auto it = _strings.find(key);
		return (it != _strings.end() ? it->second : _empty);
	}

	operator bool() const noexcept { return !_strings.empty(); }
	Str_T &addPair(Str_T const &key, Str_T const &val) { return _strings[key] = val; }

	Str_T nameValueSeparator;

private:
	std::unordered_map<Str_T, Str_T> _strings;
	Str_T _empty{};
};

template <>
inline HashedStringList<std::wstring>::HashedStringList() noexcept {
	nameValueSeparator = L"=";
}
#endif // ~HASHED_STRING_LIST_H
