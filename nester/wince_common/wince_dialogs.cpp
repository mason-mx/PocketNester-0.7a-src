/* $Id: wince_dialogs.cpp,v 1.4 2003/10/28 13:00:24 Rick Exp $ */
#include "resource.h"
#include "nesterce.h"
#include "settings.h"
#include "wince_dialogs.h"
#include "wince_emu.h"

#define IsDlgButtonChecked(hwnd, id) SendMessage(GetDlgItem(hwnd, id), BM_GETCHECK, 0, 0)
#define CheckDlgButton(hwnd, id, cmd) SendMessage(GetDlgItem(hwnd, id), BM_SETCHECK, cmd, 0)

///////////////////////////////////////////////////////
#define NESTER_ID		_T("nesterROM")
#define NES_DESCRIPTION	_T("NES ROM")
#define _tcssizeof(str) (sizeof(TCHAR) * (_tcslen(str) + 1))

boolean AssociateNESExtension()
{
	HKEY dotNES_key = NULL;
	HKEY dotNES_defIcon_key = NULL;

	HKEY nesterID_key = NULL;
	HKEY nesterID_shell_key = NULL;
	HKEY nesterID_shell_open_key = NULL;
	HKEY nesterID_shell_open_command_key = NULL;

	TCHAR defIcon_str[MAX_PATH];
	TCHAR open_command[MAX_PATH];
	TCHAR full_exe_name[MAX_PATH];

	DWORD dwDisposition;

	// copy executable name from command line
	GetModuleFileName(g_main_instance, full_exe_name, MAX_PATH);
	
	PN_TRY {
		// open the .NES key
		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, _T(".nes"), 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &dotNES_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// set the app ID string
		if(RegSetValueEx(dotNES_key, NULL, 0, REG_SZ, (LPBYTE)NESTER_ID, _tcssizeof(NESTER_ID)) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// set the default icon string
		//_tcscpy(defIcon_str, full_exe_name);
		//if (RegSetValueEx(dotNES_key, _T("DefaultIcon"), 0, REG_SZ, defIcon_str, _tcssizeof(defIcon_str)) != ERROR_SUCCESS)
		//	THROW_EXCEPTION;

		// close the .NES key
		RegCloseKey(dotNES_key);
		dotNES_key = NULL;

		// open the nesterID key
		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, NESTER_ID, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nesterID_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// set the document description string
		if(RegSetValueEx(nesterID_key, NULL, 0, REG_SZ, (LPBYTE)NES_DESCRIPTION,
										_tcssizeof(NES_DESCRIPTION)) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// open the DefaultIcon key
		if (RegCreateKeyEx(nesterID_key, _T("DefaultIcon"), 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &dotNES_defIcon_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// set the default icon string
		wsprintf(defIcon_str, _T("%s,0"), full_exe_name);
		if(RegSetValueEx(dotNES_defIcon_key, NULL, 0, REG_SZ, (LPBYTE)defIcon_str, _tcssizeof(defIcon_str)) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// close the DefaultIcon key
		RegCloseKey(dotNES_defIcon_key);
		dotNES_defIcon_key = NULL;

		// set the Open command

		// create the shell key
		if (RegCreateKeyEx(nesterID_key, _T("Shell"), 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nesterID_shell_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// create the open key
		if (RegCreateKeyEx(nesterID_shell_key, _T("Open"), 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nesterID_shell_open_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// create the command key
		if (RegCreateKeyEx(nesterID_shell_open_key, _T("Command"), 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nesterID_shell_open_command_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// create the open command string
		wsprintf(open_command, _T("\"%s\""), full_exe_name);
		_tcscat(open_command, _T(" \"%1\""));
		
		// set the open command
		if(RegSetValueEx(nesterID_shell_open_command_key, NULL, 0, REG_SZ, (LPBYTE)open_command,
										_tcssizeof(open_command)) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// close the command key
		RegCloseKey(nesterID_shell_open_command_key);
		nesterID_shell_open_command_key = NULL;

		// close the open key
		RegCloseKey(nesterID_shell_open_key);
		nesterID_shell_open_key = NULL;

		// close the shell key
		RegCloseKey(nesterID_shell_key);
		nesterID_shell_key = NULL;

		// close the nesterID key
		RegCloseKey(nesterID_key);
		nesterID_key = NULL;
	}
	PN_CATCH {
		if (dotNES_key)							RegCloseKey(dotNES_key);
		if (dotNES_defIcon_key)					RegCloseKey(dotNES_defIcon_key);
		if (nesterID_key)						RegCloseKey(nesterID_key);
		if (nesterID_shell_key)					RegCloseKey(nesterID_shell_key);
		if (nesterID_shell_open_key)			RegCloseKey(nesterID_shell_open_key);
		if (nesterID_shell_open_command_key)	RegCloseKey(nesterID_shell_open_command_key);
		return FALSE;
	}
	return TRUE;
}

void UndoAssociateNESExtension()
{
	// delete the .NES key
	RegDeleteKey(HKEY_CLASSES_ROOT, _T(".nes"));

	// delete the nesterID key
	RegDeleteKey(HKEY_CLASSES_ROOT, NESTER_ID);
}

void InitializeDialog(HWND hwndDlg)
{
#ifndef _WINCE_HPC
	SHINITDLGINFO shidi;
	shidi.dwMask = SHIDIM_FLAGS;
	shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
	shidi.hDlg = hwndDlg;
	SHInitDialog(&shidi);
#endif
}

///////////////////////////////////////////////////////
BOOL CALLBACK AboutNester_DlgProc(HWND hwndDlg, UINT message,
                                  WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_INITDIALOG:
		{
			InitializeDialog(hwndDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hwndDlg, LOWORD(wParam));
					return TRUE;
			}
			return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////
void PRF_InitDialog(HWND hwndDlg, NES_preferences_settings& settings)
{
	CheckDlgButton(hwndDlg, IDC_CHECK_SPEEDTHROTTLE, settings.speed_throttling);
	if (settings.speed_throttling) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_AUTOFRAMESKIP), TRUE);

		int id = settings.frameskip == FRAME_SKIP_AUTO ? IDC_RADIO_AUTOFRAMESKIP : IDC_RADIO_FRAMESKIP;
		CheckRadioButton(hwndDlg, IDC_RADIO_AUTOFRAMESKIP, IDC_RADIO_FRAMESKIP, id);
	}
	else {
		EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_AUTOFRAMESKIP), FALSE);
		CheckRadioButton(hwndDlg, IDC_RADIO_AUTOFRAMESKIP, IDC_RADIO_FRAMESKIP, IDC_RADIO_FRAMESKIP);
	}

	SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FRAMESKIP), CB_SETCURSEL, settings.frameskip == FRAME_SKIP_AUTO ? 2 : settings.frameskip, 0);
	SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_THREADPRIORITY), CB_SETCURSEL, settings.priority, 0);
}

void PRF_OnOK(HWND hwndDlg, NES_preferences_settings& settings)
{
	TCHAR sz[32];

	settings.speed_throttling = SendMessage(GetDlgItem(hwndDlg, IDC_CHECK_SPEEDTHROTTLE), BM_GETCHECK, 0, 0);
	if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_AUTOFRAMESKIP) && settings.speed_throttling)
		settings.frameskip = FRAME_SKIP_AUTO;
	else {
		GetWindowText(GetDlgItem(hwndDlg, IDC_COMBO_FRAMESKIP), sz, 32);
		settings.frameskip = min(max(_tcstol(sz, 0, 10), 0), MAX_FRAME_SKIP);
	}

	settings.priority = (NES_preferences_settings::NES_PRIORITY)SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_THREADPRIORITY), CB_GETCURSEL, 0, 0);
	EndDialog(hwndDlg, IDOK);
}

