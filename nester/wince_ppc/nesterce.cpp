/*
	nester for PocketPC - NES emulator
	Copyright (C) 2000  Darren Ranalli
	Copyright (C) 2002  Y.Nagamidori (PocketPC)

    PocketNester
	Copyright (c) 2002  Rick Lei(PocketPC, PocketPC2002)
*/
/* $Id: nesterce.cpp,v 1.8 2003/10/28 12:55:16 Rick Exp $ */

#include <algorithm>
#include <vector>

using namespace std;

#include "resource.h"
#include "nesterce.h"
#include "settings.h"
#include "wince_emu.h"
#include "wince_dialogs.h"
#include "wince_directory.h"

#include "profile.h"
#include "DecmpImage.h"
#include "wince_OSD_pad.h"

#define WND_TITLE	_T("PocketNester")
#define WND_CLASS	_T("pnester")

#define POCKETNESTER_VER _T("0.7")

HINSTANCE g_main_instance = NULL;
HWND g_main_window = NULL;
HWND g_hwnd_cb = NULL;
DecmpImage * g_skin;

static HDC paused_dc;
static BOOL g_paused = FALSE;
static BOOL is_fullscreen = FALSE;
//static HDC skin_dc;
//static HBITMAP skin_bmp;

wince_emu* emu;
int savestate_slot = 0;

#define IDM_MUTE_SOUND 50000
#define SOUND_ENABLED_BMP 0
#define SOUND_MUTED_BMP 1

