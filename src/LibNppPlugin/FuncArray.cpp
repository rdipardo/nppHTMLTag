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
	while (!_keyStore.empty())
		_keyStore.pop_back();
}
// --------------------------------------------------------------------------------------
size_t FuncArray::add(
    const wchar_t *cmdName, PFUNCPLUGINCMD pFunc, std::shared_ptr<ShortcutKey> const &sk, bool checkOnInit) {
	FuncItem item{};
	size_t index = _funcs.size();
	_funcs.push_back(item);
	_keyStore.push_back(sk);
	wmemcpy(_funcs[index]._itemName, cmdName, menuItemSize - 1);
	_funcs[index]._pFunc = pFunc;
	_funcs[index]._pShKey = _keyStore.back().get();
	_funcs[index]._init2Check = checkOnInit;
	return index;
}