BOOL CALLBACK PreferencesOptions_DlgProc(HWND hwndDlg, UINT message,
                                         WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_INITDIALOG:
		{
			InitializeDialog(hwndDlg);

			SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_THREADPRIORITY), CB_ADDSTRING, 0, (LPARAM)_T("NORMAL"));
			SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_THREADPRIORITY), CB_ADDSTRING, 0, (LPARAM)_T("HIGH"));
			SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_THREADPRIORITY), CB_ADDSTRING, 0, (LPARAM)_T("REAL TIME"));
			
			for (int i = 0; i <= MAX_FRAME_SKIP; i++) {
				TCHAR sz[32];
				wsprintf(sz, _T("%d"), i);
				SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FRAMESKIP), CB_ADDSTRING, 0, (LPARAM)sz);
			}
			PRF_InitDialog(hwndDlg, NESTER_settings.nes.preferences);
			if (NESTER_settings.nes.sound.enabled)
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_SPEEDTHROTTLE), FALSE);

			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					PRF_OnOK(hwndDlg, NESTER_settings.nes.preferences);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, LOWORD(wParam));
					return TRUE;
				case IDC_DEFAULTS:
					NESTER_settings.nes.preferences.SetDefaults();
					PRF_InitDialog(hwndDlg, NESTER_settings.nes.preferences);
					return TRUE;
				case IDC_CHECK_SPEEDTHROTTLE:
					if (!IsDlgButtonChecked(hwndDlg, IDC_CHECK_SPEEDTHROTTLE)) {
						CheckRadioButton(hwndDlg, IDC_RADIO_AUTOFRAMESKIP, IDC_RADIO_FRAMESKIP, IDC_RADIO_FRAMESKIP);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_AUTOFRAMESKIP), FALSE);
					}
					else
						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_AUTOFRAMESKIP), TRUE);
					return TRUE;
				case IDC_ASSOCIATE:
					AssociateNESExtension();
					return TRUE;
				case IDC_UNDO:
					UndoAssociateNESExtension();
					return TRUE;
			}
			return FALSE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////
