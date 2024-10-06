/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef HTMLTAG_TAGFINDER_H
#define HTMLTAG_TAGFINDER_H

#include "HtmlTag.h"

namespace HtmlTag {
namespace TagFinder {
	void findMatchingTag(SelectionOptions options = soNone);
}
}
#endif // ~HTMLTAG_TAGFINDER_H
