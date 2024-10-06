/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include "FuncArray.h"

// --------------------------------------------------------------------------------------
// FuncArray
// --------------------------------------------------------------------------------------
FuncArray::~FuncArray() {
	for (size_t i = 0; i < _funcs.size(); i++) {
		if (_funcs[i]._pShKey != nullptr)
			delete (_funcs[i]._pShKey);
	}
}
// --------------------------------------------------------------------------------------
size_t FuncArray::add(const wchar_t *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool checkOnInit) {
	FuncItem item{};
	size_t index = _funcs.size();
	_funcs.push_back(item);
	wmemcpy(_funcs[index]._itemName, cmdName, menuItemSize - 1);
	_funcs[index]._pFunc = pFunc;
	_funcs[index]._pShKey = sk;
	_funcs[index]._init2Check = checkOnInit;
	return index;
}
