/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
  Original Pascal unit (c) Martijn Coppoolse <https://github.com/vor0nwe>
*/
#include "TextConv.h"
#include "SciTextObjects.h"

using namespace SciTextObjects;
using namespace TextConv;

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
typedef std::vector<std::shared_ptr<SciTextRangeMark>> TextRangeMarks;
TextRangeMarks textRangeMarks;
}

// --------------------------------------------------------------------------------------
// SciTextObjects::SciWindowedObject
// --------------------------------------------------------------------------------------
LRESULT SciWindowedObject::sendMessage(const UINT msg, WPARAM wParam, LPARAM lParam) const {
	DWORD_PTR result = 0;
	try {
		::SendMessageTimeoutW(_windowHandle, msg, wParam, lParam, SMTO_NORMAL, 5000, &result);
	} catch (...) {
	}
	return static_cast<LRESULT>(result);
}
// --------------------------------------------------------------------------------------
LRESULT SciWindowedObject::sendMessage(const UINT msg, WPARAM wParam, void *lParam) const {
	DWORD_PTR result = 0;
	try {
		::SendMessageTimeoutW(
		    _windowHandle, msg, wParam, reinterpret_cast<LPARAM>(lParam), SMTO_NORMAL, 5000, &result);
	} catch (...) {
	}
	return static_cast<LRESULT>(result);
}
// --------------------------------------------------------------------------------------
void SciWindowedObject::postMessage(const UINT msg, WPARAM wParam, LPARAM lParam) const {
	try {
		::PostMessageW(_windowHandle, msg, wParam, lParam);
	} catch (...) {
	}
}
// --------------------------------------------------------------------------------------
void SciWindowedObject::postMessage(const UINT msg, WPARAM wParam, void *lParam) const {
	try {
		::PostMessageW(_windowHandle, msg, wParam, reinterpret_cast<LPARAM>(lParam));
	} catch (...) {
	}
}