void GRAPHICS_InitDialog(HWND hwndDlg, NES_graphics_settings& settings)
{
	CheckDlgButton(hwndDlg, IDC_CHECK_SHOWSPRITES, settings.show_more_than_8_sprites);
	CheckDlgButton(hwndDlg, IDC_CHECK_SHOWALLSCANLINES, settings.show_all_scanlines);
	CheckDlgButton(hwndDlg, IDC_CHECK_BLACKANDWHITE, settings.black_and_white);
	CheckDlgButton(hwndDlg, IDC_CHECK_CALCPALETTE, settings.calculate_palette);

	HWND hwndSlider = GetDlgItem(hwndDlg, IDC_SLIDER_TINT);
	SendMessage(hwndSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM) MAKELONG(0,255));
	SendMessage(hwndSlider, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)1);
	SendMessage(hwndSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)settings.tint); 
 
	hwndSlider = GetDlgItem(hwndDlg, IDC_SLIDER_HUE);
	SendMessage(hwndSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM) MAKELONG(0,255));
	SendMessage(hwndSlider, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)1);
	SendMessage(hwndSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)settings.hue); 

	EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER_TINT), settings.calculate_palette);
	EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER_HUE), settings.calculate_palette);
	EnableWindow(GetDlgItem(hwndDlg, IDC_RESET), settings.calculate_palette);

	CheckDlgButton(hwndDlg, IDC_SHOW_FPS, settings.osd.show_fps);	// Rick
#ifdef _USES_GAPI_DISPLAY
	CheckDlgButton(hwndDlg, IDC_CHECK_GAPI, settings.osd.gapi);
#else
#ifndef _WINCE_HPC
	ShowWindow(GetDlgItem(hwndDlg, IDC_CHECK_GAPI), SW_HIDE);
#endif
#endif
}

void GRAPHICS_OnOK(HWND hwndDlg, NES_graphics_settings& settings)
{
	settings.show_more_than_8_sprites = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWSPRITES);
	settings.show_all_scanlines = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOWALLSCANLINES);
	settings.black_and_white = IsDlgButtonChecked(hwndDlg, IDC_CHECK_BLACKANDWHITE);
	settings.calculate_palette = IsDlgButtonChecked(hwndDlg, IDC_CHECK_CALCPALETTE);
	settings.tint = (uint8)SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_TINT), TBM_GETPOS, (WPARAM)0, (LPARAM)0);
    settings.hue = (uint8)SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_HUE), TBM_GETPOS, (WPARAM)0, (LPARAM)0);

