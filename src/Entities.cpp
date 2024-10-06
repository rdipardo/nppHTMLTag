/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
  Original Pascal unit (c) Martijn Coppoolse <https://github.com/vor0nwe>
*/
#include "TextConv.h"
#include "HtmlTag.h"
#include "Entities.h"

using namespace HtmlTag;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
int doEncode(std::wstring &text, EntityList &entities, bool includeLineBreaks);
}

// --------------------------------------------------------------------------------------
// HtmlTag::Entities
// --------------------------------------------------------------------------------------
void Entities::encode(EntityReplacementScope scope, bool includeLineBreaks) {
	EntityList entities{};
	plugin.getEntities(entities);

	switch (scope) {
		case EntityReplacementScope::ersDocument: {
			SciActiveDocument doc = plugin.editor().activeDocument();
			SciTextRange range = doc.getRange(0, doc.length());
			std::wstring text{ range.text() };
			if (doEncode(text, entities, includeLineBreaks) > 0)
				range = text;
			break;
		}

		case EntityReplacementScope::ersAllDocuments: {
			for (size_t docIndex = 0; docIndex < plugin.editor().getViews().size(); docIndex++) {
				SciActiveDocument doc = plugin.editor().getViews()[docIndex];
				SciTextRange range = doc.getRange(0, doc.length());
				std::wstring text{ range.text() };
				if (doEncode(text, entities, includeLineBreaks) > 0)
					range = text;
			}
			break;
		}

		default: { // ersSelection
			SciActiveDocument doc = plugin.editor().activeDocument();
			std::wstring text{ doc.currentSelection().text() };
			if (doEncode(text, entities, includeLineBreaks) > 0) {
				doc.currentSelection() = text;
				doc.currentSelection().clearSelection();
			}
			break;
		}
	}
}
// --------------------------------------------------------------------------------------
int Entities::decode() {
	int result = 0;
	SciActiveDocument doc = plugin.editor().activeDocument();
	EntityList entities{};
	plugin.getEntities(entities);

	if (!entities || doc.getSelectionMode() != smStreamSingle)
		return result;

	std::wstring target{ doc.currentSelection().text() };
	size_t charIndex = target.find(L'&');

	// Make sure the selection includes the semicolon
	if (target.find(L';', charIndex) == std::wstring::npos)
		return result;

	try {
		while (charIndex != std::string::npos) {
			size_t firstPos = charIndex + 1;
			size_t lastPos = firstPos;
			size_t nextIndex = target.length() + 1;
			bool isNumeric = false, isHex = false;
			std::wstringstream allowedChars;

			for (size_t i = 1; i < target.length() - firstPos; i++) {
				if (i == 1) {
					if (target[firstPos] == L'#') {
						isNumeric = true;
						allowedChars << L"x" << scDigits;
					} else
						allowedChars << scLetters << L";";
				} else if (i == 2) {
					if (isNumeric) {
						if (target[firstPos + 1] == L'x') {
							isHex = true;
							allowedChars << scHexLetters << L";";
						} else
							allowedChars << scDigits << L";";
					}
				}

				if (allowedChars.str().find(target[firstPos + i]) == std::wstring::npos) {
					// Invalid char found
					lastPos = firstPos + i - 1;
					nextIndex = firstPos + i;
					break;
				} else if (target[firstPos + i] == L';') {
					// End found
					lastPos = firstPos + i - 1;
					nextIndex = firstPos + i + 1;
					break;
				}
			}

			int codePoint = 0;
			bool isValid = false;

			if (isNumeric) {
				if (isHex) {
					std::wstring hexStr = target.substr(firstPos + 2, lastPos - firstPos - 1);
					codePoint = std::stoi(hexStr, nullptr, 16);
					isValid = (codePoint != 0);
				} else {
					std::wstring numStr = target.substr(firstPos + 1, lastPos - firstPos);
					codePoint = std::stoi(numStr);
					isValid = (codePoint != 0);
				}
			} else {
				std::string entityCodeStr;
				std::wstring entityCode = target.substr(firstPos, lastPos - firstPos + 1);
				TextConv::textToBytes(entityCode.c_str(), entityCodeStr, CP_ACP);
				std::string entity = entities[entityCodeStr];
				if (!entity.empty()) {
					codePoint = std::stoi(entity);
					isValid = (codePoint != 0);
				} else {
					codePoint = 0;
					isValid = false;
				}
			}

			if (isValid) {
				target = target.substr(0, firstPos - 1) + static_cast<wchar_t>(codePoint) +
					 target.substr(nextIndex);
				++result;
			}

			charIndex = target.find(L'&', firstPos);
		}
	} catch (...) {
	}

	if (result > 0) {
		doc.currentSelection() = target;
		doc.currentSelection().clearSelection();
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
int doEncode(std::wstring &text, Entities::EntityList &entities, bool includeLineBreaks) {
	int result = 0;
	SciActiveDocument doc = plugin.editor().activeDocument();

	if (!entities || doc.getSelectionMode() != smStreamSingle)
		return result;

	std::wstring encodedEntity;
	bool didReplace = false;

	try {
		for (intptr_t chIndex = text.length() - 1; chIndex >= 0; chIndex--) {
			const std::wint_t charCode = text[chIndex];
			std::string entity = entities[std::to_string(charCode)];
			if (!entity.empty()) {
				didReplace = true;
				TextConv::bytesToText(entity.c_str(), encodedEntity);
			} else if (charCode > 127 || (includeLineBreaks && (charCode == L'\n' || charCode == L'\r'))) {
				didReplace = true;
				encodedEntity = L"#" + std::to_wstring(charCode);
			} else
				didReplace = false;

			if (didReplace) {
				text = text.substr(0, chIndex) + L'&' + encodedEntity + L';' + text.substr(chIndex + 1);
				++result;
			}
		}
	} catch (...) {
	}

	return result;
}
}
