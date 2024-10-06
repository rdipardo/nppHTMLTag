/**
  @brief Class library providing simplified access to Scintilla buffers

  Adapted for C++ from <c>NppSimpleObjects.pas</c>,
  part of HTML Tag: <https://bitbucket.org/rdipardo/htmltag>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
  Original Pascal unit (c) Martijn Coppoolse <https://github.com/vor0nwe>
*/
#ifndef SCI_TEXT_OBJECTS_H
#define SCI_TEXT_OBJECTS_H

#include <string>
#include <memory>
#include <windows.h>
#include "Scintilla.h"
#include "SciApi.h"

#define UNUSED 0LL
#define UNUSEDW 0ULL

namespace SciTextObjects {
enum MessageResult : LRESULT { mrFalse, mrTrue };
enum SelectionMode : unsigned { smStreamSingle = SC_SEL_STREAM, smColumn, smLines, smThin, smStreamMulti };
constexpr unsigned multiselectionMask = 0x4;

class SciTextRange;
class SciSelection;

// --------------------------------------------------------------------------------------
// SciWindowedObject
// --------------------------------------------------------------------------------------
class SciWindowedObject {

public:
	SciWindowedObject(HWND hWnd) : _windowHandle(hWnd), _apiLevel(SciApiLevel::sciApi_GTE_541) {}
	virtual ~SciWindowedObject() = default;
	virtual LRESULT sendMessage(const UINT msg, WPARAM wParam = UNUSEDW, LPARAM lParam = UNUSED) const;
	virtual LRESULT sendMessage(const UINT msg, WPARAM wParam, void *lParam) const;
	virtual void postMessage(const UINT msg, WPARAM wParam = UNUSEDW, LPARAM lParam = UNUSED) const;
	virtual void postMessage(const UINT msg, WPARAM wParam, void *lParam) const;
	SciApiLevel getApiLevel() const { return _apiLevel; }

protected:
	HWND _windowHandle;
	SciApiLevel _apiLevel;
	virtual void setApiLevel(SciApiLevel api) { _apiLevel = api; }
};

// --------------------------------------------------------------------------------------
// SciActiveDocument
// --------------------------------------------------------------------------------------
class SciActiveDocument : public SciWindowedObject {
	friend SciTextRange;

public:
	SciActiveDocument(HWND hWnd) : SciWindowedObject(hWnd) { _selection = std::make_shared<SciSelection>(*this); }
	SciTextRange getRange(const Sci_Position startPos = 0, const Sci_Position endPos = 0) const;
	SciTextRange getLines(const Sci_Position startLine, const Sci_Position count = 1) const;
	SelectionMode getSelectionMode() const;
	void insert(std::wstring const &text, const Sci_Position pos) const;
	void select(const Sci_Position start = 0, const Sci_Position length = INVALID_POSITION) const;
	void selectLines(const Sci_Position startLine, const Sci_Position lineCount = 1) const;
	void selectColumns(const Sci_Position startPos, const Sci_Position endPos = INVALID_POSITION) const;
	void find(std::wstring const &text, SciTextRange &target, const int options = 0,
	    const Sci_Position startPos = INVALID_POSITION, Sci_Position endPos = INVALID_POSITION) const;
	void find(std::wstring const &text, SciTextRange &target, const int options) const;
	SciSelection &currentSelection() const { return getSelection(); }
	Sci_Position currentPosition() const { return getCurrentPos(); }
	Sci_Position currentPosition(const Sci_Position value) const;
	Sci_Position nextLineStartPosition() const { return getNextLineStart(); }
	Sci_Position length() const { return getLength(); }

