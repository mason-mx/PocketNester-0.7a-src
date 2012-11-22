/*
	nester for PocketPC - NES emulator
	Copyright (C) 2000  Darren Ranalli
	Copyright (C) 2002  Y.Nagamidori (PocketPC)
*/

#include "resource.h"
#include "nesterce.h"
#include "settings.h"
#include "wince_emu.h"
#include "wince_dialogs.h"
#include "wince_directory.h"

#define WND_TITLE	_T("nester for HPC")
#define WND_CLASS	_T("nester")

HINSTANCE g_main_instance = NULL;
HWND g_main_window = NULL;
HWND g_hwnd_cb = NULL;
BOOL g_paused = FALSE;

wince_emu* emu;
int savestate_slot = 0;

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
void FullScreen(BOOL enable)
{
	if (!NESTER_settings.nes.graphics.osd.hide_taskbar)
		return;

	if (enable) {
		ShowWindow(g_hwnd_cb, SW_HIDE);
		InvalidateRect(g_main_window, NULL, TRUE);

		HWND hwndTB = FindWindow(_T("HHTaskBar"), _T(""));
		if (hwndTB)
			ShowWindow(hwndTB, SW_HIDE);

		RECT rc;
		GetWindowRect(g_main_window, &rc);
		SetWindowPos(g_main_window, HWND_TOPMOST, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top + 26, SWP_NOMOVE);
	}
	else {
		ShowWindow(g_hwnd_cb, SW_SHOW);
		InvalidateRect(g_main_window, NULL, TRUE);
		HWND hwndTB = FindWindow(_T("HHTaskBar"), _T(""));
		if (hwndTB)
			ShowWindow(hwndTB, SW_SHOW);
		
		RECT rc;
		GetWindowRect(g_main_window, &rc);
		SetWindowPos(g_main_window, HWND_NOTOPMOST, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top - 26, SWP_NOMOVE);
	}
}

///////////////////////////////////////////////////////
void LoadROM(LPTSTR name)
{
	char file[MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP, NULL, name, -1, file, MAX_PATH, NULL, NULL);

	resume();

	if(emu)	{
		delete emu;
		emu = NULL;
	}

	__try {
		emu = new wince_emu(g_main_window, g_main_instance, file);

		// set the Open directory
		strcpy(NESTER_settings.OpenPath, emu->getROMpath());

		// assert the priority
		assert_priority();

		// update the palette
		//SendMessage(main_window_handle, WM_QUERYNEWPALETTE, 0, 0);

		// fullScreen
		FullScreen(TRUE);
		g_paused = FALSE;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
#ifdef NESTERJ
		MessageBox(g_main_window, _T("開くことができません。"), NULL, MB_ICONSTOP);
#else
		MessageBox(g_main_window, _T("Can not open."), NULL, MB_ICONSTOP);
#endif
	}
}

void FreeROM()
{
	resume();

	if(emu)
	{
		delete emu;
		emu = NULL;
	}
	g_paused = FALSE;
	FullScreen(FALSE);
}

void init()
{
	emu = NULL;
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
		FullScreen(FALSE);
		emu->freeze();
	}
}

void thaw()
{
	if (emu) {
		FullScreen(TRUE);
		if (emu->thaw())	{
			assert_priority();
		}
	}
}


void pause()
{
	if (!g_paused) {
		g_paused = TRUE;
		freeze();
	}
}

void resume()
{
	if (g_paused) {
		g_paused = FALSE;
		thaw();
	}
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
	
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));
	__try {
		LoadCmdLineROM(lpCmdLine);
		MainWinLoop(msg, g_main_window, hAccel);
		NESTER_settings.Save();
		if (emu) FreeROM();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		errorlog("MainWinLoop error\n");
	}
	return 0;
}

HWND InitInstance(HINSTANCE hinst, int show)
{
	WNDCLASS  winclass; // window class we create
	HWND hwnd;

	g_main_instance = hinst;

	if (hwnd = FindWindow(WND_CLASS, WND_TITLE)) {
		SetForegroundWindow((HWND)(((DWORD) hwnd) | 0x01));
		return NULL;
	}
	
	__try {
		NESTER_settings.Load();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		errorlog("NESTER_settings.Load() error\n");
	}

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

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	return hwnd;
}

