/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
  Original Pascal unit (c) Martijn Coppoolse <https://github.com/vor0nwe>
*/
#include <iomanip>
#include "TextConv.h"
#include "HtmlTag.h"
#include "Unicode.h"

using namespace HtmlTag;
using namespace SciTextObjects;
using HtmlTag::Entities::EntityReplacementScope;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
int doEncode(std::wstring &text, bool multiSel);
[[maybe_unused]] int doEncode(SciTextRange &range);
}

// --------------------------------------------------------------------------------------
// HtmlTag::Unicode
// --------------------------------------------------------------------------------------
void Unicode::encode(EntityReplacementScope /* scope */) {
	SciActiveDocument const &doc = plugin.activeDocument();
	std::wstring targetText{ doc.currentSelection().text() };
	bool multiSel = (doc.getSelectionMode() != smStreamSingle);
	if (doEncode(targetText, multiSel) > 0) {
		doc.currentSelection() = &targetText[0];
		doc.currentSelection().clearSelection();
	}
}
// --------------------------------------------------------------------------------------
int Unicode::decode() {
	int result = 0;
	SciActiveDocument const &doc = plugin.activeDocument();

	if (doc.getSelectionMode() != smStreamSingle)
		return result;

	Sci_Position lenPrefix = static_cast<Sci_Position>(plugin.options.unicodePrefix.size());
	std::wstring pattern(plugin.options.unicodeRE.size() + 1, L'\0');
	TextConv::bytesToText(plugin.options.unicodeRE.c_str(), pattern, CP_ACP);
	SciTextRange target(doc, doc.currentSelection().startPos(), doc.currentSelection().endPos());
	SciTextRange match{ doc };
	std::wstring mbCharBuf(3, L'\0');

	doc.sendMessage(SCI_BEGINUNDOACTION);
	try {
		do {
			doc.find(&pattern[0], match, SCFIND_REGEXP, target.startPos(), target.endPos());
			if (match.length() != 0) {
				// Adjust the target already
				target.startPos(match.startPos() + 1);

				// Check if code point belongs to a multi-byte glyph
				int head = 0, tail = 0;
				head = std::stoi(match.text().substr(lenPrefix, 6), nullptr, 16);
				if (head >= 0x010000 && head <= 0x10FFFF) {
					tail = ((head - 0x10000) & 0x03FF) + 0xDC00;
					head = ((head - 0x10000) >> 10) + 0xD800;
					mbCharBuf[0] = static_cast<wchar_t>(head);
					mbCharBuf[1] = static_cast<wchar_t>(tail);
					match = mbCharBuf;
				} else if (head >= 0xD800 && head <= 0xDBFF) {
					SciTextRange matchNext{ doc };
					doc.find(&pattern[0], matchNext, SCFIND_REGEXP, match.endPos() - lenPrefix,
					    target.endPos());
					if (matchNext.length() != 0) {
						tail = std::stoi(matchNext.text().substr(lenPrefix, 4), nullptr, 16);
						if (tail > 0 && tail < UINT_LEAST16_MAX) {
							mbCharBuf[0] = static_cast<wchar_t>(head);
							mbCharBuf[1] = static_cast<wchar_t>(tail);
							matchNext = L"";
							match = mbCharBuf;

							if (result < 1)
								doc.currentSelection().startPos(match.startPos());
						}
					}
				} else {
					mbCharBuf[0] = static_cast<wchar_t>(head);
					mbCharBuf[1] = 0;
					match = mbCharBuf;
				}

				if (result < 1)
					doc.currentSelection().startPos(match.startPos());

				++result;
			}
		} while (match.length() != 0);
	} catch (...) {
	}
	doc.sendMessage(SCI_ENDUNDOACTION);

	if (result > 0)
		doc.currentSelection().clearSelection();

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
int doEncode(std::wstring &text, bool multiSel) {
	int result = 0;
	if (multiSel)
		return result;

	std::wstring prefix(plugin.options.unicodePrefix.size() + 1, L'\0');
	TextConv::bytesToText(plugin.options.unicodePrefix.c_str(), prefix, CP_ACP);

	for (std::ptrdiff_t chIndex = std::ssize(text) - 1; chIndex >= 0; chIndex--) {
		uint32_t charCode = text[chIndex];
		if (charCode > 127) {
			std::wstringstream encoded;
			std::streamsize nDigits = 4;
			size_t startPos = chIndex, endPos = chIndex + 1;
			const size_t chPrevIndex = static_cast<size_t>(std::max(0LL, chIndex - 1LL));
			const uint32_t chPrevCode = text[chPrevIndex];
			if (chPrevCode >= 0xD800 && chPrevCode <= 0xDBFF) {
				charCode = ((chPrevCode & 0x03FFU) << 10) | (charCode & 0x03FFU) | 0x10000U;
				startPos = chPrevIndex;
				nDigits = 6;
				chIndex--;
			}
			encoded << text.substr(0, startPos) << prefix << std::uppercase << std::hex
				<< std::setw(nDigits) << std::setfill(L'0') << charCode << text.substr(endPos);
			text = encoded.str();
			++result;
			if (chIndex >= std::ssize(text))
				break;
		}
	}
	return result;
}
// --------------------------------------------------------------------------------------
int doEncode(SciTextRange &range) {
	std::wstring targetText{ range.text() };
	int result = doEncode(targetText, false);
	if (result > 0) {
		range = &targetText[0];
		range.clearSelection();
	}
	return result;
}
}
