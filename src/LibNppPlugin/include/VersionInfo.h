/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef VERSION_INFO_H
#define VERSION_INFO_H

#include <sstream>

/// A 32-bit file version number
struct Version {
	explicit Version(int x = 0, int y = 0, int z = 0, int a = 0) : major(x), minor(y), revision(z), build(a) {}

	friend constexpr bool operator<(const Version &a, const Version &b) noexcept {
		return (a.major < b.major) || (a.major == b.major && a.minor < b.minor) ||
		       (a.major == b.major && a.minor == b.minor && a.revision < b.revision) ||
		       (a.major == b.major && a.minor == b.minor && a.revision == b.revision && a.build < b.build);
	}

	friend constexpr bool operator>(const Version &a, const Version &b) noexcept { return (b < a); }
	friend constexpr bool operator<=(const Version &a, const Version &b) noexcept { return !(a > b); }
	friend constexpr bool operator>=(const Version &a, const Version &b) noexcept { return !(a < b); }

	const std::wstring str() const {
		std::wstringstream buf;
		if (!build)
			buf << major << L'.' << minor << L'.' << revision;
		else
			buf << major << L'.' << minor << L'.' << revision << L'.' << build;
		return buf.str();
	}

	int major, minor, revision, build;
};
#endif // ~VERSION_INFO_H