static TBBUTTON buttons[] = {
    {4+STD_FILEOPEN, IDM_FILE_OPEN,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
	{SOUND_ENABLED_BMP, IDM_MUTE_SOUND,   TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
    {0,              0,                   TBSTATE_ENABLED, TBSTYLE_SEP,    0, 0, 0, 0},
    {2,              IDM_FILE_QUICK_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
    {3,              IDM_FILE_QUICK_LOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
};

static TCHAR * button_tips[] = {
    NULL, //menu skipping
	NULL,
	NULL,
	TEXT("Open ROM"),
	TEXT("Mute Sound"),
    TEXT("Quick Save"),
    TEXT("Quick Load")
};

static UINT num_tips = 7;

HWND InitInstance(HINSTANCE hinst, int show);
void MainWinLoop(MSG& msg, HWND hwnd, HACCEL hAccel);
LRESULT MainWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void OnCommand(HWND hwnd, WPARAM wp, LPARAM lp);
HWND CreateCommandBar(HWND hwnd);
void OnFileOpen(HWND hwnd);
void OnFileROMInfo(HWND hwnd);
void OnSaveState(HWND hwnd);
void OnLoadState(HWND hwnd);
void OnQuickSave();
void OnQuickLoad();
void UpdateToolBar();
void SetFullScreen();

void LoadROM(LPTSTR name);
void FreeROM();

void init();
void shutdown();
void freeze();
void thaw();
void pause();
void resume();
void assert_priority();

///////////////////////////////////////////////////////
void LoadROM(LPTSTR name)
{
	char file[MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP, NULL, name, -1, file, MAX_PATH, NULL, NULL);

	BOOL saved_is_fullscreen = is_fullscreen;
	is_fullscreen = FALSE;	// do not switch to full screen at present
	resume();
	is_fullscreen = saved_is_fullscreen;

	if(emu)	{
		delete emu;
		emu = NULL;
	}

	PN_TRY {
		emu = new wince_emu(g_main_window, g_main_instance, file);

		// set the Open directory
		strcpy(NESTER_settings.OpenPath, emu->getROMpath());

		// assert the priority
		assert_priority();

		// update the palette
		//SendMessage(main_window_handle, WM_QUERYNEWPALETTE, 0, 0);
	}
	PN_CATCH {
#ifdef NESTERJ
		MessageBox(g_main_window, _T("ŠJ‚­‚±‚Æ‚ª‚Å‚«‚Ü‚¹‚ñB"), NULL, MB_ICONSTOP);
#else
		MessageBox(g_main_window, _T("Can not open."), NULL, MB_ICONSTOP);
#endif
	}
	UpdateToolBar();
	if (is_fullscreen) {
		SetFullScreen();
	}
}

void FreeROM()
{
	BOOL saved_is_fullscreen = is_fullscreen;
	is_fullscreen = FALSE;	// we are destroying emu, so do not switch to full screen.
	resume();
	is_fullscreen = saved_is_fullscreen;

	if(emu)
	{
		delete emu;
		emu = NULL;
	}
	RECT r;
	GetClientRect(g_main_window, &r);
	InvalidateRect(g_main_window, &r, TRUE);
	UpdateToolBar();
}

void init()
{
	emu = NULL;
	UpdateToolBar();
}

void shutdown()
{
	if (emu)
	{
		delete emu;
		emu = NULL;
	}
}

void freeze()
{
	if (emu)	{
		emu->freeze();
#ifdef PROFILING
		TCHAR buf[512];
		swprintf(buf, _T("cpu_time: %d %d %.3f\napu_time: %d %d %.3f\nppu_time: %d %d %.3f\nblt_time: %d %d %.3f"),
			cpu_time, cpu_called_times, (float)cpu_time / cpu_called_times,
			apu_time, apu_called_times, (float)apu_time / apu_called_times,
			ppu_time, ppu_called_times, (float)ppu_time / ppu_called_times,
			blt_time, blt_called_times, (float)blt_time / blt_called_times);
		MessageBox(g_main_window, buf, _T("PROFILE"), 0);
		RESET_PROFILE_DATA();
#endif
	}
}

void thaw()
{
	if (emu) {
		if (emu->thaw())	{
			assert_priority();
		}
	}
}

void pause()
{
	//if (!g_paused) {
	if (emu && !g_paused) {
		g_paused = TRUE;	// g_paused should not be set to TRUE if there is no emu. Rick.
		freeze();

		if (is_fullscreen) {
			// back from full screen
			// From STFullScreen & PocketSNES
			SetForegroundWindow(g_main_window);

			CommandBar_Show(g_hwnd_cb, TRUE);

			RECT rc;
			GetWindowRect(g_main_window, &rc);
			rc.top    = 26;
			rc.bottom = 320 - 26;
			MoveWindow(g_main_window, rc.left, rc.top, rc.right, rc.bottom, TRUE);
	
			SHFullScreen(g_main_window, SHFS_SHOWSIPBUTTON|SHFS_SHOWTASKBAR|SHFS_SHOWSTARTICON);
			
			//ShowWindow(g_hwnd_cb, SW_SHOW);
		}

		HBITMAP hbmp = emu->get_paused_bitmap();
		if (hbmp) {
			HDC	hdc = GetDC(g_main_window);
			paused_dc = CreateCompatibleDC(hdc);

			SelectObject(paused_dc, hbmp);
			ReleaseDC(g_main_window, hdc);
		}
	}
}

void resume()
{
	//if (g_paused) {
	if (emu && g_paused) { // Rick
		g_paused = FALSE;

		if (is_fullscreen) {
			SetFullScreen();
		} else {
			// erase "Game paused..." blahblah
			HDC hdc = GetDC(g_main_window);
			static RECT r = {0, 6 + 240, 240, 6 + 240 + 24};
			
			FillRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
			ReleaseDC(g_main_window, hdc);
		}
		if (paused_dc) {
			DeleteDC(paused_dc);
			paused_dc = NULL;
		}

		thaw();
	}
}

void SetFullScreen()
{
	/*
	// full screen. borrowed from PocketSNES :)
	ShowWindow(g_hwnd_cb, SW_HIDE);
	
	RECT rc;
	GetWindowRect(g_main_window, &rc);
	rc.top    = 0;
	rc.bottom = 320;
	SetWindowPos(g_main_window, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom, 0);
	
	if (SHFullScreen(g_main_window, SHFS_HIDESIPBUTTON|SHFS_HIDETASKBAR|SHFS_HIDESTARTICON))
	{
		SHSipPreference(g_main_window, SIP_FORCEDOWN);
	}
	
	SetForegroundWindow(g_main_window);
	*/
	// From STFullScreen & PocketSNES
	SHSipPreference(g_main_window, SIP_FORCEDOWN);
	SetForegroundWindow(g_main_window);

	RECT rc;
	GetWindowRect(g_main_window, &rc);
	rc.top    = 0;
	rc.bottom = 320;
	MoveWindow(g_main_window, rc.left, rc.top, rc.right, rc.bottom, FALSE);

	CommandBar_Show(g_hwnd_cb, FALSE);
	SHFullScreen(g_main_window, SHFS_HIDESIPBUTTON|SHFS_HIDETASKBAR|SHFS_HIDESTARTICON);

	// lazy loading of skin image
	HDC hdc = GetDC(g_main_window);
	PN_TRY {
		if (g_skin == NULL) {
			HRSRC hres = FindResource(g_main_instance, MAKEINTRESOURCE(IDR_PAD_PNG),_T("PNG"));
			DWORD rsize = SizeofResource(g_main_instance, hres);
			HGLOBAL hMem = ::LoadResource(g_main_instance, hres);
			if (hMem == NULL)
				THROW_EXCEPTION;
			
			void* lpVoid = LockResource(hMem);
			if (lpVoid == NULL)
				THROW_EXCEPTION;

			g_skin = new DecmpImage((BYTE *)lpVoid, rsize, hdc);
		}
	}
	PN_CATCH {
		MessageBox(g_main_window, _T("Error loading skin image"), NULL, MB_ICONSTOP);
		if (g_skin) {
			delete g_skin;
			g_skin = NULL;
		}
	}
	ReleaseDC(g_main_window, hdc);
}

void set_priority(DWORD priority)
{
	HANDLE thread = GetCurrentThread();
	SetThreadPriority(thread, priority);
}

void assert_priority()
{
	switch(NESTER_settings.nes.preferences.priority)
	{
	case NES_preferences_settings::PRI_NORMAL:
		set_priority(THREAD_PRIORITY_NORMAL);
		break;
	case NES_preferences_settings::PRI_HIGH:
		set_priority(THREAD_PRIORITY_ABOVE_NORMAL);
		break;
	case NES_preferences_settings::PRI_REALTIME:
		set_priority(THREAD_PRIORITY_HIGHEST);
		break;
	}
}

void LoadCmdLineROM(LPTSTR rom_name)
{
	if (!_tcslen(rom_name)) return;

	// are there quotes?
	if (rom_name[0] == _T('"')) {
		rom_name++;
		if (rom_name[_tcslen(rom_name) - 1] == _T('"')) {
			rom_name[_tcslen(rom_name) - 1] = _T('\0');
		}
	}

	if (!_tcslen(rom_name)) return;

	LoadROM(rom_name);
}

///////////////////////////////////////////////////////
int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	MSG      msg;
	HACCEL   hAccel;

	if (!(g_main_window = InitInstance(hInstance, nCmdShow)))
		return 0;
	
	//hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));
	hAccel = NULL;

	PN_TRY {
		LoadCmdLineROM(lpCmdLine);
		MainWinLoop(msg, g_main_window, hAccel);
		NESTER_settings.Save();
		if (emu) FreeROM();
	}
	PN_CATCH {
		errorlog("MainWinLoop error\n");
	}
	return 0;
}

HWND InitInstance(HINSTANCE hinst, int show)
{
	WNDCLASS  winclass; // window class we create
	HWND hwnd;
	RECT rc;

	g_main_instance = hinst;

	if (hwnd = FindWindow(WND_CLASS, WND_TITLE)) {
		SetForegroundWindow((HWND)(((DWORD) hwnd) | 0x01));
		return NULL;
	}

	PN_TRY {
		NESTER_settings.Load();
	}
	PN_CATCH {
		errorlog("NESTER_settings.Load() error\n");
	}
	// Rick
	is_fullscreen = NESTER_settings.nes.graphics.osd.gapi && NESTER_settings.nes.graphics.osd.fullscreen;

	InitCommonControls();

	winclass.style = CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = (WNDPROC) MainWindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = hinst;
	winclass.hIcon = NULL;
	winclass.hCursor = 0;
	winclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName = 0;
	winclass.lpszClassName = WND_CLASS;

	if (!RegisterClass(&winclass))
		return FALSE;
	
	if (!(hwnd = CreateWindow(WND_CLASS, WND_TITLE, WS_VISIBLE,
							 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							 CW_USEDEFAULT, NULL, NULL, hinst, NULL)))
		return 0;


	GetWindowRect(hwnd, &rc);
	rc.bottom -= MENU_HEIGHT;
	if (g_hwnd_cb)
		MoveWindow(hwnd, rc.left, rc.top, rc.right, rc.bottom, FALSE);

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	return hwnd;
}

void MainWinLoop(MSG& msg, HWND hwnd, HACCEL hAccel)
{
	while(TRUE) {
		if (emu && !emu->frozen()) {
			PN_TRY {
				emu->do_frame();
			}
			PN_CATCH {
				errorlog("emu->do_frame() error\n");
				FreeROM();
				MessageBox(hwnd, _T("emu->do_frame() error"), NULL, MB_ICONSTOP);
			}
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if(msg.message == WM_QUIT) return;

				if(!TranslateAccelerator(hwnd, hAccel, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		else {
			if(GetMessage(&msg, NULL, 0, 0)) {
				if(!TranslateAccelerator(hwnd, hAccel, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			else {
				return;
			}
		}
	}
}

HWND CreateCommandBar(HWND hwnd)
{
	SHMENUBARINFO info;
	memset(&info, 0, sizeof(SHMENUBARINFO));
	info.cbSize = sizeof(SHMENUBARINFO);
	info.hwndParent = hwnd;
	info.nToolBarId = IDR_MENU;
	info.hInstRes = g_main_instance;
	info.nBmpId = 0;
	info.cBmpImages = 0;
	if (!SHCreateMenuBar(&info)) 
		return NULL;

    CommandBar_AddToolTips(info.hwndMB, num_tips, button_tips);
    CommandBar_AddBitmap(info.hwndMB, g_main_instance, IDR_TOOLBAR, 2, 16, 16);

	// add system-defined button bitmaps
    CommandBar_AddBitmap(info.hwndMB, HINST_COMMCTRL, IDB_STD_SMALL_COLOR, 15, 16, 16);

    CommandBar_AddButtons(info.hwndMB, sizeof(buttons)/sizeof(TBBUTTON), buttons);

#ifndef _USES_GAPI_DISPLAY
	HMENU hMenu = (HMENU)SendMessage(info.hwndMB, SHCMBM_GETMENU, 0, 0);
	DeleteMenu(hMenu, IDM_OPTIONS_FULLSCREEN, MF_BYCOMMAND);
#endif

	return info.hwndMB;
}

#define EnableToolButton(id, enable) \
	::SendMessage(g_hwnd_cb, TB_ENABLEBUTTON, id, MAKELPARAM((enable), 0))

void UpdateToolBar()
{
	if (g_hwnd_cb) {
		BOOL gameLoaded = (emu != NULL) ? TRUE : FALSE;

		EnableToolButton(IDM_MUTE_SOUND, gameLoaded);

		if (emu) {
			BOOL enabled = NESTER_settings.nes.sound.enabled;
			::SendMessage(g_hwnd_cb, TB_CHANGEBITMAP, IDM_MUTE_SOUND,
				MAKELPARAM(enabled ? SOUND_ENABLED_BMP : SOUND_MUTED_BMP, 0));
			::SendMessage(g_hwnd_cb, TB_CHECKBUTTON, IDM_MUTE_SOUND, MAKELPARAM(!enabled, 0));
		}

		EnableToolButton(IDM_FILE_QUICK_SAVE, gameLoaded);
		EnableToolButton(IDM_FILE_QUICK_LOAD, gameLoaded);
	}
}

void ProcessOsdPadEvent(UINT msg, UINT x, UINT y)
{
	if (g_skin == NULL)
		return;

	int32 color = -1;
	if (y >= 240 && (msg == WM_LBUTTONDOWN || msg == WM_MOUSEMOVE)) {
		uint8 * bits = g_skin->GetBits();
		//bits += 3 * 240 * (y - 240 + 80) + x * 3;
		//color = (bits[0] << 16) | (bits[1] << 8) | bits[2];
		bits += 3 * 240 * (79 - (y - 240)) + x * 3;
		color = (bits[2] << 16) | (bits[1] << 8) | bits[0];
	}

	wince_OSD_pad *pad = emu->get_osd_pad();
	
	switch (msg) {
	case WM_LBUTTONDOWN:
		pad->OnStylusDown(color);
		break;

	case WM_MOUSEMOVE:
		pad->OnStylusMove(color);
		break;

	case WM_LBUTTONUP:
		pad->OnStylusUp(-1);
		break;
	}
}

typedef vector<KeyListener *> ListenerArray;
static ListenerArray listeners;

void InitListeners()
{
    listeners.clear();
}

void RegisterKeyListener(KeyListener *listener)
{
    listeners.push_back(listener);
}

void UnregisterKeyListener(KeyListener *listener)
{
    ListenerArray::iterator it = find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end()) {
        listeners.erase(it);
    }
}

LRESULT MainWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_CREATE:
			g_hwnd_cb = CreateCommandBar(hwnd);
			init();
			break;
		case WM_COMMAND:
			OnCommand(hwnd, wp, lp);
			break;
		case WM_INITMENUPOPUP:
		{
			pause();
			
			UINT flag = emu ? MF_ENABLED | MF_BYCOMMAND : MF_GRAYED | MF_BYCOMMAND;
			flag |= MF_BYCOMMAND;
			EnableMenuItem((HMENU)wp, IDM_FILE_CLOSE,  flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_RESET,      flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_RESUME,     flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_ROMINFO,    flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_LOAD_STATE, flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_SAVE_STATE, flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_QUICK_LOAD, flag);
			EnableMenuItem((HMENU)wp, IDM_FILE_QUICK_SAVE, flag);
			CheckMenuRadioItem((HMENU)wp, IDM_FILE_SLOT_0, IDM_FILE_SLOT_9, IDM_FILE_SLOT_0 + savestate_slot, MF_BYCOMMAND);

#ifdef _USES_GAPI_DISPLAY
			flag = NESTER_settings.nes.graphics.osd.gapi ? MF_ENABLED  | MF_BYCOMMAND : MF_GRAYED | MF_BYCOMMAND;
			//flag = MF_ENABLED  | MF_BYCOMMAND;
			EnableMenuItem((HMENU)wp, IDM_OPTIONS_FULLSCREEN, flag);

			flag = NESTER_settings.nes.graphics.osd.fullscreen ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_FULLSCREEN, flag);
#endif
			flag = NESTER_settings.nes.input.player1.btnA.turbo ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_TURBO_A, flag);
			flag = NESTER_settings.nes.input.player1.btnB.turbo ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_TURBO_B, flag);

			// Rick
			flag = NESTER_settings.nes.preferences.force_pal ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_FORCEPAL, flag);

			break;
		}
		case WM_LBUTTONDOWN:
			if (g_paused) {
				resume();
			} else {
				UINT xPos = LOWORD(lp); 
				UINT yPos = HIWORD(lp);

				if (emu && is_fullscreen && yPos >= 240) {
					ProcessOsdPadEvent(msg, xPos, yPos);
				} else {
					pause();
				}
			}
			break;
		case WM_MOUSEMOVE:
		case WM_LBUTTONUP:
			if (emu && is_fullscreen) {
				UINT xPos = LOWORD(lp); 
				UINT yPos = HIWORD(lp);
				ProcessOsdPadEvent(msg, xPos, yPos);
			}
			break;
		case WM_KILLFOCUS:		// Rick
			if (wp == 0) {
			// some dirty workaround. Windoz send WM_KILLFOCUS not only on app's losing focus,
			// but also on app's own focus switching(memu close, dialog, etc). It seems like
			// that wp is NULL when losing focus, and not NULL on the other hand.
				pause();
			}
			break;
		case WM_ENTERMENULOOP:
			pause();
			break;
		case WM_DESTROY:
			CommandBar_Destroy(g_hwnd_cb);
			shutdown();
			PostQuitMessage(0);
			break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            if (!g_paused) {
                for (ListenerArray::iterator it = listeners.begin(); it != listeners.end(); it++) {
                    KeyListener *l = (*it);
                    if (l->getKeyCode() == wp) {
                        l->keyEvent((msg == WM_KEYDOWN) ? 1 : 0);
                    }
                }
                break;
            }
        }

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
/*
			BeginPaint(hwnd, &ps);
#ifdef _USES_GAPI_DISPLAY
			if (emu && !NESTER_settings.nes.graphics.osd.gapi)
				emu->blt();
#else
			if (emu)
				emu->blt();
#endif
*/
			HDC hdc = BeginPaint(hwnd, &ps);
			if (emu) {
				if (g_paused) {
					static TCHAR paused[] = _T("Game paused. Tap screen to resume...");
					static RECT r = {0, 6 + 240, 240, 6 + 240 + 24};
					
					BitBlt(hdc, 0, 6, 240, 6 + 240, paused_dc, 8, 0, SRCCOPY);
					
					SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
					SetBkColor(hdc, RGB(0, 0, 0));
					DrawText(hdc, paused, -1, &r, DT_CENTER|DT_SINGLELINE|DT_VCENTER);
				}
			} else {
				//static TCHAR welcome[] = _T("PocketNester ") POCKETNESTER_VER _T("\n\nhttp://jetech.org\n\nChoose \"Open ROM...\" from File menu to load game");
				static TCHAR welcome[] = _T("PocketNester ") POCKETNESTER_VER _T(" Build ") _T(__DATE__);
				static RECT r = {0, 26+140, 240, 26 + 240};
				
				SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
				SetBkColor(hdc, RGB(0, 0, 0));
				//DrawText(hdc, welcome, -1, &r, DT_CENTER | DT_WORDBREAK);
				DrawText(hdc, welcome, -1, &r, DT_RIGHT | DT_SINGLELINE | DT_BOTTOM);
			}
			EndPaint(hwnd, &ps);
			break;
		}
		default:
			return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

void OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
	switch (LOWORD(wp)) {
		case IDM_FILE_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDM_FILE_OPEN:
			pause();	// for toolbar shortcut
			OnFileOpen(hwnd);
			break;
		case IDM_FILE_CLOSE:
			FreeROM();
			break;
		case IDM_FILE_RESUME:
			resume();
			break;
		case IDM_FILE_RESET:
			resume();
			emu->reset();
			break;
		case IDM_FILE_ROMINFO:
			OnFileROMInfo(hwnd);
			break;
		case IDM_FILE_SAVE_STATE:
			OnSaveState(hwnd);
			break;
		case IDM_FILE_LOAD_STATE:
			OnLoadState(hwnd);
			break;
		case IDM_FILE_QUICK_SAVE:
			OnQuickSave();
			break;
		case IDM_FILE_QUICK_LOAD:
			OnQuickLoad();
			break;
		case IDM_MUTE_SOUND:
			if (emu) {
				BOOL enabled = !NESTER_settings.nes.sound.enabled;
				NESTER_settings.nes.sound.enabled = enabled;
				emu->enable_sound(enabled);
				UpdateToolBar();
			}
			break;

		case IDM_FILE_SLOT_0:
		case IDM_FILE_SLOT_1:
		case IDM_FILE_SLOT_2:
		case IDM_FILE_SLOT_3:
		case IDM_FILE_SLOT_4:
		case IDM_FILE_SLOT_5:
		case IDM_FILE_SLOT_6:
		case IDM_FILE_SLOT_7:
		case IDM_FILE_SLOT_8:
		case IDM_FILE_SLOT_9:
			savestate_slot = LOWORD(wp) - IDM_FILE_SLOT_0;
			break;
		case IDM_OPTIONS_PREFERENCES:
			DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_OPTIONS_PREFERENCES), hwnd, PreferencesOptions_DlgProc);
			break;
		case IDM_OPTIONS_GRAPHICS:
			if (DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_OPTIONS_GRAPHICS), hwnd, GraphicsOptions_DlgProc) == IDOK) {
				if (emu)
					emu->assert_palette();
				//Rick
				if (!NESTER_settings.nes.graphics.osd.gapi && NESTER_settings.nes.graphics.osd.fullscreen) {
					MessageBox(g_main_window, _T("You had disabled GAPI, which is required by Full Screen mode. Full Screen will be disabled now."), _T("Warning"), MB_ICONWARNING | MB_OK);
				}
				is_fullscreen = NESTER_settings.nes.graphics.osd.gapi && NESTER_settings.nes.graphics.osd.fullscreen;
			}
			break;
		case IDM_OPTIONS_SOUND:
			if (DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_OPTIONS_SOUND), hwnd, SoundOptions_DlgProc) == IDOK) {
				if (emu)
					emu->enable_sound(NESTER_settings.nes.sound.enabled);
			}
			break;
		case IDM_OPTIONS_CONTROLLERS:
			if (DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_OPTIONS_CONTROLLERS), hwnd, ControllersOptions_DlgProc) == IDOK) {
				if (emu)
					emu->input_settings_changed();
			}
			break;
		case IDM_OPTIONS_DIRECTORY:
			DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_OPTIONS_DIRECTORY), hwnd, DirectoryOptions_DlgProc);
			break;
		case IDM_OPTIONS_FULLSCREEN:
			NESTER_settings.nes.graphics.osd.fullscreen = !NESTER_settings.nes.graphics.osd.fullscreen;
			is_fullscreen = NESTER_settings.nes.graphics.osd.gapi && NESTER_settings.nes.graphics.osd.fullscreen;
			break;
		// Rick
		case IDM_OPTIONS_FORCEPAL:
			NESTER_settings.nes.preferences.force_pal = !NESTER_settings.nes.preferences.force_pal;
			break;
		case IDM_OPTIONS_TURBO_A:
			NESTER_settings.nes.input.player1.btnA.turbo = !NESTER_settings.nes.input.player1.btnA.turbo;
			if (emu)
				emu->input_settings_changed();
			break;
		case IDM_OPTIONS_TURBO_B:
			NESTER_settings.nes.input.player1.btnB.turbo = !NESTER_settings.nes.input.player1.btnB.turbo;
			if (emu)
				emu->input_settings_changed();
			break;
		case IDM_HELP_ABOUT:
			DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_HELP_ABOUT), hwnd, AboutNester_DlgProc);
			break;
	}
}

