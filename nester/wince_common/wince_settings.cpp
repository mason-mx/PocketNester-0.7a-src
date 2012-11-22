/*
** nester - NES emulator
** Copyright (C) 2000  Darren Ranalli
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
/* $Id: wince_settings.cpp,v 1.4 2003/10/28 13:01:38 Rick Exp $ */

#include <windows.h>
#include <windowsx.h>

#include <stdio.h>
#include "debug.h"
#include "settings.h"

// this function sets the OS-dependent input setting defaults
#include "OSD_ButtonSettings.h"

void NES_controller_input_settings::OSD_SetDefaults(int num)
{
	if (num == 0) {
		btnUp.SetKeyboard(VK_UP);
		btnDown.SetKeyboard(VK_DOWN);
		btnLeft.SetKeyboard(VK_LEFT);
		btnRight.SetKeyboard(VK_RIGHT);
#ifdef _WINCE_HPC
		btnSelect.SetKeyboard('A');
		btnStart.SetKeyboard('S');
		btnA.SetKeyboard('X');
		btnB.SetKeyboard('Z');
#else
		btnSelect.SetKeyboard(0xC1);
		btnStart.SetKeyboard(0xC2);
		btnA.SetKeyboard(0xC4);
		btnB.SetKeyboard(0xC3);
#endif
	}
	else {
		btnUp.SetNone();
		btnDown.SetNone();
		btnLeft.SetNone();
		btnRight.SetNone();
		btnSelect.SetNone();
		btnStart.SetNone();
		btnB.SetNone();
		btnA.SetNone();
	}
}

/////////////////////////////////////////////////
// Windows emulator settings saving/loading code
// uses the *registry*!  tada!
/////////////////////////////////////////////////
#define REG_BOOLEAN REG_DWORD

// KEY NAMES
#define NESTER_KEY_NAME           _T("Software\\PocketNester")
#define NES_KEY_NAME              _T("NES")
#define NES_PREFERENCES_KEY_NAME  _T("Preferences")
#define NES_GRAPHICS_KEY_NAME     _T("Graphics")
#define NES_SOUND_KEY_NAME        _T("Sound")
#define NES_INPUT_KEY_NAME        _T("Input")
#define NES_INPUT_CNT1_KEY_NAME   _T("Controller1")
#define NES_INPUT_CNT2_KEY_NAME   _T("Controller2")
#define NES_INPUT_UP_KEY_NAME     _T("Up")
#define NES_INPUT_DOWN_KEY_NAME   _T("Down")
#define NES_INPUT_LEFT_KEY_NAME   _T("Left")
#define NES_INPUT_RIGHT_KEY_NAME  _T("Right")
#define NES_INPUT_SELECT_KEY_NAME _T("Select")
#define NES_INPUT_START_KEY_NAME  _T("Start")
#define NES_INPUT_B_KEY_NAME      _T("B")
#define NES_INPUT_A_KEY_NAME      _T("A")
#define NES_INPUT_ALT_KEYSCAN     _T("AltKeyScan")
//#define RECENT_KEY_NAME           _T("Recent")

// VALUE NAMES
#define NES_PREFERENCES_RUNINBACKGROUND_VALUE_NAME      _T("RunInBackground")
#define NES_PREFERENCES_SPEEDTHROTTLE_VALUE_NAME        _T("SpeedThrottle")
#define NES_PREFERENCES_FRAMESKIP_VALUE_NAME            _T("Frameskip")
#define NES_PREFERENCES_PRIORITY_VALUE_NAME             _T("Priority")
#define NES_PREFERENCES_SAVERAMDIRTYPE_VALUE_NAME       _T("SaveRamDirType")
#define NES_PREFERENCES_SAVERAMOTHERDIR_VALUE_NAME      _T("SaveRamOtherDir")
#define NES_PREFERENCES_SAVESTATEDIRTYPE_VALUE_NAME     _T("SaveStateDirType")
#define NES_PREFERENCES_SAVESTATEOTHERDIR_VALUE_NAME    _T("SaveStateOtherDir")
// Rick
#define NES_PREFERENCES_FORCEPAL_VALUE_NAME             _T("ForcePAL")
#define NES_GRAPHICS_SHOW_FPS_VALUE_NAME                _T("ShowFPS")