void MainWinLoop(MSG& msg, HWND hwnd, HACCEL hAccel)
{
	while(TRUE) {
		if (emu && !emu->frozen()) {
			__try {
				emu->do_frame();
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				errorlog("emu->do_frame() error\n");
				FreeROM();
				MessageBox(hwnd, _T("エミュレーションエラー"), NULL, MB_ICONSTOP);
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
	HWND hwndCB = CommandBar_Create(g_main_instance, hwnd, IDR_MENU);
	CommandBar_InsertMenubar(hwndCB, g_main_instance, IDR_MENU, 0);
	CommandBar_AddAdornments(hwndCB, 0, 0);
	return hwndCB;
}

LRESULT MainWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_CREATE:
			g_hwnd_cb = CreateCommandBar(hwnd);
			SendMessage(hwnd, WM_SETICON , ICON_BIG, 
				(LPARAM)LoadImage(g_main_instance, MAKEINTRESOURCE(IDI_NESTERICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
			SendMessage(hwnd, WM_SETICON , ICON_SMALL, 
				(LPARAM)LoadImage(g_main_instance, MAKEINTRESOURCE(IDI_NESTERICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
			init();
			break;
		case WM_COMMAND:
			pause();
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

			flag = NESTER_settings.nes.graphics.osd.hide_taskbar ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_HIDE_TASKBAR, flag);
			flag = NESTER_settings.nes.input.player1.btnA.turbo ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_TURBO_A, flag);
			flag = NESTER_settings.nes.input.player1.btnB.turbo ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND;
			CheckMenuItem((HMENU)wp, IDM_OPTIONS_TURBO_B, flag);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_ENTERMENULOOP:
			pause();
			break;
		case WM_DESTROY:
			CommandBar_Destroy(g_hwnd_cb);
			shutdown();
			PostQuitMessage(0);
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			if (emu)
				emu->blt();
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_ACTIVATE:
		{
			if (!NESTER_settings.nes.preferences.run_in_background) {
				if (LOWORD(wp) == WA_INACTIVE)
					pause();
			}
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
		case IDM_OPTIONS_HIDE_TASKBAR:
			NESTER_settings.nes.graphics.osd.hide_taskbar = !NESTER_settings.nes.graphics.osd.hide_taskbar;
			break;
		case IDM_HELP_ABOUT:
			DialogBox(g_main_instance, MAKEINTRESOURCE(IDD_HELP_ABOUT), hwnd, AboutNester_DlgProc);
			break;
		case ID_KEY_PAUSE:
			pause();
			break;
	}
}

void OnFileOpen(HWND hwnd)
{
	OPENFILENAME ofn;
	TCHAR name[MAX_PATH] = _T("");
	TCHAR init_path[MAX_PATH] = _T("");

	MultiByteToWideChar(CP_ACP, 0, NESTER_settings.OpenPath, -1, init_path, MAX_PATH);
	
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner = hwnd;
#ifdef NESTERJ
	ofn.lpstrTitle = _T("ROM を開く...");
	ofn.lpstrFilter = _T("NES (*.nes)\0*.nes\0すべてのﾌｧｲﾙ (*.*)\0*.*\0");
#else
	ofn.lpstrTitle = _T("Open ROM...");
	ofn.lpstrFilter = _T("NES (*.nes;*.zip)\0*.nes;*.zip\0All Files (*.*)\0*.*\0");
#endif
	ofn.lpstrFile = name;
	ofn.nMaxFile = MAX_PATH;	
	ofn.Flags = OFN_EXPLORER;
	ofn.lpstrInitialDir = init_path;

	if (!GetOpenFileName(&ofn))
		return;

	LoadROM(name);

	LPTSTR psz = _tcsrchr(name, _T('\\'));
	if (psz) *psz = 0;
	WideCharToMultiByte(CP_ACP, NULL, name, -1, NESTER_settings.OpenPath, MAX_PATH, NULL, NULL);
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
	wsprintf(temp, _T("ﾌｧｲﾙ名:\t%s\nﾏｯﾊﾟｰ #:\t%d\nﾐﾗｰﾘﾝｸﾞ:\t%s\nROM ｻｲｽﾞ:\t%dk\nVROM ｻｲｽﾞ:\t%dk\nﾄﾚｲﾅｰ:\t%s\nｾｰﾌﾞ RAM:\t%s"),
		name, rom_info->get_mapper_num(), mirroring, rom_info->get_num_16k_ROM_banks() * 16, 
		rom_info->get_num_8k_VROM_banks() * 8, rom_info->has_trainer() ? _T("あり") : _T("なし"),
		rom_info->has_save_RAM() ? _T("あり") : _T("なし"));

	MessageBox(hwnd, temp, _T("ROM 情報"), MB_OK);
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
	TCHAR init_path[MAX_PATH] = _T("");
	MakeShortSaveStateFilename(savestate_filename);
	MultiByteToWideChar(CP_ACP, 0, savestate_filename, -1, savestate_filename2, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, NESTER_settings.OpenPath, -1, init_path, MAX_PATH);
	
	OPENFILENAME ofn;
	memset(&(ofn), 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner   = hwnd;
	ofn.lpstrFile   = savestate_filename2;
	ofn.nMaxFile    = MAX_PATH;
#ifdef NESTERJ
	ofn.lpstrTitle  = _T("State 保存...");
	ofn.lpstrFilter = _T("save file (*.ss0)\0*.ss0\0save file (*.ss1)\0*.ss1\0save file (*.ss2)\0*.ss2\0save file (*.ss3)\0*.ss3\0save file (*.ss4)\0*.ss4\0save file (*.ss5)\0*.ss5\0save file (*.ss6)\0*.ss6\0save file (*.ss7)\0*.ss7\0save file (*.ss8)\0*.ss8\0save file (*.ss9)\0*.ss9\0すべてのﾌｧｲﾙ (*.*)\0*.*\0\0");
#else
	ofn.lpstrTitle  = _T("Save state...");
	ofn.lpstrFilter = _T("save file (*.ss0)\0*.ss0\0save file (*.ss1)\0*.ss1\0save file (*.ss2)\0*.ss2\0save file (*.ss3)\0*.ss3\0save file (*.ss4)\0*.ss4\0save file (*.ss5)\0*.ss5\0save file (*.ss6)\0*.ss6\0save file (*.ss7)\0*.ss7\0save file (*.ss8)\0*.ss8\0save file (*.ss9)\0*.ss9\0All Files (*.*)\0*.*\0\0");
#endif
	ofn.nFilterIndex = savestate_slot;
	ofn.Flags       = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir = init_path;
    if (GetSaveFileName(&ofn)) {
		WideCharToMultiByte(CP_ACP, NULL, savestate_filename2, -1, savestate_filename, MAX_PATH, NULL, NULL);
		emu->saveState(savestate_filename);

		LPTSTR psz = _tcsrchr(savestate_filename2, _T('\\'));
		if (psz) *psz = 0;
		WideCharToMultiByte(CP_ACP, NULL, savestate_filename2, -1, NESTER_settings.OpenPath, MAX_PATH, NULL, NULL);

		resume();
	}
}

void OnLoadState(HWND hwnd)
{
	if (!emu) return;

	char savestate_filename[MAX_PATH];
	TCHAR savestate_filename2[MAX_PATH];
	TCHAR init_path[MAX_PATH] = _T("");
	MakeShortSaveStateFilename(savestate_filename);
	MultiByteToWideChar(CP_ACP, 0, savestate_filename, -1, savestate_filename2, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, NESTER_settings.OpenPath, -1, init_path, MAX_PATH);

	OPENFILENAME ofn;
	memset(&(ofn), 0, sizeof(ofn));
	ofn.lStructSize	= sizeof(ofn);
	ofn.hwndOwner   = hwnd;
	ofn.lpstrFile   = savestate_filename2;
	ofn.nMaxFile    = MAX_PATH;	
#ifdef NESTERJ
	ofn.lpstrTitle  = _T("State読み込み...");
	ofn.lpstrFilter = _T("save file (*.ss0)\0*.ss0\0save file (*.ss1)\0*.ss1\0save file (*.ss2)\0*.ss2\0save file (*.ss3)\0*.ss3\0save file (*.ss4)\0*.ss4\0save file (*.ss5)\0*.ss5\0save file (*.ss6)\0*.ss6\0save file (*.ss7)\0*.ss7\0save file (*.ss8)\0*.ss8\0save file (*.ss9)\0*.ss9\0すべてのﾌｧｲﾙ (*.*)\0*.*\0\0");
#else
	ofn.lpstrTitle  = _T("Load state...");
	ofn.lpstrFilter = _T("save file (*.ss0)\0*.ss0\0save file (*.ss1)\0*.ss1\0save file (*.ss2)\0*.ss2\0save file (*.ss3)\0*.ss3\0save file (*.ss4)\0*.ss4\0save file (*.ss5)\0*.ss5\0save file (*.ss6)\0*.ss6\0save file (*.ss7)\0*.ss7\0save file (*.ss8)\0*.ss8\0save file (*.ss9)\0*.ss9\0All Files (*.*)\0*.*\0\0");
#endif
	ofn.nFilterIndex = savestate_slot;
	ofn.Flags       = OFN_EXPLORER | OFN_PATHMUSTEXIST;
	ofn.lpstrInitialDir = init_path;
	if (GetOpenFileName(&ofn)) {
		WideCharToMultiByte(CP_ACP, NULL, savestate_filename2, -1, savestate_filename, MAX_PATH, NULL, NULL);
		emu->loadState(savestate_filename);

		LPTSTR psz = _tcsrchr(savestate_filename2, _T('\\'));
		if (psz) *psz = 0;
		WideCharToMultiByte(CP_ACP, NULL, savestate_filename2, -1, NESTER_settings.OpenPath, MAX_PATH, NULL, NULL);

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