#ifdef _USES_GAPI_DISPLAY
	settings.osd.gapi = IsDlgButtonChecked(hwndDlg, IDC_CHECK_GAPI);
#endif
	settings.osd.show_fps = IsDlgButtonChecked(hwndDlg, IDC_SHOW_FPS);

	EndDialog(hwndDlg, IDOK);
}

BOOL CALLBACK GraphicsOptions_DlgProc(HWND hwndDlg, UINT message,
                                      WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_INITDIALOG:
		{
			InitializeDialog(hwndDlg);

			GRAPHICS_InitDialog(hwndDlg, NESTER_settings.nes.graphics);

			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_CHECK_CALCPALETTE:
				{
					BOOL enable = IsDlgButtonChecked(hwndDlg, IDC_CHECK_CALCPALETTE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER_TINT), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SLIDER_HUE), enable);
					EnableWindow(GetDlgItem(hwndDlg, IDC_RESET), enable);
					return TRUE;
				}
				case IDC_RESET:
				{
					NES_graphics_settings settings;
					settings.reset_palette();
					SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_TINT), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)settings.tint);
					SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_HUE), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)settings.hue);
					return TRUE;
				}
				case IDC_DEFAULTS:
				{
					NES_graphics_settings settings;
					settings.SetDefaults();
					GRAPHICS_InitDialog(hwndDlg, settings);
					return TRUE;
				}
				case IDOK:
					GRAPHICS_OnOK(hwndDlg, NESTER_settings.nes.graphics);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, LOWORD(wParam));
					return TRUE;
			}
			return FALSE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////
void SOUND_UpdateControls(HWND hwndDlg)
{
	int enable = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SOUNDENABLE);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_RECTANGLE1), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_RECTANGLE2), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_TRIANGLE), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_NOISE), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_DPCM), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_EXTERNAL), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_SAMPLERATE), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_SAMPLESIZE), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_FILTER), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_BUFFERLENGTH), enable);
}

void SOUND_InitDialog(HWND hwndDlg, NES_sound_settings& settings)
{
	CheckDlgButton(hwndDlg, IDC_CHECK_SOUNDENABLE, settings.enabled);
	CheckDlgButton(hwndDlg, IDC_CHECK_RECTANGLE1, settings.rectangle1_enabled);
	CheckDlgButton(hwndDlg, IDC_CHECK_RECTANGLE2, settings.rectangle2_enabled);
	CheckDlgButton(hwndDlg, IDC_CHECK_TRIANGLE, settings.triangle_enabled);
	CheckDlgButton(hwndDlg, IDC_CHECK_NOISE, settings.noise_enabled);
	CheckDlgButton(hwndDlg, IDC_CHECK_DPCM, settings.dpcm_enabled);
	CheckDlgButton(hwndDlg, IDC_CHECK_EXTERNAL, settings.external_enabled);

	int n;
	// sample rate
	HWND hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_SAMPLERATE);
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("8000"));
    SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("11025"));
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("16000"));
    SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("22050"));
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("32000"));
    SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("44100"));
    SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("48000"));
	switch (settings.sample_rate) {
		case 8000: n = 0; break;
		case 11025: n = 1; break;
		case 16000: n = 2; break;
		case 22050: n = 3; break;
		case 32000: n = 4; break;
		case 44100: n = 5; break;
		case 48000: n = 6; break;
		default: n = 1;
	}
	SendMessage(hwndCombo, CB_SETCURSEL, n, 0);

	// sample size
	hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_SAMPLESIZE);
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("8"));
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("16"));
	n = (settings.sample_size == 16) ? 1 : 0;
	SendMessage(hwndCombo, CB_SETCURSEL, n, 0);	

	// filter
	hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_FILTER);
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("none"));
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("simple low-pass"));
	SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("weighted low-pass"));
	switch (settings.filter_type) {
		case NES_sound_settings::FILTER_NONE: n = 0; break;
		case NES_sound_settings::FILTER_LOWPASS: n = 1; break;
		case NES_sound_settings::FILTER_LOWPASS_WEIGHTED: n = 2; break;
		default: n = 0;
	}
	SendMessage(hwndCombo, CB_SETCURSEL, n, 0);

	hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_BUFFERLENGTH);
	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	for (int i = 1; i <= 60; i++) {
		TCHAR sz[32];
		wsprintf(sz, _T("%d"), i);
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)sz);
	}
	SendMessage(hwndCombo, CB_SETCURSEL, settings.buffer_len - 1, 0);
	SOUND_UpdateControls(hwndDlg);
}

