/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include "HtmlTag.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

using namespace HtmlTag;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID /*lpReserved*/) {
	try {
		switch (dwReason) {
			case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
			{
				int dbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
				dbgFlags |= _CRTDBG_ALLOC_MEM_DF;
				dbgFlags |= _CRTDBG_LEAK_CHECK_DF;
				dbgFlags &= ~_CRTDBG_CHECK_CRT_DF;
				_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
				_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
				_CrtSetDbgFlag(dbgFlags);
			}
#endif
				plugin.initialize(hModule);
				break;

			case DLL_PROCESS_DETACH:
				break;
		}
	} catch (...) {
		return FALSE;
	}

	return TRUE;
}

#define PLUGINEXPORT extern "C" __declspec(dllexport)

PLUGINEXPORT void setInfo(NppData data) {
	plugin.setInfo(&data);
}

PLUGINEXPORT const wchar_t *getName() {
	return plugin.pluginMenuName;
}

PLUGINEXPORT FuncItem *getFuncsArray(int *nbF) {
	*nbF = plugin.funcItems.count();
	return &plugin.funcItems;
}

PLUGINEXPORT void beNotified(SCNotification *notifyCode) {
	plugin.beNotified(notifyCode);
}

PLUGINEXPORT LRESULT messageProc(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/) {
	return TRUE;
}

PLUGINEXPORT BOOL isUnicode() {
	return TRUE;
}