#define NES_GRAPHICS_BLACKANDWHITE_VALUE_NAME           _T("BlackAndWhite")
#define NES_GRAPHICS_MORETHAN8SPRITESPERLINE_VALUE_NAME _T("MoreThan8Sprites")
#define NES_GRAPHICS_SHOWALLSCANLINES_VALUE_NAME        _T("ShowAllScanlines")
#define NES_GRAPHICS_CALCPALETTE_VALUE_NAME             _T("CalculatePalette")
#define NES_GRAPHICS_TINT_VALUE_NAME                    _T("Tint")
#define NES_GRAPHICS_HUE_VALUE_NAME                     _T("Hue")
#ifdef _WINCE_HPC
#define NES_GRAPHICS_OSD_HIDETASKBAR_VALUE_NAME         _T("HideTaskbar")
#else
#define NES_GRAPHICS_OSD_FULLSCREEN_VALUE_NAME          _T("FullScreen")
#define NES_GRAPHICS_OSD_GAPI_VALUE_NAME				_T("gapi")
#endif

#define NES_SOUND_SOUNDENABLED_VALUE_NAME               _T("SoundEnabled")
#define NES_SOUND_SAMPLERATE_VALUE_NAME                 _T("SampleRate")
#define NES_SOUND_SAMPLESIZE_VALUE_NAME                 _T("SampleSize")
#define NES_SOUND_RECTANGLE1_VALUE_NAME                 _T("Rectangle1")
#define NES_SOUND_RECTANGLE2_VALUE_NAME                 _T("Rectangle2")
#define NES_SOUND_TRIANGLE_VALUE_NAME                   _T("Triangle")
#define NES_SOUND_NOISE_VALUE_NAME                      _T("Noise")
#define NES_SOUND_DPCM_VALUE_NAME                       _T("DPCM")
#define NES_SOUND_EXTERNAL_VALUE_NAME                   _T("External")
#define NES_SOUND_BUFFERLEN_VALUE_NAME                  _T("BufferLength")
#define NES_SOUND_FILTERTYPE_VALUE_NAME                 _T("FilterType")

#define NES_INPUT_BUTTON_DEVICETYPE_VALUE_NAME          _T("DeviceType")
#define NES_INPUT_BUTTON_KEY_VALUE_NAME                 _T("Key")
#define OPENPATH_VALUE_NAME                             _T("OpenPath")

#define LOAD_SETTING(KEY, VAR, VAL_NAME) \
  data_size = sizeof(VAR); \
  RegQueryValueEx(KEY, VAL_NAME, NULL, &data_type, (LPBYTE)&VAR, &data_size);

#define SAVE_SETTING(KEY, VAR, TYPE, VAL_NAME) \
  RegSetValueEx(KEY, VAL_NAME, 0, TYPE, (CONST BYTE*)&VAR, sizeof(VAR));


void LoadNESSettings(HKEY nester_key, NES_settings& settings);
void SaveNESSettings(HKEY nester_key, NES_settings& settings);
void LoadControllerSettings(HKEY NES_input_key, LPCTSTR keyName, NES_controller_input_settings* settings);
void SaveControllerSettings(HKEY NES_input_key, LPCTSTR keyName, NES_controller_input_settings* settings);

boolean OSD_LoadSettings(class settings_t& settings)
{
	HKEY nester_key;
	DWORD dwDisposition;

	// open the "nester" key
	if (RegCreateKeyEx(HKEY_CURRENT_USER, NESTER_KEY_NAME, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nester_key, &dwDisposition) != ERROR_SUCCESS)
		return FALSE;

	// load the NES settings
	LoadNESSettings(nester_key, settings.nes);

	// load the open path
	{
	DWORD data_type;
	DWORD data_size;
	LOAD_SETTING(nester_key, settings.OpenPath, OPENPATH_VALUE_NAME);
	}

	if (strcmp(settings.OpenPath, ".") == 0)
		settings.OpenPath[0] = '\\';

	// close the keys
	RegCloseKey(nester_key);
	return TRUE;
}