void SOUND_OnOK(HWND hwndDlg, NES_sound_settings& settings)
{
	settings.enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_SOUNDENABLE);
	settings.rectangle1_enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_RECTANGLE1);
	settings.rectangle2_enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_RECTANGLE2);
	settings.triangle_enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_TRIANGLE);
	settings.noise_enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_NOISE);
	settings.dpcm_enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_DPCM);
	settings.external_enabled = IsDlgButtonChecked(hwndDlg, IDC_CHECK_EXTERNAL);

	TCHAR sz[32];
	GetWindowText(GetDlgItem(hwndDlg, IDC_COMBO_SAMPLERATE), sz, 32);
	settings.sample_rate = _tcstol(sz, 0, 10);
	
	GetWindowText(GetDlgItem(hwndDlg, IDC_COMBO_SAMPLESIZE), sz, 32);
	settings.sample_size = _tcstol(sz, 0, 10);

	switch (SendMessage(GetDlgItem(hwndDlg, IDC_COMBO_FILTER), CB_GETCURSEL, 0, 0)) {
		case 0:
			settings.filter_type = NES_sound_settings::FILTER_NONE; break;
		case 1:
			settings.filter_type = NES_sound_settings::FILTER_LOWPASS; break;
		case 2:
			settings.filter_type = NES_sound_settings::FILTER_LOWPASS_WEIGHTED; break;
		default:
			settings.filter_type = NES_sound_settings::FILTER_NONE; break;
	}

	GetWindowText(GetDlgItem(hwndDlg, IDC_COMBO_BUFFERLENGTH), sz, 32);
	settings.buffer_len = _tcstol(sz, 0, 10);
	EndDialog(hwndDlg, IDOK);
}

BOOL CALLBACK SoundOptions_DlgProc(HWND hwndDlg, UINT message,
                                      WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_INITDIALOG:
		{
			InitializeDialog(hwndDlg);

			SOUND_InitDialog(hwndDlg, NESTER_settings.nes.sound);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_CHECK_SOUNDENABLE:
					SOUND_UpdateControls(hwndDlg);
					return TRUE;
				case IDOK:
					SOUND_OnOK(hwndDlg, NESTER_settings.nes.sound);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, LOWORD(wParam));
					return TRUE;
				case IDC_DEFAULTS:
				{
					NES_sound_settings settings;
					settings.SetDefaults();
					SOUND_InitDialog(hwndDlg, settings);
				}
			}
	}
	return FALSE;
}

///////////////////////////////////////////////////////
#define MAX_BUTTON 8
int press_key;
static int altKeyScan;

BOOL CALLBACK CTR_KeyPressDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG:
			press_key = 0;
			return TRUE;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL) {
				press_key = 0;
				EndDialog(hwndDlg, IDCANCEL);
			}
			return TRUE;
#ifdef _WINCE_HPC
		case WM_KEYUP:
			press_key = wParam;
			EndDialog(hwndDlg, IDOK);
			return TRUE;
#else
		case WM_KEYDOWN:
		{
            // Rick
            if (altKeyScan) {
                if (wParam == VK_LWIN || wParam == 0x84 /* VK_F21 */) {
                    return FALSE;
                }
                if (wParam == VK_RETURN || wParam == 0x86 /* VK_F23 */) {
                    // let WM_KEYUP to handle the enter key to avoid this
                    // dialog to be activated again
                    return FALSE;
                }
                press_key = wParam;
                EndDialog(hwndDlg, IDOK);
                return TRUE;
            }
			if (wParam == 0x5B) {
				for (int i = 0xC1; i < 0xC6; i++) {
					if (GetAsyncKeyState(i)) {
						press_key = i;
						EndDialog(hwndDlg, IDOK);
						return TRUE;
					}
				}
			}
			// for jornada 56x
			if (wParam == 0x84) {
				for (int i = 0x25; i < 0x29; i++) {
					if (GetAsyncKeyState(i)) {
						press_key = i;
						EndDialog(hwndDlg, IDOK);
						return TRUE;
					}
				}
			}
			return FALSE;
		}
		case WM_KEYUP:
			if (wParam == 0xD)
				press_key = 0x86;
			else
				press_key = wParam;
			EndDialog(hwndDlg, IDOK);
			return TRUE;
