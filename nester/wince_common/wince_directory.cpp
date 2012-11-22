/*
** nester - NES emulator
** Copyright (C) 2000  Darren Ranalli
** Copyright (C) 2002  Y.Nagamidori
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/
/* $Id: wince_directory.cpp,v 1.2 2003/01/27 14:00:49 Rick Exp $ */

#include "nesterce.h"
#include "wince_directory.h"
#include "debug.h"

BOOL DIR_createDirectory(LPCTSTR directory)
{
	TCHAR sz[MAX_PATH] = _T("");
	
	LPTSTR prev, p;
	p = _tcschr(directory, _T('\\'));
	if (!p || p != directory) return FALSE; // not CE directory
	prev = p;
	while (p = _tcschr(prev + 1, _T('\\'))) {
		_tcsncat(sz, prev, p - prev);
		if (!CreateDirectory(sz, NULL)) {
			if (!(GetFileAttributes(sz) & FILE_ATTRIBUTE_DIRECTORY))
				return FALSE;
		}
		prev = p;
	}
	_tcscat(sz, prev);
	if (!CreateDirectory(sz, NULL)) {
		if (!(GetFileAttributes(sz) & FILE_ATTRIBUTE_DIRECTORY))
			return FALSE;
	}
	return TRUE;
}

void DIR_createFileName(char* buf, const char* basePath,
                        const char* relativePath,
                        const char* fileName, const char* fileExtension)
{
	TCHAR sz[MAX_PATH];
	char temp[MAX_PATH];

	sprintf(temp, "%s%s", basePath, relativePath);
	MultiByteToWideChar(CP_ACP, 0, temp, -1, sz, MAX_PATH);
	if (!DIR_createDirectory(sz))
		return;
	
	sprintf(buf, "%s%s\\%s%s", basePath, relativePath, fileName, fileExtension);
}

void DIR_createNesFileName(NES_ROM* rom, char* buf, NES_preferences_settings::SAVE_DIR_TYPE dirType,
                           const char* relativePath, const char* fileName,
                           const char* fileExtension)
{
	*buf = '\0';
	// currently support NES_preferences_settings::ROM_DIR only
	if (dirType == NES_preferences_settings::ROM_DIR) {
		const char* basepath = rom->GetRomPath();
		sprintf(buf, "%s%s%s", basepath, fileName, fileExtension);
	}
	else if (dirType == NES_preferences_settings::NESTER_DIR) {
		TCHAR sz[MAX_PATH];
		char basepath[MAX_PATH];
		GetModuleFileName(g_main_instance, sz, MAX_PATH);
		WideCharToMultiByte(CP_ACP, NULL, sz, -1, basepath, MAX_PATH, NULL, NULL);
		
		char* p = strrchr(basepath, '\\');
		if (p) *p = '\0';
		DIR_createFileName(buf, basepath, relativePath, fileName, fileExtension);
	}
}

