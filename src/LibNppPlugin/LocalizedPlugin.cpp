/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.

  Copyright (c) 2024 Robert Di Pardo <dipardo.r@gmail.com>
*/
#include <cstdio>
#include <share.h>
#include <tinyxml2.h>
#include "LocalizedPlugin.h"

// --------------------------------------------------------------------------------------
// LocalizedPlugin
// --------------------------------------------------------------------------------------
void LocalizedPlugin::setLanguage() {
	// Always make sure that 'nativeLang.xml' exists
	// https://github.com/notepad-plus-plus/notepad-plus-plus/commit/ea08a89
	if (!std::filesystem::exists(getNativeLangFile()))
		return;

	if (supportsLocalizedPluginMenus()) {
		intptr_t fnameLen = sendNppMessage(NPPM_GETNATIVELANGFILENAME);
		std::string fname(++fnameLen, 0);
		sendNppMessage(NPPM_GETNATIVELANGFILENAME, fnameLen, &fname[0]);
		_nativeLangId = path_t(fname).stem().string();
		return;
	}

	FILE *xml = _wfsopen(getNativeLangFile().c_str(), L"rb", _SH_DENYNO);
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(xml) != tinyxml2::XML_SUCCESS) {
		fclose(xml);
		return;
	}

	tinyxml2::XMLElement *root = doc.FirstChildElement();
	while (root && !root->FirstAttribute())
		root = root->FirstChildElement();
	if (root && root->Attribute("filename"))
		_nativeLangId = path_t(root->Attribute("filename")).stem().string();
	fclose(xml);
}
// --------------------------------------------------------------------------------------
path_t LocalizedPlugin::getNativeLangFile() const {
	path_t path;
	if (editor().windowHandle() != 0)
		path = pluginsHomeDir().parent_path() / path_t(L"nativeLang.xml");
	return path;
}
// --------------------------------------------------------------------------------------
bool LocalizedPlugin::supportsLocalizedPluginMenus() const noexcept {
	const Version target{ 8, 7 };
	return nppVersion() >= target;
}