#endif
		default: return FALSE;
	}
}

int CTR_StartKeyDialog(HWND hwndParent)
{
    // Rick
    HWND altKeyScanBox = GetDlgItem(hwndParent, IDC_ALT_KEYSCAN);
    altKeyScan = (::SendMessage(altKeyScanBox, BM_GETCHECK, 0, 0) == BST_CHECKED ) ? 1 : 0;

#ifdef _USES_GAPI_INPUT
	GXOpenInput();
	DialogBox(g_main_instance, (LPCTSTR)IDD_OPTIONS_KEYPRESS, hwndParent, CTR_KeyPressDlg);
	GXCloseInput();
	return press_key;
#else
	DialogBox(g_main_instance, (LPCTSTR)IDD_OPTIONS_KEYPRESS, hwndParent, CTR_KeyPressDlg);
	return press_key;
#endif
}

void CTR_InitDialog(HWND hwndDlg, NES_input_settings& settings, uint8 keys[])
{
	keys[0] = settings.player1.btnA.key;
	keys[1] = settings.player1.btnB.key;
	keys[2] = settings.player1.btnSelect.key;
	keys[3] = settings.player1.btnStart.key;
	keys[4] = settings.player1.btnUp.key;
	keys[5] = settings.player1.btnDown.key;
	keys[6] = settings.player1.btnLeft.key;
	keys[7] = settings.player1.btnRight.key;

    // Rick
    SendMessage(GetDlgItem(hwndDlg, IDC_ALT_KEYSCAN), BM_SETCHECK,
        settings.wince_altKeyScan ? BST_CHECKED : BST_UNCHECKED, 0);

	TCHAR sz[32];
	for (int i = 0; i < MAX_BUTTON; i++) {
		wsprintf(sz, _T("0x%X"), keys[i]);
		SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_KEY_A + i), sz);	
	}
}

void CTR_OnOK(HWND hwndDlg, NES_input_settings& settings, uint8 keys[])
{
	settings.player1.btnA.key = keys[0];
	settings.player1.btnB.key = keys[1];
	settings.player1.btnSelect.key = keys[2];
	settings.player1.btnStart.key = keys[3];
	settings.player1.btnUp.key = keys[4];
	settings.player1.btnDown.key = keys[5];
	settings.player1.btnLeft.key = keys[6];
	settings.player1.btnRight.key = keys[7];

    // Rick
    HWND altKeyScanBox = GetDlgItem(hwndDlg, IDC_ALT_KEYSCAN);
    settings.wince_altKeyScan = (::SendMessage(altKeyScanBox, BM_GETCHECK, 0, 0) == BST_CHECKED ) ? 1 : 0;

	EndDialog(hwndDlg, IDOK);
}