boolean OSD_SaveSettings(class settings_t& settings)
{
	HKEY nester_key;
	DWORD dwDisposition;

	// open the "nester" key
	if (RegCreateKeyEx(HKEY_CURRENT_USER, NESTER_KEY_NAME, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nester_key, &dwDisposition) != ERROR_SUCCESS)
		return FALSE;

	// save the NES settings
	SaveNESSettings(nester_key, settings.nes);

	// save the open path
	SAVE_SETTING(nester_key, settings.OpenPath, REG_BINARY, OPENPATH_VALUE_NAME);

	// close the keys
	RegCloseKey(nester_key);
	return TRUE;
}


void LoadNESSettings(HKEY nester_key, NES_settings& settings)
{
	HKEY NES_key;
	HKEY NES_preferences_key;
	HKEY NES_graphics_key;
	HKEY NES_sound_key;
	HKEY NES_input_key;
	DWORD data_type;
	DWORD data_size;
	DWORD dwDisposition;

	// open the "NES" key
	if (RegCreateKeyEx(nester_key, NES_KEY_NAME, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_key, &dwDisposition) != ERROR_SUCCESS)
		return;

	// load preferences settings
	PN_TRY {
		NES_preferences_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_PREFERENCES_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_preferences_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// load settings
		LOAD_SETTING(NES_preferences_key, settings.preferences.speed_throttling, NES_PREFERENCES_SPEEDTHROTTLE_VALUE_NAME);
		LOAD_SETTING(NES_preferences_key, settings.preferences.frameskip, NES_PREFERENCES_FRAMESKIP_VALUE_NAME);
		LOAD_SETTING(NES_preferences_key, settings.preferences.priority, NES_PREFERENCES_PRIORITY_VALUE_NAME);
		LOAD_SETTING(NES_preferences_key, settings.preferences.saveRamDirType, NES_PREFERENCES_SAVERAMDIRTYPE_VALUE_NAME);
		LOAD_SETTING(NES_preferences_key, settings.preferences.saveRamDir, NES_PREFERENCES_SAVERAMOTHERDIR_VALUE_NAME);
		LOAD_SETTING(NES_preferences_key, settings.preferences.saveStateDirType, NES_PREFERENCES_SAVESTATEDIRTYPE_VALUE_NAME);
		LOAD_SETTING(NES_preferences_key, settings.preferences.saveStateDir, NES_PREFERENCES_SAVESTATEOTHERDIR_VALUE_NAME);
		// Rick
		LOAD_SETTING(NES_preferences_key, settings.preferences.force_pal, NES_PREFERENCES_FORCEPAL_VALUE_NAME);

		RegCloseKey(NES_preferences_key);
	}
	PN_CATCH {
		if (NES_preferences_key) RegCloseKey(NES_preferences_key);
	}

	// load graphics settings
	PN_TRY {
		NES_graphics_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_GRAPHICS_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_graphics_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// load settings
		LOAD_SETTING(NES_graphics_key, settings.graphics.black_and_white, NES_GRAPHICS_BLACKANDWHITE_VALUE_NAME);
		LOAD_SETTING(NES_graphics_key, settings.graphics.show_more_than_8_sprites, NES_GRAPHICS_MORETHAN8SPRITESPERLINE_VALUE_NAME);
		LOAD_SETTING(NES_graphics_key, settings.graphics.show_all_scanlines, NES_GRAPHICS_SHOWALLSCANLINES_VALUE_NAME);
		LOAD_SETTING(NES_graphics_key, settings.graphics.calculate_palette, NES_GRAPHICS_CALCPALETTE_VALUE_NAME);
		LOAD_SETTING(NES_graphics_key, settings.graphics.tint, NES_GRAPHICS_TINT_VALUE_NAME);
		LOAD_SETTING(NES_graphics_key, settings.graphics.hue, NES_GRAPHICS_HUE_VALUE_NAME);
#ifdef _WINCE_HPC
		LOAD_SETTING(NES_graphics_key, settings.graphics.osd.hide_taskbar, NES_GRAPHICS_OSD_HIDETASKBAR_VALUE_NAME);
#else
		LOAD_SETTING(NES_graphics_key, settings.graphics.osd.fullscreen, NES_GRAPHICS_OSD_FULLSCREEN_VALUE_NAME);
		LOAD_SETTING(NES_graphics_key, settings.graphics.osd.gapi, NES_GRAPHICS_OSD_GAPI_VALUE_NAME);
#endif
		// Rick
		LOAD_SETTING(NES_graphics_key, settings.graphics.osd.show_fps, NES_GRAPHICS_SHOW_FPS_VALUE_NAME);

		RegCloseKey(NES_graphics_key);
	}
	PN_CATCH {
		if (NES_graphics_key) RegCloseKey(NES_graphics_key);
	}

	// load sound settings
	PN_TRY {
		NES_sound_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_SOUND_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_sound_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// load settings
		LOAD_SETTING(NES_sound_key, settings.sound.enabled, NES_SOUND_SOUNDENABLED_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.sample_rate, NES_SOUND_SAMPLERATE_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.sample_size, NES_SOUND_SAMPLESIZE_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.rectangle1_enabled, NES_SOUND_RECTANGLE1_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.rectangle2_enabled, NES_SOUND_RECTANGLE2_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.triangle_enabled, NES_SOUND_TRIANGLE_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.noise_enabled, NES_SOUND_NOISE_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.dpcm_enabled, NES_SOUND_DPCM_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.external_enabled, NES_SOUND_EXTERNAL_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.buffer_len, NES_SOUND_BUFFERLEN_VALUE_NAME);
		LOAD_SETTING(NES_sound_key, settings.sound.filter_type, NES_SOUND_FILTERTYPE_VALUE_NAME);

		RegCloseKey(NES_sound_key);
	}
	PN_CATCH {
		if (NES_sound_key) RegCloseKey(NES_sound_key);
	}

	// load input settings
	PN_TRY {
		NES_input_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_INPUT_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_input_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// Rick
        LOAD_SETTING(NES_input_key, settings.input.wince_altKeyScan, NES_INPUT_ALT_KEYSCAN);

		// load settings
		// load controller 1
		LoadControllerSettings(NES_input_key, NES_INPUT_CNT1_KEY_NAME, &settings.input.player1);

		// load controller 2
		LoadControllerSettings(NES_input_key, NES_INPUT_CNT2_KEY_NAME, &settings.input.player2);

		RegCloseKey(NES_input_key);
	}
	PN_CATCH {
		if (NES_input_key) RegCloseKey(NES_input_key);
	}

	// close the "NES" key
	RegCloseKey(NES_key);
}

