/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
  Original Pascal unit (c) Martijn Coppoolse <https://github.com/vor0nwe>
*/
#include "TextConv.h"
#include "TagFinder.h"

using namespace HtmlTag;
using namespace SciTextObjects;
using namespace TextConv;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
enum SearchDirection { dirBackward = -1, dirNone, dirForward, dirUnknown };

struct TagPair {
	std::string name;
	std::shared_ptr<SciTextRange> tag;
};

std::shared_ptr<SciTextRange> extractTagName(
    std::string &tagName, bool &isOpenTag, bool &isEndTag, Sci_Position tagPos = -1);
void selectTags(SciTextRange *startTag, SciTextRange *endTag = nullptr);

/* https://html.spec.whatwg.org/multipage/syntax.html#void-elements */
constexpr const char *voidElements[] = {
	"AREA",
	"BASE",
	"BASEFONT",
	"BR",
	"COL",
	"EMBED",
	"FRAME",
	"HR",
	"IMG",
	"INPUT",
	"ISINDEX",
	"LINK",
	"META",
	"PARAM",
	"SOURCE",
	"TRACK",
	"WBR",
};
constexpr int ncHighlightTimeout = 1000;
}

// --------------------------------------------------------------------------------------
// HtmlTag::TagFinder
// --------------------------------------------------------------------------------------
void TagFinder::findMatchingTag(unsigned options) {
	std::string tagName;
	bool dispose = false;
	SearchDirection searchDirection = dirUnknown;
	SciActiveDocument const &doc = plugin.activeDocument();
	SciTextRange match{ doc };
	std::shared_ptr<SciTextRange> currentTag = nullptr;
	std::vector<TagPair> matchingTags;
	// --------------------------------------------------------------------------------------
	auto classifyTag = [&matchingTags, &currentTag, &match, &tagName, &searchDirection, &dispose](
			       SearchDirection processDirection, char prefix) {
		tagName = prefix + tagName;

		if (matchingTags.empty()) {
			matchingTags.push_back(TagPair{ tagName, currentTag });
			dispose = false;
			searchDirection = processDirection;
		} else if (sameText(tagName.substr(1), matchingTags.front().name.substr(1))) {
			if (searchDirection == processDirection) {
				matchingTags.push_back(TagPair{ tagName, currentTag });
				dispose = false;
			} else {
				if (matchingTags.size() > 1) {
					matchingTags.pop_back();
				} else {
					match = *currentTag;
					matchingTags.push_back(TagPair{ tagName, currentTag });
					dispose = false;
				}
			}
		}
	};
	// --------------------------------------------------------------------------------------
	SciTextRange nextTag{ doc };
	bool isStartTag = false, isEndTag = false;
	bool isXML = (plugin.documentLangType() == L_XML);
	bool wantSelection = !(options & soNone);
	bool contentsOnly = wantSelection && !(options & soTags);
	bool tagsOnly = wantSelection && !(options & soContents);

	try {
		do {
			dispose = true;
			if (!nextTag) {
				// The first time, begin at the document's current position
				currentTag = extractTagName(tagName, isStartTag, isEndTag);
			} else {
				currentTag = extractTagName(tagName, isStartTag, isEndTag, nextTag.startPos() + 1);
				nextTag.clearRange();
			}

			if (currentTag && !tagName.empty()) {
				// HTML void elements are self-closing
				if (!isXML && isStartTag && !isEndTag) {
					for (size_t i = 0; i < ARRAYSIZE(voidElements); i++) {
						if (sameText(tagName, voidElements[i])) {
							isEndTag = true;
							break;
						}
					}
				}

				if (isStartTag && isEndTag) {
					tagName = '*' + tagName;
					if (matchingTags.empty()) {
						match = *currentTag;
						matchingTags.push_back(TagPair{ tagName, currentTag });
						dispose = false;
						searchDirection = dirNone;
					}
				} else if (isStartTag) {
					classifyTag(dirForward, '+');
				} else if (isEndTag) {
					classifyTag(dirBackward, '-');
				} else { // A tag that doesn't open and doesn't close?!?
					::MessageBeep(MB_ICONWARNING);
				}
			}

			// Find the next tag in the search direction
			switch (searchDirection) {
				case dirForward: { // Look forward for corresponding closing tag
					nextTag.clearRange();
					doc.find(LR"(<[^%\?\r\n\t 0-9])", nextTag, SCFIND_REGEXP | SCFIND_POSIX,
					    currentTag->endPos());
					if (nextTag.length() != 0)
						nextTag.endPos(nextTag.endPos() - 1);
					else
						nextTag.clearRange();
					break;
				}
				case dirBackward: { // Look backward for corresponding opening tag
					Sci_Position initPos = currentTag->startPos();
					do {
						nextTag.clearRange();
						doc.find(L">", nextTag, 0, initPos, 0);
						if (nextTag.length() != 0) {
							if (nextTag.startPos() == 0) {
								nextTag.clearRange();
								break;
							}
							nextTag.startPos(nextTag.startPos() - 1);
							if (nextTag.text()[0] == L'%' || nextTag.text()[0] == L'?') {
								initPos = nextTag.startPos();
								continue;
							} else {
								nextTag.startPos(nextTag.startPos() + 1);
								break;
							}
						} else
							nextTag.clearRange();
					} while (nextTag);
					break;
				}
				default: // dirUnknown, dirNone
					nextTag.clearRange();
					break;
			}

			if (dispose) {
				currentTag = nullptr;
			}
		} while (nextTag && !match);

		if (match) {
			if (matchingTags.size() == 2) {
				currentTag = matchingTags.front().tag;

				// Matching tag may be hidden by a fold
				doc.sendMessage(SCI_FOLDLINE, doc.sendMessage(SCI_LINEFROMPOSITION, match.startPos()),
				    SC_FOLDACTION_EXPAND);

				if (wantSelection && !tagsOnly) {
					SciTextRange selRange{ doc }, selRangeNoSpaces{ doc };
					if (currentTag->startPos() < match.startPos()) {
						if (contentsOnly) {
							selRange.startPos(currentTag->endPos());
							selRange.endPos(match.startPos());
						} else {
							selRange.startPos(currentTag->startPos());
							selRange.endPos(match.endPos());
						}
					} else {
						if (contentsOnly) {
							selRange.startPos(match.endPos());
							selRange.endPos(currentTag->startPos());
						} else {
							selRange.startPos(match.startPos());
							selRange.endPos(currentTag->endPos());
						}
					}
					// TODO: make optional, read setting from .ini ([MatchTag] SkipWhitespace=1)
					if (contentsOnly) {
						// Leave out whitespace at beginning
						doc.find(LR"([^ \r\n\t])", selRangeNoSpaces,
						    SCFIND_REGEXP | SCFIND_POSIX, selRange.startPos(),
						    selRange.endPos());
						if (selRangeNoSpaces.length() != 0)
							selRange.startPos(selRangeNoSpaces.startPos());
						// Also leave out whitespace at end
						doc.find(LR"([^ \r\n\t])", selRangeNoSpaces,
						    SCFIND_REGEXP | SCFIND_POSIX, selRange.endPos(),
						    selRange.startPos());
						if (selRangeNoSpaces.length() != 0)
							selRange.endPos(selRangeNoSpaces.endPos());
					}
					selRange.select();
				} else if (wantSelection) {
					selectTags(currentTag.get(), &match);
				} else {
					match.select();
				}
			} else { // Self-closing tag
				if (tagsOnly)
					selectTags(&match);
				else
					match.select();
			}
		} else if (!matchingTags.empty()) { // A tag with no match
			currentTag = matchingTags.front().tag;

			if (wantSelection)
				currentTag->select();

			currentTag->mark(STYLE_BRACEBAD, ncHighlightTimeout);
			::MessageBeep(MB_ICONWARNING);
		}

		while (!matchingTags.empty())
			matchingTags.pop_back();

	} catch (...) {
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
std::shared_ptr<SciTextRange> extractTagName(
    std::string &tagName, bool &isOpenTag, bool &isEndTag, Sci_Position tagPos) {
	SciActiveDocument const &doc = plugin.activeDocument();
	bool closureFound = false;
	isOpenTag = true;
	isEndTag = false;
	tagName.clear();

	if (tagPos < 0) {
		const Sci_Position &caret = doc.currentPosition();
		// Make sure the default search direction is forward
		tagPos = (caret <= doc.sendMessage(SCI_GETANCHOR)) ? caret + 1 : caret;
	}

	std::shared_ptr<SciTextRange> result = std::make_shared<SciTextRange>(doc);
	// Search backward -- i.e, endPos(0) < startPos(tagPos) -- in case the nearest tag is *before* the caret
	doc.find(L"<", *result, 0, tagPos, 0);
	if (result->length() == 0) {
		// Try searching forward, in case the nearest tag is *after* the caret
		doc.find(L"<", *result, 0, tagPos);
		if (result->length() == 0)
			return result;
	}

	SciTextRange tagEnd{ doc };
	doc.find(L">", tagEnd, 0, result->endPos() + 1);
	if (tagEnd.length() == 0)
		return result;
	else
		result->endPos(tagEnd.endPos());

	std::string digits, letters, attrchars{ "-_.:" };
	textToBytes(result->text().data(), tagName, CP_ACP);
	textToBytes(&Entities::scDigits[0], digits, CP_ACP);
	textToBytes(&Entities::scLetters[0], letters, CP_ACP);

	size_t startIndex = 0;
	size_t endIndex = 0;

	for (size_t i = 1; i < tagName.length(); i++) {
		// Exit early when it's obviously a self-closing tag
		if (tagName.substr(i) == "/>") {
			isOpenTag = true;
			isEndTag = true;
			endIndex = i - 1;
			break;
		} else if (startIndex == 0) {
			if (tagName[i] == '/') {
				isOpenTag = false;
				isEndTag = true;
			} else if (digits.find(tagName[i]) != std::string::npos ||
				   letters.find(tagName[i]) != std::string::npos ||
				   attrchars.find(tagName[i]) != std::string::npos) {
				startIndex = i;
			}
		} else if (endIndex == 0) {
			if (tagName[i] && digits.find(tagName[i]) == std::string::npos &&
			    letters.find(tagName[i]) == std::string::npos &&
			    attrchars.find(tagName[i]) == std::string::npos) {
				endIndex = i - 1;
				if (isEndTag) {
					break;
				}
			}
		} else {
			if (tagName[i] == '/') {
				closureFound = true;
			} else if (closureFound && !isspace(tagName[i])) {
				closureFound = false;
			}
		}
	}

	isEndTag = (isEndTag || closureFound);
	if (endIndex == 0) {
		tagName = tagName.substr(startIndex);
	} else {
		tagName = tagName.substr(startIndex, endIndex - startIndex + 1);
	}

	return result;
}
// --------------------------------------------------------------------------------------
void selectTags(SciTextRange *startTag, SciTextRange *endTag) {
	SciActiveDocument const &doc = plugin.activeDocument();
	const std::wstring startTagName = startTag->text();
	std::wstring tagNameBuf;
	size_t tagAttrPos = pos(L" ", startTagName);

	// Trim attributes from tag selection
	if (tagAttrPos > pos(L"<", startTagName)) {
		startTag->endPos(startTag->startPos() + tagAttrPos);
	}
	// Trim '<' or '</' and '>' from selection
	if (endTag == nullptr) {
		// Narrow the selection around a self-closing tag
		startTag->startPos(startTag->startPos() + pos(L"<", startTagName));
		if (startTag->text().find(L"/>") != std::wstring::npos)
			startTag->endPos(startTag->endPos() - 1);
	} else {
		tagNameBuf = startTagName.substr(0, startTag->length());
		startTag->startPos(startTag->startPos() + (pos(L"/", tagNameBuf) >> 1) + 1);
	}

	doc.sendMessage(SCI_SETSELECTION, startTag->startPos(), startTag->endPos() - 1);

	if (endTag) {
		const std::wstring endTagName = endTag->text();
		tagAttrPos = pos(L" ", endTagName);

		if (tagAttrPos > pos(L"<", endTagName))
			endTag->endPos(endTag->startPos() + tagAttrPos);

		tagNameBuf = endTagName.substr(0, endTag->length());
		doc.sendMessage(
		    SCI_ADDSELECTION, endTag->startPos() + (pos(L"/", tagNameBuf) >> 1) + 1, endTag->endPos() - 1);
	}
}
}