BOOL CALLBACK ControllersOptions_DlgProc(HWND hwndDlg, UINT message,
                                         WPARAM wParam, LPARAM lParam)
{
	static uint8 keys[MAX_BUTTON];
	switch (message) {
		case WM_INITDIALOG:
		{
			InitializeDialog(hwndDlg);

			CTR_InitDialog(hwndDlg, NESTER_settings.nes.input, keys);
			break;
		}
		case WM_COMMAND:
		{
			WORD wID = LOWORD(wParam);
			if (wID == IDOK) {
				CTR_OnOK(hwndDlg, NESTER_settings.nes.input, keys);
				return TRUE;
			}
			else if (wID == IDCANCEL) {
				EndDialog(hwndDlg, LOWORD(wParam));
				return TRUE;
			}
			else if (wID >= IDC_A && wID <= IDC_RIGHT) {
				int n = CTR_StartKeyDialog(hwndDlg);
				if (n) {
					TCHAR sz[32];
					int nIndex = wID - IDC_A;
					wsprintf(sz, _T("0x%X"), n);
					SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_KEY_A + nIndex), sz);
					keys[nIndex] = n;
				}
				return TRUE;
			}
			else if (wID >= IDC_DEL_A && wID <= IDC_DEL_RIGHT) {
				TCHAR sz[32]; 
				int nIndex = wID - IDC_DEL_A;
				wsprintf(sz, _T("0x%X"), 0);
				SetWindowText(GetDlgItem(hwndDlg, IDC_STATIC_KEY_A + nIndex), sz);
				keys[nIndex] = 0;
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

///////////////////////////////////////////////////////
void DIR_InitDialog(HWND hwndDlg, NES_preferences_settings& settings)
{
	int n;
	TCHAR sz[MAX_PATH];
	switch (settings.saveRamDirType) {
		case NES_preferences_settings::ROM_DIR:
			n = IDC_RADIO_SRAM_ROMDIRECTORY; break;
		case NES_preferences_settings::NESTER_DIR:
			n = IDC_RADIO_SRAM_NESTERDIRECTORY; break;
		default:
			n = IDC_RADIO_SRAM_ROMDIRECTORY; break;
	}
	CheckRadioButton(hwndDlg, IDC_RADIO_SRAM_ROMDIRECTORY, IDC_RADIO_SRAM_NESTERDIRECTORY, n);
	MultiByteToWideChar(CP_ACP, 0, settings.saveRamDir, -1, sz, MAX_PATH);
	SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_SRAM), sz);

	switch (settings.saveStateDirType) {
		case NES_preferences_settings::ROM_DIR:
			n = IDC_RADIO_SAVESTATE_ROMDIRECTORY; break;
		case NES_preferences_settings::NESTER_DIR:
			n = IDC_RADIO_SAVESTATE_NESTERDIRECTORY; break;
		default:
			n = IDC_RADIO_SAVESTATE_ROMDIRECTORY; break;
	}
	CheckRadioButton(hwndDlg, IDC_RADIO_SAVESTATE_ROMDIRECTORY, IDC_RADIO_SAVESTATE_NESTERDIRECTORY, n);
	MultiByteToWideChar(CP_ACP, 0, settings.saveStateDir, -1, sz, MAX_PATH);
	SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_SAVESTATE), sz);
}

void DIR_OnOK(HWND hwndDlg, NES_preferences_settings& settings)
{
	TCHAR sz[MAX_PATH];
	settings.saveRamDirType = IsDlgButtonChecked(hwndDlg, IDC_RADIO_SRAM_ROMDIRECTORY) ?
								NES_preferences_settings::ROM_DIR :	NES_preferences_settings::NESTER_DIR;
	GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_SRAM), sz, MAX_PATH);
	WideCharToMultiByte(CP_ACP, NULL, sz, -1, settings.saveRamDir, MAX_PATH, NULL, NULL);

	settings.saveStateDirType= IsDlgButtonChecked(hwndDlg, IDC_RADIO_SAVESTATE_ROMDIRECTORY) ?
								NES_preferences_settings::ROM_DIR :	NES_preferences_settings::NESTER_DIR;
	GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_SAVESTATE), sz, MAX_PATH);
	WideCharToMultiByte(CP_ACP, NULL, sz, -1, settings.saveStateDir, MAX_PATH, NULL, NULL);

	EndDialog(hwndDlg, IDOK);
}

BOOL CALLBACK DirectoryOptions_DlgProc(HWND hwndDlg, UINT message,
                                      WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_INITDIALOG:
		{
			InitializeDialog(hwndDlg);

			DIR_InitDialog(hwndDlg, NESTER_settings.nes.preferences);

			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_DEFAULTS:
				{
					NES_preferences_settings settings;
					settings.SetDefaults();
					DIR_InitDialog(hwndDlg, settings);
					return TRUE;
				}
				case IDOK:
					DIR_OnOK(hwndDlg, NESTER_settings.nes.preferences);
					return TRUE;
				case IDCANCEL:
					EndDialog(hwndDlg, LOWORD(wParam));
					return TRUE;
			}
			return FALSE;
	}
	return FALSE;
}