void SaveNESSettings(HKEY nester_key, NES_settings& settings)
{
	HKEY NES_key;
	HKEY NES_preferences_key;
	HKEY NES_graphics_key;
	HKEY NES_sound_key;
	HKEY NES_input_key;
	DWORD dwDisposition;

	// open the "NES" key
	if (RegCreateKeyEx(nester_key, NES_KEY_NAME, 0, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_key, &dwDisposition) != ERROR_SUCCESS)
		return;

	// save preferences settings
	PN_TRY {
		NES_preferences_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_PREFERENCES_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_preferences_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		SAVE_SETTING(NES_preferences_key, settings.preferences.run_in_background, REG_BOOLEAN, NES_PREFERENCES_RUNINBACKGROUND_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.speed_throttling, REG_BOOLEAN, NES_PREFERENCES_SPEEDTHROTTLE_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.frameskip, REG_BOOLEAN, NES_PREFERENCES_FRAMESKIP_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.priority, REG_DWORD, NES_PREFERENCES_PRIORITY_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.saveRamDirType, REG_DWORD, NES_PREFERENCES_SAVERAMDIRTYPE_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.saveRamDir, REG_BINARY, NES_PREFERENCES_SAVERAMOTHERDIR_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.saveStateDirType, REG_DWORD, NES_PREFERENCES_SAVESTATEDIRTYPE_VALUE_NAME);
		SAVE_SETTING(NES_preferences_key, settings.preferences.saveStateDir, REG_BINARY, NES_PREFERENCES_SAVESTATEOTHERDIR_VALUE_NAME);
		// Rick
		SAVE_SETTING(NES_preferences_key, settings.preferences.force_pal, REG_BOOLEAN, NES_PREFERENCES_FORCEPAL_VALUE_NAME);

		RegCloseKey(NES_preferences_key);
	}
	PN_CATCH {
		if (NES_preferences_key) RegCloseKey(NES_preferences_key);
	}

	// save graphics settings
	PN_TRY {
		NES_graphics_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_GRAPHICS_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_graphics_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		SAVE_SETTING(NES_graphics_key, settings.graphics.black_and_white, REG_BOOLEAN, NES_GRAPHICS_BLACKANDWHITE_VALUE_NAME);
		SAVE_SETTING(NES_graphics_key, settings.graphics.show_more_than_8_sprites, REG_BOOLEAN, NES_GRAPHICS_MORETHAN8SPRITESPERLINE_VALUE_NAME);
		SAVE_SETTING(NES_graphics_key, settings.graphics.show_all_scanlines, REG_BOOLEAN, NES_GRAPHICS_SHOWALLSCANLINES_VALUE_NAME);
		SAVE_SETTING(NES_graphics_key, settings.graphics.calculate_palette, REG_BOOLEAN, NES_GRAPHICS_CALCPALETTE_VALUE_NAME);
		SAVE_SETTING(NES_graphics_key, settings.graphics.tint, REG_BINARY, NES_GRAPHICS_TINT_VALUE_NAME);
		SAVE_SETTING(NES_graphics_key, settings.graphics.hue, REG_BINARY, NES_GRAPHICS_HUE_VALUE_NAME);
#ifdef _WINCE_HPC
		SAVE_SETTING(NES_graphics_key, settings.graphics.osd.hide_taskbar, REG_BOOLEAN, NES_GRAPHICS_OSD_HIDETASKBAR_VALUE_NAME);
#else
		SAVE_SETTING(NES_graphics_key, settings.graphics.osd.fullscreen, REG_BOOLEAN, NES_GRAPHICS_OSD_FULLSCREEN_VALUE_NAME);
		SAVE_SETTING(NES_graphics_key, settings.graphics.osd.gapi, REG_BOOLEAN, NES_GRAPHICS_OSD_GAPI_VALUE_NAME);
#endif
		// Rick
		SAVE_SETTING(NES_graphics_key, settings.graphics.osd.show_fps, REG_BOOLEAN, NES_GRAPHICS_SHOW_FPS_VALUE_NAME);

		RegCloseKey(NES_graphics_key);
	}
	PN_CATCH {
		if (NES_graphics_key) RegCloseKey(NES_graphics_key);
	}

	// save sound settings
	PN_TRY {
		NES_sound_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_SOUND_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_sound_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		SAVE_SETTING(NES_sound_key, settings.sound.enabled, REG_BOOLEAN, NES_SOUND_SOUNDENABLED_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.sample_rate, REG_DWORD, NES_SOUND_SAMPLERATE_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.sample_size, REG_DWORD, NES_SOUND_SAMPLESIZE_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.rectangle1_enabled, REG_BOOLEAN, NES_SOUND_RECTANGLE1_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.rectangle2_enabled, REG_BOOLEAN, NES_SOUND_RECTANGLE2_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.triangle_enabled, REG_BOOLEAN, NES_SOUND_TRIANGLE_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.noise_enabled, REG_BOOLEAN, NES_SOUND_NOISE_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.dpcm_enabled, REG_BOOLEAN, NES_SOUND_DPCM_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.external_enabled, REG_BOOLEAN, NES_SOUND_EXTERNAL_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.buffer_len, REG_DWORD, NES_SOUND_BUFFERLEN_VALUE_NAME);
		SAVE_SETTING(NES_sound_key, settings.sound.filter_type, REG_DWORD, NES_SOUND_FILTERTYPE_VALUE_NAME);

		RegCloseKey(NES_sound_key);
	}
	PN_CATCH {
		if (NES_sound_key) RegCloseKey(NES_sound_key);
	}

	// save input settings
	PN_TRY {
		NES_input_key = 0;

		// open key
		if (RegCreateKeyEx(NES_key, NES_INPUT_KEY_NAME, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_input_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

        // Rick
        SAVE_SETTING(NES_input_key, settings.input.wince_altKeyScan, REG_DWORD, NES_INPUT_ALT_KEYSCAN);

		// save settings
		// save controller 1
		SaveControllerSettings(NES_input_key, NES_INPUT_CNT1_KEY_NAME, &settings.input.player1);

		// save controller 2
		//SaveControllerSettings(NES_input_key, NES_INPUT_CNT2_KEY_NAME, &settings.input.player2);

		RegCloseKey(NES_input_key);
	}
	PN_CATCH {
		if (NES_input_key) RegCloseKey(NES_input_key);
	}

	// close the "NES" key
	RegCloseKey(NES_key);
}

void LoadButtonSettings(HKEY NES_cntr_key, LPCTSTR keyName, OSD_ButtonSettings* settings)
{
	HKEY NES_button_key;
	DWORD data_type;
	DWORD data_size;
	DWORD dwDisposition;

	PN_TRY {
		NES_button_key = 0;

		// open key
		if (RegCreateKeyEx(NES_cntr_key, keyName, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_button_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		//LOAD_SETTING(NES_button_key, settings->type, NES_INPUT_BUTTON_DEVICETYPE_VALUE_NAME);
		LOAD_SETTING(NES_button_key, settings->key, NES_INPUT_BUTTON_KEY_VALUE_NAME);
		RegCloseKey(NES_button_key);
	}
	PN_CATCH {
		if (NES_button_key) RegCloseKey(NES_button_key);
	}

}

void SaveButtonSettings(HKEY NES_cntr_key, LPCTSTR keyName, OSD_ButtonSettings* settings)
{
	HKEY NES_button_key;
	DWORD dwDisposition;

	PN_TRY {
		NES_button_key = 0;

		// open key
		if (RegCreateKeyEx(NES_cntr_key, keyName, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_button_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		//SAVE_SETTING(NES_button_key, settings->type, REG_DWORD, NES_INPUT_BUTTON_DEVICETYPE_VALUE_NAME);
		SAVE_SETTING(NES_button_key, settings->key, REG_BINARY, NES_INPUT_BUTTON_KEY_VALUE_NAME);
		
		RegCloseKey(NES_button_key);
	}
	PN_CATCH {
		if (NES_button_key) RegCloseKey(NES_button_key);
	}
}

void LoadControllerSettings(HKEY NES_input_key, LPCTSTR keyName, NES_controller_input_settings* settings)
{
	HKEY NES_cntr_key;
	DWORD dwDisposition;

	PN_TRY {
		NES_cntr_key = 0;

		// open key
		if (RegCreateKeyEx(NES_input_key, keyName, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_cntr_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		LoadButtonSettings(NES_cntr_key, NES_INPUT_UP_KEY_NAME, &settings->btnUp);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_DOWN_KEY_NAME, &settings->btnDown);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_LEFT_KEY_NAME, &settings->btnLeft);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_RIGHT_KEY_NAME, &settings->btnRight);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_SELECT_KEY_NAME, &settings->btnSelect);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_START_KEY_NAME, &settings->btnStart);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_B_KEY_NAME, &settings->btnB);
		LoadButtonSettings(NES_cntr_key, NES_INPUT_A_KEY_NAME, &settings->btnA);

		RegCloseKey(NES_cntr_key);
	}
	PN_CATCH {
		if (NES_cntr_key) RegCloseKey(NES_cntr_key);
	}
}

void SaveControllerSettings(HKEY NES_input_key, LPCTSTR keyName, NES_controller_input_settings* settings)
{
	HKEY NES_cntr_key;
	DWORD dwDisposition;

	PN_TRY {
		NES_cntr_key = 0;

		// open key
		if (RegCreateKeyEx(NES_input_key, keyName, 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &NES_cntr_key, &dwDisposition) != ERROR_SUCCESS)
			THROW_EXCEPTION;

		// save settings
		SaveButtonSettings(NES_cntr_key, NES_INPUT_UP_KEY_NAME, &settings->btnUp);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_DOWN_KEY_NAME, &settings->btnDown);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_LEFT_KEY_NAME, &settings->btnLeft);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_RIGHT_KEY_NAME, &settings->btnRight);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_SELECT_KEY_NAME, &settings->btnSelect);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_START_KEY_NAME, &settings->btnStart);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_B_KEY_NAME, &settings->btnB);
		SaveButtonSettings(NES_cntr_key, NES_INPUT_A_KEY_NAME, &settings->btnA);

		RegCloseKey(NES_cntr_key);
	}
	PN_CATCH {
		if (NES_cntr_key) RegCloseKey(NES_cntr_key);
	}
}