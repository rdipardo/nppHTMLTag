/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef PLUGIN_BASE_H
#define PLUGIN_BASE_H

#include <filesystem>
#include "PluginInterface.h"
#include "SciTextObjects.h"
#include "FuncArray.h"
#include "VersionInfo.h"

using namespace SciTextObjects;
using path_t = std::filesystem::path;

/// Manager of Scintilla edit views
class SciViewList;

/// Communication broker between the plugin and Scintilla
class SciApplication final : public SciWindowedObject {

public:
	static SciApplication *getApplication(const NppData *data = nullptr) {
		if (data && !_instance)
			_instance = new SciApplication(data);

		return _instance;
	}

	static SciApplication *getApplication(const NppData *data, SciApiLevel api) {
		SciApplication *app = SciApplication::getApplication(data);
		app->setApiLevel(api);
		return app;
	}

	SciApplication(const SciApplication &) = delete;
	SciApplication(SciApplication &&) = delete;
	void operator=(const SciApplication &) = delete;
	void operator=(SciApplication &&) = delete;

	void setApiLevel(SciApiLevel api) override;
	HWND const &windowHandle() const noexcept { return _windowHandle; }
	SciViewList const &getViews() const noexcept { return *_viewList; }
	SciActiveDocument const &activeDocument() const { return getDocument(); }

private:
	static inline SciApplication *_instance = nullptr;
	std::unique_ptr<SciViewList> _viewList = nullptr;
	SciActiveDocument const &getDocument() const;
	explicit SciApplication(const NppData *data)
	    : SciWindowedObject(data->_nppHandle),
	      _viewList(std::make_unique<SciViewList>(data)) {}
};

/// Default plugin implementation
class PluginBase {

public:
	explicit PluginBase() noexcept {}
	~PluginBase();
	PluginBase(const PluginBase &) = delete;
	PluginBase(PluginBase &&) = delete;
	PluginBase &operator=(const PluginBase &) = delete;
	PluginBase &operator=(PluginBase &&) = delete;

	virtual void setInfo(const NppData *data);
	virtual void beNotified(SCNotification *scn) = 0;

	/// @brief Default API call wrapper
	LRESULT sendNppMessage(const UINT msg, WPARAM wparam = UNUSEDW, LPARAM lparam = UNUSED) const;
	/// @brief Wraps API calls when @p lparam is a pointer
	LRESULT sendNppMessage(const UINT msg, WPARAM wparam, void *lparam = nullptr) const;
	bool openFile(wchar_t *filename) const;
	bool openFile(wchar_t *filename, Sci_Position line) const;
	path_t pluginsHomeDir() const;
	path_t pluginsConfigDir() const;
	LangType documentLangType() const;
	/// @brief @c true if N++ is v8.0 or later
	bool supportsDarkMode() const noexcept;
	/// @brief @c true if N++ is v8.3 or later
	bool supportsBigFiles() const noexcept;
	/// @brief @c true if N++ is v8.4 or later
	bool hasV5Apis() const noexcept;
	/// @brief @c true if N++ is v8.4.3 or later
	bool hasFullRangeApis() const noexcept;
	/// @brief @c true if N++ is v8.4.8 or later
	bool hasMinimalReplacementApi() const noexcept;
	/// @brief @c true if N++ is v8.5.4 or later
	bool supportsDarkModeSubclassing() const noexcept;
	/// @brief @c true if N++ is v8.6.1 or later
	bool hasMultiSelectionModeApis() const noexcept;
	/// @brief @c true if the dark mode setting can be detected by sending @c NPPM_ISDARKMODEENABLED
	bool isDarkModeEnabled() const;

	SciApplication &editor() const noexcept { return *_editor; }
	HINSTANCE instance() const noexcept { return reinterpret_cast<HINSTANCE>(_hModule); }
	Version const &nppVersion() const noexcept { return _nppVersion; }
	SciApiLevel apiLevel() const;
	HWND currentScintilla() const;

	FuncArray funcItems{};

protected:
	path_t pluginNameFromModule(HMODULE hInstace);

private:
	HMODULE _hModule;
	NppData _data;
	Version _nppVersion;
	SciApplication *_editor = nullptr;
};

// --------------------------------------------------------------------------------------
// SciViewList
// --------------------------------------------------------------------------------------
typedef std::vector<std::shared_ptr<SciActiveDocument>> ActiveDocuments;

class SciViewList final {

public:
	explicit SciViewList(const NppData *data) noexcept
	    : _views({
		  std::make_shared<SciActiveDocument>(data->_scintillaMainHandle),
		  std::make_shared<SciActiveDocument>(data->_scintillaSecondHandle),
	      }) {}

	~SciViewList() noexcept {
		for (size_t i = 0; i < _views.size(); i++)
			_views[i] = nullptr;
	}

	size_t size() const noexcept { return _views.size(); }
	SciActiveDocument &operator[](size_t index) const noexcept { return *_views[index]; }

private:
	ActiveDocuments _views;
};
#endif // ~PLUGIN_BASE_H