// --------------------------------------------------------------------------------------
// SciTextObjects::SciActiveDocument
// --------------------------------------------------------------------------------------
SciTextRange SciActiveDocument::getRange(const Sci_Position startPos, const Sci_Position endPos) const {
	return SciTextRange(*this, startPos, endPos);
}
// --------------------------------------------------------------------------------------
SciTextRange SciActiveDocument::getLines(const Sci_Position firstLine, const Sci_Position count) const {
	const Sci_Position LineCount = getLineCount();
	if (firstLine + count > LineCount) {
		return SciTextRange(*this, sendMessage(SCI_POSITIONFROMLINE, firstLine), getLength());
	} else {
		return SciTextRange(*this, sendMessage(SCI_POSITIONFROMLINE, firstLine),
		    sendMessage(SCI_GETLINEENDPOSITION, firstLine + count));
	}
}
// --------------------------------------------------------------------------------------
SelectionMode SciActiveDocument::getSelectionMode() const {
	LRESULT mode = sendMessage(SCI_GETSELECTIONMODE);
	if (mode == SC_SEL_STREAM && sendMessage(SCI_GETSELECTIONS) > 1)
		mode |= multiselectionMask;
	return static_cast<SelectionMode>(mode);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::insert(std::wstring const &text, const Sci_Position position) const {
	std::string chars(sizeof(wchar_t) * text.size() + 1, 0);
	textToBytes(text.c_str(), chars, (UINT)sendMessage(SCI_GETCODEPAGE));
	sendMessage(SCI_INSERTTEXT, position, &chars[0]);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::select(const Sci_Position startPos, const Sci_Position length) const {
	const SelectionMode mode = static_cast<SelectionMode>(getSelectionMode() & ~multiselectionMask);
	if (mode != SelectionMode::smStreamSingle)
		sendMessage(SCI_SETSELECTIONMODE, SC_SEL_STREAM);
	sendMessage(SCI_SETSEL, startPos, startPos + length);
	if (mode != SelectionMode::smStreamSingle)
		sendMessage(SCI_SETSELECTIONMODE, mode);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::selectLines(const Sci_Position firstLine, const Sci_Position count) const {
	const unsigned SelMode = (unsigned)sendMessage(SCI_GETSELECTIONMODE);
	if (SelMode != SC_SEL_LINES)
		sendMessage(SCI_SETSELECTIONMODE, SC_SEL_LINES);
	sendMessage(SCI_SETSEL, sendMessage(SCI_POSITIONFROMLINE, firstLine),
	    sendMessage(SCI_GETLINEENDPOSITION, firstLine + count));
	if (SelMode != SC_SEL_LINES)
		sendMessage(SCI_SETSELECTIONMODE, SelMode);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::selectColumns(const Sci_Position startPos, const Sci_Position endPos) const {
	const unsigned SelMode = (unsigned)sendMessage(SCI_GETSELECTIONMODE);
	if (SelMode != SC_SEL_RECTANGLE)
		sendMessage(SCI_SETSELECTIONMODE, SC_SEL_RECTANGLE);
	sendMessage(SCI_SETSEL, startPos, endPos);
	if (SelMode != SC_SEL_RECTANGLE)
		sendMessage(SCI_SETSELECTIONMODE, SelMode);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::find(std::wstring const &text, SciTextRange &target, const int options) const {
	if (options != 0)
		find(text, target, options, target.getStart(), target.getEnd());
	else
		find(text, target);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::find(std::wstring const &text, SciTextRange &target, const int options,
    const Sci_Position startPos, Sci_Position endPos) const {
	UINT sciMsg = (this->_apiLevel < SciApiLevel::sciApi_GTE_523) ? SCI_FINDTEXT : SCI_FINDTEXTFULL;
	Sci_TextToFindFull ttf = Sci_TextToFindFull{};

	if (startPos < 0)
		ttf.chrg.cpMin = 0;
	else
		ttf.chrg.cpMin = startPos;
	if (endPos < 0)
		ttf.chrg.cpMax = INTPTR_MAX;
	else
		ttf.chrg.cpMax = endPos;

	std::string lpstrText(sizeof(wchar_t) * text.size() + 1, 0);
	textToBytes(text.c_str(), lpstrText, (UINT)sendMessage(SCI_GETCODEPAGE));
	ttf.lpstrText = &lpstrText[0];
	ttf.chrgText = ttf.chrg;
	LRESULT rngStart = sendMessage(sciMsg, options, &ttf);

	if (rngStart == INVALID_POSITION) {
		target.setStart(0);
		target.setEnd(0);
	} else {
		target.setStart(ttf.chrgText.cpMin);
		target.setEnd(ttf.chrgText.cpMax);
	}
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::getCurrentPos() const {
	return sendMessage(SCI_GETCURRENTPOS);
}
// --------------------------------------------------------------------------------------
SciSelection &SciActiveDocument::getSelection() const {
	return *_selection;
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::getLength() const {
	return sendMessage(SCI_GETLENGTH);
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::getLineCount() const {
	return sendMessage(SCI_GETLINECOUNT);
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::getNextLineStart() const {
	const Sci_Position lineEndCurrent =
	    sendMessage(SCI_GETLINEENDPOSITION, sendMessage(SCI_LINEFROMPOSITION, currentPosition()));
	return sendMessage(SCI_POSITIONAFTER, lineEndCurrent);
}
// --------------------------------------------------------------------------------------
bool SciActiveDocument::getReadOnly() const {
	return bool(sendMessage(SCI_GETREADONLY));
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::getFirstVisibleLine() const {
	return sendMessage(SCI_GETFIRSTVISIBLELINE);
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::getLinesOnScreen() const {
	return sendMessage(SCI_LINESONSCREEN);
}
// --------------------------------------------------------------------------------------
void SciActiveDocument::setReadOnly(const bool value) {
	sendMessage(SCI_SETREADONLY, static_cast<uintptr_t>(value));
}
// --------------------------------------------------------------------------------------
Sci_Position SciActiveDocument::currentPosition(const Sci_Position value) const {
	sendMessage(SCI_SETANCHOR, value);
	return (sendMessage(SCI_SETCURRENTPOS, value) == 0) ? value : INVALID_POSITION;
}

// --------------------------------------------------------------------------------------
// SciTextObjects::SciTextRange
// --------------------------------------------------------------------------------------
SciTextRange::SciTextRange(SciActiveDocument const &editor_, Sci_Position startPos, Sci_Position endPos)
    : _editor(editor_),
      _text(std::wstring(512, L'\0')),
      _startPos(startPos),
      _endPos(endPos) {
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getAnchor() const {
	return _editor.sendMessage(SCI_GETANCHOR);
}
// --------------------------------------------------------------------------------------
void SciTextRange::setAnchor(const Sci_Position value) {
	_editor.sendMessage(SCI_SETANCHOR, value);
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::startPos(const Sci_Position value) {
	setStart(value);
	return _startPos;
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::endPos(const Sci_Position value) {
	setEnd(value);
	return _endPos;
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getStart() const {
	return _startPos;
}
// --------------------------------------------------------------------------------------
void SciTextRange::setStart(const Sci_Position value) {
	_startPos = (value <= INVALID_POSITION) ? 0 : value;
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getEnd() const {
	return (_endPos <= INVALID_POSITION) ? this->getLength() : _endPos;
}
// --------------------------------------------------------------------------------------
void SciTextRange::setEnd(const Sci_Position value) {
	if (value <= INVALID_POSITION)
		_endPos = _editor.sendMessage(SCI_GETLENGTH);
	else
		_endPos = value;
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getLength() const {
	return std::abs(_endPos - _startPos);
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getLineCount() const {
	return getLastLine() - getFirstLine() + 1;
}
// --------------------------------------------------------------------------------------
void SciTextRange::setLength(const Sci_Position value) {
	setEnd(_startPos + value);
}
// --------------------------------------------------------------------------------------
const std::wstring SciTextRange::text() {
	if (getLength() <= 0)
		return _text;

	UINT sciMsg = (_editor._apiLevel < SciApiLevel::sciApi_GTE_523) ? SCI_GETTEXTRANGE : SCI_GETTEXTRANGEFULL;
	Sci_TextRangeFull tr = Sci_TextRangeFull{};
	std::string lpstrText(getLength() + 1, 0);
	tr.chrg.cpMin = _startPos;
	tr.chrg.cpMax = _endPos;
	tr.lpstrText = &lpstrText[0];

	_editor.sendMessage(sciMsg, 0, &tr);
	bytesToText(tr.lpstrText, _text, (UINT)_editor.sendMessage(SCI_GETCODEPAGE));
	return _text;
}
// --------------------------------------------------------------------------------------
void SciTextRange::setText(std::wstring const &value) {
	Sci_Position txtRng = 0, nReplaced = 0;
	std::string chars(sizeof(wchar_t) * value.size() + 1, 0);
	UINT sciMsg = (_editor._apiLevel < SciApiLevel::sciApi_GTE_532) ? SCI_REPLACETARGET : SCI_REPLACETARGETMINIMAL;
	textToBytes(&value[0], chars, (UINT)_editor.sendMessage(SCI_GETCODEPAGE));
	txtRng = static_cast<Sci_Position>(chars.substr(0, chars.find_first_of('\0')).size());
	_editor.sendMessage(SCI_SETTARGETSTART, _startPos);
	_editor.sendMessage(SCI_SETTARGETEND, _endPos);
	nReplaced = _editor.sendMessage(sciMsg, txtRng, &chars[0]);
	_endPos -= (_endPos - _startPos) - nReplaced;
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getStartCol() const {
	return _editor.sendMessage(SCI_GETCOLUMN, _startPos);
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getFirstLine() const {
	return _editor.sendMessage(SCI_LINEFROMPOSITION, _startPos);
}
// --------------------------------------------------------------------------------------
int SciTextRange::getIndentLevel() const {
	return (int)_editor.sendMessage(SCI_GETLINEINDENTATION, getFirstLine());
}
// --------------------------------------------------------------------------------------
void SciTextRange::setIndentLevel(const int value) {
	_editor.sendMessage(SCI_SETLINEINDENTATION, value);
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getEndCol() const {
	return _editor.sendMessage(SCI_GETCOLUMN, _endPos);
}
// --------------------------------------------------------------------------------------
Sci_Position SciTextRange::getLastLine() const {
	return _editor.sendMessage(SCI_LINEFROMPOSITION, _endPos);
}
// --------------------------------------------------------------------------------------
void SciTextRange::indent(const int levels) {
	for (Sci_Position i = getFirstLine(); i <= getLastLine(); i++) {
		_editor.sendMessage(
		    SCI_SETLINEINDENTATION, i, _editor.sendMessage(SCI_GETLINEINDENTATION, i) + LRESULT(levels));
	}
}
// --------------------------------------------------------------------------------------
void SciTextRange::mark(const int style, const unsigned durationInMs) {
	Sci_Position CurrentStyleEnd = _editor.sendMessage(SCI_GETENDSTYLED);
	_editor.sendMessage(SCI_STARTSTYLING, _startPos);
	_editor.sendMessage(SCI_SETSTYLING, getLength(), style);
	_editor.sendMessage(SCI_STARTSTYLING, CurrentStyleEnd);

	if (durationInMs > 0)
		textRangeMarks.push_back(std::make_shared<SciTextRangeMark>(*this, durationInMs));
}
// --------------------------------------------------------------------------------------
void SciTextRange::select() {
	_editor.sendMessage(SCI_SETSELECTION, _endPos, _startPos);
	_editor.sendMessage(SCI_SCROLLCARET);
}
// --------------------------------------------------------------------------------------
void SciTextRange::clearSelection() {
	setAnchor(_editor.currentPosition());
}

// --------------------------------------------------------------------------------------
// SciTextObjects::SciSelection
// --------------------------------------------------------------------------------------
Sci_Position SciSelection::getCurrentPos() const {
	return _editor.sendMessage(SCI_GETCURRENTPOS);
}
// --------------------------------------------------------------------------------------
Sci_Position SciSelection::startPos(const Sci_Position value) {
	setStart(value);
	return getStart();
}
// --------------------------------------------------------------------------------------
void SciSelection::setCurrentPos(const Sci_Position value) {
	_editor.sendMessage(SCI_SETCURRENTPOS, value);
}
// --------------------------------------------------------------------------------------
Sci_Position SciSelection::getEnd() const {
	return _editor.sendMessage(SCI_GETSELECTIONEND);
}
// --------------------------------------------------------------------------------------
void SciSelection::setEnd(const Sci_Position value) {
	_editor.sendMessage(SCI_SETSELECTIONEND, value);
}
// --------------------------------------------------------------------------------------
Sci_Position SciSelection::getLength() const {
	return std::abs(this->getEnd() - this->getStart());
}
// --------------------------------------------------------------------------------------
void SciSelection::setLength(const Sci_Position value) {
	_editor.sendMessage(SCI_SETSELECTIONEND, this->getStart() + value);
}
// --------------------------------------------------------------------------------------
Sci_Position SciSelection::getStart() const {
	return _editor.sendMessage(SCI_GETSELECTIONSTART);
}
// --------------------------------------------------------------------------------------
void SciSelection::setStart(const Sci_Position value) {
	_editor.sendMessage(SCI_SETSELECTIONSTART, value);
}
// --------------------------------------------------------------------------------------
const std::wstring SciSelection::text() {
	Sci_Position lenSel = _editor.sendMessage(SCI_GETSELTEXT, 0, nullptr);
	if (_editor.getApiLevel() >= SciApiLevel::sciApi_GTE_515)
		lenSel++;
	std::string chars(lenSel, 0);
	_editor.sendMessage(SCI_GETSELTEXT, 0, &chars[0]);
	bytesToText(&chars[0], _text, (UINT)_editor.sendMessage(SCI_GETCODEPAGE));
	return _text;
}
// --------------------------------------------------------------------------------------
void SciSelection::setText(std::wstring const &value) {
	Sci_Position lenNew = 0, endPos = 0;
	std::string chars(sizeof(wchar_t) * value.size() + 1, 0);
	textToBytes(&value[0], chars, (UINT)_editor.sendMessage(SCI_GETCODEPAGE));
	lenNew = static_cast<Sci_Position>(chars.substr(0, chars.find_first_of('\0')).size());
	bool Reversed = (this->getAnchor() > this->getCurrentPos());
	_editor.sendMessage(SCI_REPLACESEL, 0, &chars[0]);
	endPos = this->getCurrentPos();
	if (Reversed)
		_editor.sendMessage(SCI_SETSEL, endPos, endPos - lenNew);
	else
		_editor.sendMessage(SCI_SETSEL, endPos - lenNew, endPos);
}

// --------------------------------------------------------------------------------------
// SciTextObjects::SciTextRangeMark
// --------------------------------------------------------------------------------------
namespace {
void CALLBACK TextRangeUnmarkTimer(
    HWND /*unnamedParam1*/, UINT /*unnamedParam2*/, UINT_PTR eventID, DWORD /*unnamedParam4*/) {
	::KillTimer(0, eventID);
	for (auto it = textRangeMarks.cbegin(); it != textRangeMarks.cend();) {
		if ((*it)->timer() == eventID)
			it = textRangeMarks.erase(it);
		else
			++it;
	}
}
}
// --------------------------------------------------------------------------------------
SciTextRangeMark::SciTextRangeMark(SciTextRange &range, unsigned durationInMS)
    : _editor(range.editor()),
      _startPos(range.startPos()),
      _endPos(range.endPos()),
      _timerID(::SetTimer(0, 0, durationInMS, TIMERPROC(&TextRangeUnmarkTimer))) {
}
// --------------------------------------------------------------------------------------
SciTextRangeMark::~SciTextRangeMark() {
	::KillTimer(0, _timerID);
	Sci_Position currentStyleEnd = _editor.sendMessage(SCI_GETENDSTYLED);
	if (_startPos < currentStyleEnd)
		currentStyleEnd = _startPos;
	_editor.sendMessage(SCI_STARTSTYLING, currentStyleEnd);
}