	void setApiLevel(SciApiLevel api) override { SciWindowedObject::setApiLevel(api); };

private:
	std::shared_ptr<SciSelection> _selection = nullptr;
	SciSelection &getSelection() const;
	Sci_Position getCurrentPos() const;
	Sci_Position getLength() const;
	Sci_Position getLineCount() const;
	Sci_Position getNextLineStart() const;
	Sci_Position getFirstVisibleLine() const;
	Sci_Position getLinesOnScreen() const;
	bool getReadOnly() const;
	void setReadOnly(bool value);
};

// --------------------------------------------------------------------------------------
// SciTextRange
// --------------------------------------------------------------------------------------
class SciTextRange {
	friend SciActiveDocument;

public:
	explicit SciTextRange(SciActiveDocument const &editor_, Sci_Position startPos = 0, Sci_Position endPos = 0);
	virtual ~SciTextRange() = default;
	virtual Sci_Position startPos(const Sci_Position value);
	virtual Sci_Position startPos() const { return getStart(); }
	virtual Sci_Position endPos(const Sci_Position value);
	virtual Sci_Position endPos() const { return getEnd(); }
	virtual Sci_Position length() const { return getLength(); }
	virtual const std::wstring text();

	virtual SciTextRange operator=(SciTextRange const &other) {
		setStart(other.getStart());
		setEnd(other.getEnd());
		return *this;
	}

	virtual SciTextRange operator=(std::wstring const &value) {
		setText(value);
		return *this;
	}

	virtual operator bool() const noexcept { return getLength() > 0; }

	void select();
	void clearSelection();
	void indent(const int levels = 1);
	void mark(const int style, const unsigned timeoutMSecs = 0);
	SciActiveDocument const &editor() const { return _editor; }

protected:
	SciActiveDocument _editor;
	std::wstring _text;
	Sci_Position _startPos;
	Sci_Position _endPos;
	Sci_Position getAnchor() const;
	Sci_Position getFirstLine() const;
	Sci_Position getStartCol() const;
	Sci_Position getLastLine() const;
	Sci_Position getEndCol() const;
	Sci_Position getLineCount() const;
	int getIndentLevel() const;
	void setAnchor(const Sci_Position value);
	void setIndentLevel(const int value);
	virtual Sci_Position getStart() const;
	virtual void setStart(const Sci_Position value);
	virtual Sci_Position getEnd() const;
	virtual void setEnd(const Sci_Position value);
	virtual Sci_Position getLength() const;
	virtual void setLength(const Sci_Position value);
	virtual void setText(std::wstring const &value);
};

// --------------------------------------------------------------------------------------
// SciSelection
// --------------------------------------------------------------------------------------
class SciSelection final : public SciTextRange {

public:
	explicit SciSelection(SciActiveDocument const &editor_) : SciTextRange(editor_) {}

	Sci_Position startPos(const Sci_Position value) override;
	Sci_Position startPos() const override { return getStart(); }
	Sci_Position endPos() const override { return getEnd(); }
	Sci_Position length() const override { return getLength(); }
	const std::wstring text() override;

	SciTextRange operator=(std::wstring const &value) override {
		setText(value);
		return *this;
	}

	operator bool() const noexcept override { return getLength() > 0; }

private:
	Sci_Position getCurrentPos() const;
	void setCurrentPos(const Sci_Position value);
	Sci_Position getStart() const override;
	Sci_Position getEnd() const override;
	Sci_Position getLength() const override;
	void setStart(const Sci_Position value) override;
	void setEnd(const Sci_Position value) override;
	void setLength(const Sci_Position value) override;
	void setText(std::wstring const &value) override;
};

// --------------------------------------------------------------------------------------
// SciTextRangeMark
// --------------------------------------------------------------------------------------
class SciTextRangeMark {

public:
	SciTextRangeMark(SciTextRange &range, unsigned timeoutMSecs);
	~SciTextRangeMark();
	const uintptr_t timer() { return _timerID; }

private:
	SciActiveDocument _editor;
	Sci_Position _startPos;
	Sci_Position _endPos;
	uintptr_t _timerID;
};
}
#endif // ~SCI_TEXT_OBJECTS_H