void OnFileOpen(HWND hwnd)
{
	OPENFILENAME ofn;
	TCHAR name[MAX_PATH] = _T("");
	
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner = hwnd;
#ifdef NESTERJ
	ofn.lpstrTitle = _T("ROM ‚ðŠJ‚­...");
	ofn.lpstrFilter = _T("NES (*.nes)\0*.nes\0‚·‚×‚Ä‚ÌÌ§²Ù (*.*)\0*.*\0");
#else
	ofn.lpstrTitle = _T("Open ROM...");
	ofn.lpstrFilter = _T("NES (*.nes;*.zip)\0*.nes;*.zip\0All Files (*.*)\0*.*\0");
#endif
	ofn.lpstrFile = name;
	ofn.nMaxFile = MAX_PATH;	
	ofn.Flags = OFN_EXPLORER;

	if (!GetOpenFileName(&ofn))
		return;

	LoadROM(name);
}

void OnFileROMInfo(HWND hwnd)
{
	TCHAR temp[512], name[MAX_PATH], mirroring[32];

	if (!emu)
		return;

	NES_ROM* rom_info = emu->get_NES_ROM();;
	if (!rom_info)
		return;

	// name
	MultiByteToWideChar(CP_ACP, 0, rom_info->GetRomNameExt(), -1, name, MAX_PATH);

	// mirroring
	switch(rom_info->get_mirroring())
	{
	case NES_PPU::MIRROR_FOUR_SCREEN:
		_tcscpy(mirroring, _T("four-screen"));
		break;
	case NES_PPU::MIRROR_VERT:
		_tcscpy(mirroring, _T("vertical"));
		break;
	case NES_PPU::MIRROR_HORIZ:
		_tcscpy(mirroring, _T("horizontal"));
		break;
	}

#ifdef NESTERJ
	wsprintf(temp, _T("Ì§²Ù–¼:\t%s\nÏ¯Êß° #:\t%d\nÐ×°ØÝ¸Þ:\t%s\nROM »²½Þ:\t%dk\nVROM »²½Þ:\t%dk\nÄÚ²Å°:\t%s\n¾°ÌÞ RAM:\t%s"),
		name, rom_info->get_mapper_num(), mirroring, rom_info->get_num_16k_ROM_banks() * 16, 
		rom_info->get_num_8k_VROM_banks() * 8, rom_info->has_trainer() ? _T("‚ ‚è") : _T("‚È‚µ"),
		rom_info->has_save_RAM() ? _T("‚ ‚è") : _T("‚È‚µ"));

	MessageBox(hwnd, temp, _T("ROM î•ñ"), MB_OK);
#else
	wsprintf(temp, _T("filename:\t%s\nmapper #:\t%d\nmirroring:\t%s\nROM size:\t%dk\nVROM size:\t%dk\nhas trainer:\t%s\nhas save RAM:\t%s"),
		name, rom_info->get_mapper_num(), mirroring, rom_info->get_num_16k_ROM_banks() * 16, 
		rom_info->get_num_8k_VROM_banks() * 8, rom_info->has_trainer() ? _T("yes") : _T("no"),
		rom_info->has_save_RAM() ? _T("yes") : _T("no"));

	MessageBox(hwnd, temp, _T("ROM info"), MB_OK);
#endif
}

