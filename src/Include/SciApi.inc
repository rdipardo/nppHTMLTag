{$IFNDEF SCIAPI_INC}
{$DEFINE SCIAPI_INC}
{
  Copyright (c) 2022, 2023 Robert Di Pardo <dipardo.r@gmail.com>

  This Source Code Form is subject to the terms of the Mozilla Public License,
  v. 2.0. If a copy of the MPL was not distributed with this file, You can
  obtain one at http://mozilla.org/MPL/2.0/.
}
{$MINENUMSIZE 4}
type
    TSciApiLevel = (
        /// <= v4.4.6
        sciApi_LT_5,

        /// >= v5.1.5
        /// SCI_GETTEXT, SCI_GETSELTEXT and SCI_GETCURLINE return a string length that *does not* count the terminating NULL
        /// https://groups.google.com/g/scintilla-interest/c/DoRE5t2vihE
        sciApi_GTE_515,

        /// >= v5.2.3
        /// SCI_GETTEXTRANGEFULL, SCI_FINDTEXTFULL and SCI_FORMATRANGEFULL introduced
        /// https://groups.google.com/g/scintilla-interest/c/mPLwYdC0-FE
        sciApi_GTE_523,

        /// >= v5.3.2
        /// SCI_REPLACETARGETMINIMAL introduced
        /// https://groups.google.com/g/scintilla-interest/c/9OG2VdnWJ5I
        sciApi_GTE_532
    );

{$ENDIF ~SCIAPI_INC}
