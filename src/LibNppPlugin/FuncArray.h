/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#ifndef FUNC_ARRAY_H
#define FUNC_ARRAY_H

#include "PluginInterface.h"

/// Manager of FuncItem objects
class FuncArray final {
public:
	explicit FuncArray() noexcept {};
	~FuncArray();
	FuncArray(const FuncArray &) = delete;
	FuncArray(FuncArray &&) = delete;
	void operator=(const FuncArray &) = delete;
	void operator=(FuncArray &&) = delete;

	operator bool() const noexcept { return !_funcs.empty(); }
	FuncItem *operator&() const noexcept { return &_funcs[0]; }
	FuncItem &operator[](size_t index) const noexcept { return _funcs[index]; }

	int count() const noexcept { return static_cast<int>(_funcs.size()); }
	int getItemCmdId(size_t index) const noexcept { return (index < _funcs.size()) ? _funcs[index]._cmdID : -1; }

	/// Initializes a plugin command.
	/// @param cmdName The command name that you want to see in plugin menu
	/// @param pFunc The symbol of function (function pointer) associated with this command
	/// @param sk Define a shortcut to trigger this command
	/// @param checkOnInit Make this menu item be checked visually
	/// @return The new size of this @c FuncArray
	size_t add(const wchar_t *cmdName, PFUNCPLUGINCMD pFunc = nullptr, ShortcutKey *sk = nullptr,
	    bool checkOnInit = false);

private:
	mutable std::vector<FuncItem> _funcs;
};
#endif // ~FUNC_ARRAY_H
