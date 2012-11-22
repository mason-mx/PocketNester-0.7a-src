#ifndef _NESTERCE_H_
#define _NESTERCE_H_

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <sipapi.h>
#include <aygshell.h>

#if defined(_USES_GAPI_DISPLAY) || defined(_USES_GAPI_INPUT)
#include "gx.h"
#endif

#define MENU_HEIGHT 26

extern HINSTANCE g_main_instance;

#endif // _NESTERCE_H_