void MakeSaveStateFilename(char* buf)
{
	char extension[5];

	if (emu)	{
		sprintf(extension, ".ss%i", savestate_slot);
		DIR_createNesFileName(emu->get_NES_ROM(), buf,
								NESTER_settings.nes.preferences.saveStateDirType,
								NESTER_settings.nes.preferences.saveStateDir,
								emu->getROMname(), extension);
		/*
		strcpy(buf, emu->getROMpath());
		strcat(buf, emu->getROMname());
		sprintf(extension, ".ss%i", savestate_slot);
		strcat(buf, extension);
		*/
	}
	else {
		strcpy(buf, "");
	}
}

void MakeShortSaveStateFilename(char* buf)
{
	char extension[5];

	if (emu) {
		strcpy(buf, emu->getROMname());
		sprintf(extension, ".ss%i", savestate_slot);
		strcat(buf, extension);
	}
	else {
		strcpy(buf, "");
	}
}

void OnSaveState(HWND hwnd)
{
	if (!emu) return;

	char savestate_filename[MAX_PATH] = "";
	TCHAR savestate_filename2[MAX_PATH] = _T("");
	TCHAR filter[MAX_PATH] = _T("");
	MakeShortSaveStateFilename(savestate_filename);
	MultiByteToWideChar(CP_ACP, 0, savestate_filename, -1, savestate_filename2, MAX_PATH);
	
	OPENFILENAME ofn;
	memset(&(ofn), 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner   = hwnd;
	ofn.lpstrFile   = savestate_filename2;
	ofn.nMaxFile    = MAX_PATH;
#ifdef NESTERJ
	ofn.lpstrTitle  = _T("State •Û‘¶...");
	ofn.lpstrFilter = _T("save file (*.ss0)\0*.ss0\0save file (*.ss1)\0*.ss1\0save file (*.ss2)\0*.ss2\0save file (*.ss3)\0*.ss3\0save file (*.ss4)\0*.ss4\0save file (*.ss5)\0*.ss5\0save file (*.ss6)\0*.ss6\0save file (*.ss7)\0*.ss7\0save file (*.ss8)\0*.ss8\0save file (*.ss9)\0*.ss9\0‚·‚×‚Ä‚ÌÌ§²Ù (*.*)\0*.*\0\0");
#else
	ofn.lpstrTitle  = _T("Save state...");
	ofn.lpstrFilter = _T("save file (*.ss0)\0*.ss0\0save file (*.ss1)\0*.ss1\0save file (*.ss2)\0*.ss2\0save file (*.ss3)\0*.ss3\0save file (*.ss4)\0*.ss4\0save file (*.ss5)\0*.ss5\0save file (*.ss6)\0*.ss6\0save file (*.ss7)\0*.ss7\0save file (*.ss8)\0*.ss8\0save file (*.ss9)\0*.ss9\0All Files (*.*)\0*.*\0\0");
#endif
	ofn.nFilterIndex = savestate_slot;
	ofn.Flags       = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    if (GetSaveFileName(&ofn)) {
		WideCharToMultiByte(CP_ACP, NULL, savestate_filename2, -1, savestate_filename, MAX_PATH, NULL, NULL);
		emu->saveState(savestate_filename);
		resume();
	}
}

void OnLoadState(HWND hwnd)
{
	if (!emu) return;

	char savestate_filename[MAX_PATH];
	TCHAR savestate_filename2[MAX_PATH];
	MakeShortSaveStateFilename(savestate_filename);
	MultiByteToWideChar(CP_ACP, 0, savestate_filename, -1, savestate_filename2, MAX_PATH);

	OPENFILENAME ofn;
	memset(&(ofn), 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner   = hwnd;
	ofn.lpstrFile   = savestate_filename2;
	ofn.nMaxFile    = MAX_PATH;	
#ifdef NESTERJ
	ofn.lpstrTitle  = _T("State“Ç‚Ýž‚Ý...");
	ofn.lpstrFilter = _T("save state (*.ss*)\0*.ss0;*.ss1;*.ss2;*.ss3;*.ss4;*.ss5;*.ss6;*.ss7;*.ss8;*.ss9\0‚·‚×‚Ä‚ÌÌ§²Ù (*.*)\0*.*\0\0");
#else
	ofn.lpstrTitle  = _T("Load state...");
	ofn.lpstrFilter = _T("save state (*.ss*)\0*.ss0;*.ss1;*.ss2;*.ss3;*.ss4;*.ss5;*.ss6;*.ss7;*.ss8;*.ss9\0All Files (*.*)\0*.*\0\0");
#endif
	ofn.Flags       = OFN_EXPLORER | OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn)) {
		WideCharToMultiByte(CP_ACP, NULL, savestate_filename2, -1, savestate_filename, MAX_PATH, NULL, NULL);
		emu->loadState(savestate_filename);
		resume();
	}
}

void OnQuickSave()
{
	if (!emu) return;

	char savestate_filename[_MAX_PATH];
    MakeSaveStateFilename(savestate_filename);
	resume();
    emu->saveState(savestate_filename);
}

void OnQuickLoad()
{
	if (!emu) return;

	char savestate_filename[_MAX_PATH];
	MakeSaveStateFilename(savestate_filename);
	resume();
	emu->loadState(savestate_filename);
}
