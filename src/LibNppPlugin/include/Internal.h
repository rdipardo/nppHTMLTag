/*
  This file is part of Notepad++ project
  Copyright (C)2023 Don HO <don.h@free.fr>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  at your option any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef SCINTILLA_USER

#include <windows.h>

/**
 * @defgroup NPP_INTERNAL
 * @brief Internal window messages used by Notepad++
 * @{
 * @def SCINTILLA_USER
 * @see https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/resource.h
 */
#define SCINTILLA_USER (WM_USER + 2000)
/**
 * @def WM_DOCK_USERDEFINE_DLG
 * @see
 * https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/ScintillaComponent/ScintillaEditView.h
 */
#define WM_DOCK_USERDEFINE_DLG (SCINTILLA_USER + 1)
#define WM_UNDOCK_USERDEFINE_DLG (SCINTILLA_USER + 2)	 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_CLOSE_USERDEFINE_DLG (SCINTILLA_USER + 3)	 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_REMOVE_USERLANG (SCINTILLA_USER + 4)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_RENAME_USERLANG (SCINTILLA_USER + 5)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_REPLACEALL_INOPENEDDOC (SCINTILLA_USER + 6)	 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FINDALL_INOPENEDDOC (SCINTILLA_USER + 7)	 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_DOOPEN (SCINTILLA_USER + 8)			 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FINDINFILES (SCINTILLA_USER + 9)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_REPLACEINFILES (SCINTILLA_USER + 10)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FINDALL_INCURRENTDOC (SCINTILLA_USER + 11)	 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FRSAVE_INT (SCINTILLA_USER + 12)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FRSAVE_STR (SCINTILLA_USER + 13)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FINDALL_INCURRENTFINDER (SCINTILLA_USER + 14) /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_FINDINPROJECTS (SCINTILLA_USER + 15)		 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
#define WM_REPLACEINPROJECTS (SCINTILLA_USER + 16)	 /**< @copydoc #WM_DOCK_USERDEFINE_DLG */
/** @} */
#endif /* ~SCINTILLA_USER */
