/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <cmath>
#include "Internal.h"
#include "TextConv.h"
#include "PluginBase.h"

#define MAX_WIDE_PATH 0x7fffULL

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
path_t getModulePath(HMODULE hInstace);
}

// --------------------------------------------------------------------------------------
// PluginBase
// --------------------------------------------------------------------------------------
PluginBase::~PluginBase() {
	funcItems.~FuncArray();
	if (_editor)
		delete _editor;
}
// --------------------------------------------------------------------------------------
void PluginBase::setInfo(const NppData *data) {
	_data = *data;
	_editor = SciApplication::getApplication(data, apiLevel());

	DWORD versionWords = static_cast<DWORD>(sendNppMessage(NPPM_GETNPPVERSION));
	div_t loWords = ::div((versionWords & 0xffff) * 10, 10);
	while (loWords.quot > 9) {
		_nppVersion.build = loWords.rem;
		loWords = ::div(loWords.quot, 10);
	}
	_nppVersion.major = ((versionWords >> 0x10) & 0xffff);
	_nppVersion.minor = loWords.quot;
	_nppVersion.revision = loWords.rem;
}
// --------------------------------------------------------------------------------------
HWND PluginBase::currentScintilla() const {
	intptr_t index = -1;
	sendNppMessage(NPPM_GETCURRENTSCINTILLA, UNUSEDW, &index);
	return (index > 0) ? _data._scintillaSecondHandle : _data._scintillaMainHandle;
}
// --------------------------------------------------------------------------------------
LRESULT PluginBase::sendNppMessage(const UINT msg, WPARAM wparam, LPARAM lparam) const {
	return SendMessageW(_data._nppHandle, msg, wparam, lparam);
}
// --------------------------------------------------------------------------------------
LRESULT PluginBase::sendNppMessage(const UINT msg, WPARAM wparam, void *lparam) const {
	return sendNppMessage(msg, wparam, reinterpret_cast<LPARAM>(lparam));
}
// --------------------------------------------------------------------------------------
SciApiLevel PluginBase::apiLevel() const {
	if (hasMultiSelectionModeApis())
		return SciApiLevel::sciApi_GTE_541;
	else if (hasMinimalReplacementApi())
		return SciApiLevel::sciApi_GTE_532;
	else if (hasFullRangeApis())
		return SciApiLevel::sciApi_GTE_523;
	else if (hasV5Apis())
		return SciApiLevel::sciApi_GTE_515;

	return SciApiLevel::sciApi_LT_5;
}
// --------------------------------------------------------------------------------------
bool PluginBase::supportsDarkMode() const noexcept {
	return _nppVersion.major >= 8;
}
// --------------------------------------------------------------------------------------
bool PluginBase::supportsBigFiles() const noexcept {
	const Version target{ 8, 3 };
	return _nppVersion >= target;
}
// --------------------------------------------------------------------------------------
bool PluginBase::hasV5Apis() const noexcept {
	const Version target{ 8, 4 };
	return _nppVersion >= target;
}
// --------------------------------------------------------------------------------------
bool PluginBase::hasFullRangeApis() const noexcept {
	const Version target{ 8, 4, 3 };
	return _nppVersion >= target;
}
// --------------------------------------------------------------------------------------
bool PluginBase::hasMinimalReplacementApi() const noexcept {
	const Version target{ 8, 4, 8 };
	return _nppVersion >= target;
}
// --------------------------------------------------------------------------------------
bool PluginBase::supportsDarkModeSubclassing() const noexcept {
	const Version target{ 8, 5, 4 };
	return _nppVersion >= target;
}
// --------------------------------------------------------------------------------------
bool PluginBase::hasMultiSelectionModeApis() const noexcept {
	const Version target{ 8, 6, 1 };
	return _nppVersion >= target;
}
// --------------------------------------------------------------------------------------
bool PluginBase::isDarkModeEnabled() const {
	const Version target{ 8, 4, 1 };
	return (_nppVersion >= target) && (sendNppMessage(NPPM_ISDARKMODEENABLED) == MessageResult::mrTrue);
}
// --------------------------------------------------------------------------------------
path_t PluginBase::pluginsConfigDir() const {
	wchar_t s[MAX_WIDE_PATH]{};
	sendNppMessage(NPPM_GETPLUGINSCONFIGDIR, (MAX_WIDE_PATH - 1ULL), s);
	return path_t(s, path_t::format::native_format);
}
// --------------------------------------------------------------------------------------
LangType PluginBase::documentLangType() const {
	int typeInt;
	sendNppMessage(NPPM_GETCURRENTLANGTYPE, 0, &typeInt);
	return static_cast<LangType>(typeInt);
}
// --------------------------------------------------------------------------------------
path_t PluginBase::pluginsHomeDir() const {
	wchar_t s[MAX_WIDE_PATH]{};
	sendNppMessage(NPPM_GETNPPDIRECTORY, (MAX_WIDE_PATH - 1ULL), s);
	return path_t(s, path_t::format::native_format) / path_t(L"plugins");
}
// --------------------------------------------------------------------------------------
bool PluginBase::openFile(wchar_t *filename) const {
	path_t s = currentBufferPath();
	// Ask if we are not already opened
	if (TextConv::sameText(s, filename))
		return true;
	return (sendNppMessage(WM_DOOPEN, UNUSEDW, &filename[0]) == MessageResult::mrFalse);
}
// --------------------------------------------------------------------------------------
bool PluginBase::openFile(wchar_t *filename, Sci_Position line) const {
	bool r = openFile(filename);
	if (!r)
		::SendMessageW(currentScintilla(), SCI_GOTOLINE, line, UNUSED);
	return r;
}
// --------------------------------------------------------------------------------------
path_t PluginBase::currentBufferPath(uintptr_t bufferId) const {
	std::wstring result;
	wchar_t pathbuf[MAX_WIDE_PATH]{};

	if (bufferId > 0)
		sendNppMessage(NPPM_GETFULLPATHFROMBUFFERID, bufferId, &pathbuf[0]);
	else
		sendNppMessage(NPPM_GETFULLCURRENTPATH, (MAX_WIDE_PATH - 1ULL), &pathbuf[0]);

	result.assign(&pathbuf[0], std::wcslen(&pathbuf[0]));
	return path_t(result.cbegin(), result.cend(), path_t::format::native_format);
}
// --------------------------------------------------------------------------------------
path_t PluginBase::pluginNameFromModule(HMODULE hInstace) {
	_hModule = hInstace;
	const path_t modulePath = getModulePath(_hModule);
	return modulePath.stem().native();
}

