#ifndef _WINCE_EMU_H_
#define _WINCE_EMU_H_

#include <windows.h>   // include important windows stuff
#include <windowsx.h>

#include "emulator.h"

#ifdef _USES_GAPI_DISPLAY
#include "wince_ppc_NES_screen_mgr.h"
#endif
#include "wince_GDI_NES_screen_mgr.h"

#include "input_mgr.h"
#include "wince_sound_mgr.h"
#include "null_sound_mgr.h"
#include "NES_pad.h"
#include "wince_NES_pad.h"

// Rick
#include "wince_OSD_pad.h"

class wince_emu : public emulator
{
public:
	wince_emu(HWND parent_window_handle, HINSTANCE parent_instance_handle, const char* ROM_name);
	~wince_emu();

	void PollInput();

	// emulator interface
	const char* getROMname();
	const char* getROMnameExt();
	const char* getROMpath();
	NES_ROM* get_NES_ROM();
	boolean loadState(const char* fn);
	boolean saveState(const char* fn);
	void reset();
	void do_frame();

	// screen manager interface
	void blt();
	void flip();
	void assert_palette();

	// sound interface
	void enable_sound(boolean enable);
	boolean sound_enabled();
	boolean set_sample_rate(int sample_rate);
	int get_sample_rate();

	// called when input settings are changed
	void input_settings_changed();
	void reset_last_frame_time();

	HBITMAP get_paused_bitmap()
	{
#ifdef _USES_GAPI_DISPLAY
		if (NESTER_settings.nes.graphics.osd.gapi)
			return ((wince_ppc_NES_screen_mgr*)scr_mgr)->get_paused_bitmap();
		else
			return NULL;
#else
		return NULL;
#endif
	}
	wince_OSD_pad * get_osd_pad()
	{
		return osd_pad;
	}

	double getFrameRate()
	{
		if (emu == NULL) {
			return 60000.0/1001.0;
		} else {
			return emu->getFrameRate();
		}
	}

protected:
	HWND parent_wnd_handle;

#ifdef _USES_GAPI_DISPLAY
	NES_screen_mgr* scr_mgr;
#else
	wince_GDI_NES_screen_mgr* scr_mgr;
#endif
	wince_input_mgr* inp_mgr;
	sound_mgr* snd_mgr;
	emulator* emu;
	NES_pad pad1;
	NES_pad pad2;

	// token, local null sound manager; always there
	null_sound_mgr local_null_snd_mgr;

	void onFreeze();
	void onThaw();

	void CreateWincePads();
	void DeleteWincePads();
	wince_NES_pad* wince_pad1;
	wince_NES_pad* wince_pad2;
	// Rick
	wince_OSD_pad* osd_pad;

	boolean emulate_frame(boolean draw);

	double last_frame_time;
	double cur_time;
	int32 skip_count;

	int32 auto_skip_count;
	int   reset_timer;
private:
};
#endif //_WINCE_EMU_H_