// --------------------------------------------------------------------------------------
// SciApplication
// --------------------------------------------------------------------------------------
SciActiveDocument const &SciApplication::getDocument() const {
	intptr_t index = -1;
	sendMessage(NPPM_GETCURRENTSCINTILLA, UNUSEDW, &index);
	return getViews()[(index > 0)];
}
// --------------------------------------------------------------------------------------
void SciApplication::setApiLevel(SciApiLevel api) {
	SciViewList views = getViews();
	for (size_t i = 0; i < views.size(); i++)
		views[i].setApiLevel(api);
}

/////////////////////////////////////////////////////////////////////////////////////////
namespace {
path_t getModulePath(HMODULE hInstace) {
	uintptr_t iResult, iError, iSize = MAX_WIDE_PATH;
	std::wstring result(iSize, L'\0');

	do {
		::SetLastError(0);
		iResult = ::GetModuleFileNameW(hInstace, &result[0], static_cast<DWORD>(iSize));
		iError = ::GetLastError();

		if (iResult == 0) {
			if (iError == ERROR_SUCCESS || iError == ERROR_MOD_NOT_FOUND) {
				result.clear();
				break;
			}
		} else if (iResult >= iSize) {
			iSize = iResult + 1;
		} else {
			result.resize(iResult);
			break;
		}
	} while (iResult < iSize);

	if (result.substr(0, 4) == L"\\\\?\\") {
		result = result.substr(4);
	}

	return path_t(result.cbegin(), result.cend(), path_t::format::native_format);
}
}
