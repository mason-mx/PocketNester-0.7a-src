/* $Id: wince_emu.cpp,v 1.7 2003/10/28 13:03:15 Rick Exp $ */
#include "wince_emu.h"
#include "wince_input_mgr.h"
#include "wince_timing.h"
#include "debug.h"
#include "NES_settings.h"

// Rick
#include "profile.h"

/*
When the NTSC standard was designed, certain frequencies involved
in the color subcarrier were interfering with the 60 Hz power lines.  So
the NTSC engineers set the framerate to 60000/1001 Hz.  See also
"drop frame timecode" on any search engine for the full story.
*/
#define NTSC_FRAMERATE (60000.0/1001.0)
#define PAL_FRAMERATE (50000.0/1001.0)
//#define FRAME_PERIOD   (1000.0/NTSC_FRAMERATE)
//#define FRAME_PERIOD   (1000.0/PAL_FRAMERATE)
//#define FRAME_PERIOD  (FORCE_PAL ? (1000.0/PAL_FRAMERATE) : (1000.0/NTSC_FRAMERATE))

#define THROTTLE_SPEED  (NESTER_settings.nes.preferences.speed_throttling && !NESTER_settings.nes.sound.enabled)
#define SKIP_FRAMES     (NESTER_settings.nes.preferences.frameskip)
#define AUTO_SKIP_FRAMES (NESTER_settings.nes.preferences.frameskip == FRAME_SKIP_AUTO)

wince_emu::wince_emu(HWND parent_window_handle, HINSTANCE parent_instance_handle, const char* ROM_name)
{
	parent_wnd_handle = parent_window_handle;
	scr_mgr = NULL;
	inp_mgr = NULL;
	snd_mgr = &local_null_snd_mgr;
	emu = NULL;

	wince_pad1 = NULL;
	wince_pad2 = NULL;

	osd_pad = NULL;

	SYS_TimeInit();

	PN_TRY {
		PN_TRY {
#ifdef _USES_GAPI_DISPLAY
			if (NESTER_settings.nes.graphics.osd.gapi)
				scr_mgr = new wince_ppc_NES_screen_mgr(parent_wnd_handle);
			else
				scr_mgr = new wince_GDI_NES_screen_mgr(parent_wnd_handle);
#else
			scr_mgr = new wince_GDI_NES_screen_mgr(parent_wnd_handle);
#endif
		}
		PN_CATCH {
			errorlog("wince_ppc_NES_screen_mgr creation error\n");
			THROW_EXCEPTION;
		}

		PN_TRY {
			inp_mgr = new wince_input_mgr();
		}
		PN_CATCH {
			errorlog("wince_input_mgr creation error\n");
			THROW_EXCEPTION;
		}

		// get a null sound mgr
		snd_mgr = &local_null_snd_mgr;

		PN_TRY {
			emu = new NES(ROM_name, scr_mgr, snd_mgr);
		}
		PN_CATCH {
			errorlog("NES creation error\n");
			THROW_EXCEPTION;
		}
		scr_mgr->setParentNES((NES*)emu);
		CreateWincePads();

		// start the timer off right
		reset_last_frame_time();

		// try to init dsound if appropriate
		enable_sound(NESTER_settings.nes.sound.enabled);

		// set up control pads
		emu->set_pad1(&pad1);
		emu->set_pad2(&pad2);

		reset_timer = 1;
	}
	PN_CATCH {
		errorlog("wince_emu creation error\n");
		// careful of the order here
		DeleteWincePads();
		if (emu) delete emu;
		if (scr_mgr) delete scr_mgr;
		if (inp_mgr) delete inp_mgr;
		if (snd_mgr != &local_null_snd_mgr) delete snd_mgr;
		THROW_EXCEPTION;
	}
}

wince_emu::~wince_emu()
{
	DeleteWincePads();
	if (emu) delete emu;
	if (scr_mgr) delete scr_mgr;
	if (inp_mgr) delete inp_mgr;
	if (snd_mgr != &local_null_snd_mgr) delete snd_mgr;
}

void wince_emu::PollInput()
{
	// if we don't have the input focus, release all buttons
	if(GetForegroundWindow() != parent_wnd_handle)
	{
		pad1.release_all_buttons();
		pad2.release_all_buttons();
		return;
	}

	inp_mgr->Poll();

	if (wince_pad1) wince_pad1->Poll();
	if (osd_pad) osd_pad->Poll();	// Must be placed after wince_pad1->Poll()

	if (wince_pad2) wince_pad2->Poll();
}

void wince_emu::input_settings_changed()
{
	DeleteWincePads();
	CreateWincePads();
}

void wince_emu::CreateWincePads()
{
	wince_input_mgr* wince_inp_mgr;

	DeleteWincePads();

	wince_inp_mgr = (wince_input_mgr*)inp_mgr; // naughty

	PN_TRY {
		wince_pad1 = new wince_NES_pad(&NESTER_settings.nes.input.player1, &pad1, wince_inp_mgr);
	}
	PN_CATCH {
		errorlog("CreateWincePads 1 error\n")
		LOG("Error creating pad 1 - " << s << endl);
		wince_pad1 = NULL;
	}

	// Rick
	// cooperate with wince_pad1
	osd_pad = new wince_OSD_pad(&NESTER_settings.nes.input.player1, &pad1);

	PN_TRY {
		wince_pad2 = new wince_NES_pad(&NESTER_settings.nes.input.player2, &pad2, wince_inp_mgr);
	}
	PN_CATCH {
		errorlog("CreateWincePads 2 error\n")
		LOG("Error creating pad 2 - " << s << endl);
		wince_pad2 = NULL;
	}
}

void wince_emu::DeleteWincePads()
{
	if (wince_pad1)	{
		delete wince_pad1;
		wince_pad1 = NULL;
	}
	if (wince_pad2)	{
		delete wince_pad2;
		wince_pad2 = NULL;
	}
	if (osd_pad) {
		delete osd_pad;
		osd_pad = NULL;
	}
}

boolean wince_emu::emulate_frame(boolean draw)
{
	return emu->emulate_frame(draw);
}

void wince_emu::onFreeze()
{
	emu->freeze();
	scr_mgr->freeze();
	inp_mgr->freeze();
}

void wince_emu::onThaw()
{
	inp_mgr->thaw();
	scr_mgr->thaw();
	emu->thaw();
	reset_timer = 1;
}

void wince_emu::reset_last_frame_time()
{
	last_frame_time = SYS_TimeInMilliseconds();
	auto_skip_count = 0;
}

const char* wince_emu::getROMname()
{
	return emu->getROMname();
}

const char* wince_emu::getROMnameExt()
{
	return emu->getROMnameExt();
}

const char* wince_emu::getROMpath()
{
	return emu->getROMpath();
}

NES_ROM* wince_emu::get_NES_ROM()
{
	return emu->get_NES_ROM();
}

boolean wince_emu::loadState(const char* fn)
{
	boolean result;

	freeze();
	result = emu->loadState(fn);
	thaw();

	return result;
}

boolean wince_emu::saveState(const char* fn)
{
	boolean result;

	freeze();
	result = emu->saveState(fn);
	thaw();

	return result;
}

void wince_emu::reset()
{
	freeze();
	emu->reset();
	thaw();
}

void wince_emu::blt()
{
	scr_mgr->blt();
}

void wince_emu::flip()
{
	scr_mgr->flip();
}

void wince_emu::assert_palette()
{
#ifdef _USES_GAPI_DISPLAY
	if (scr_mgr) {
		delete scr_mgr;
		scr_mgr = NULL;
	}

	if (NESTER_settings.nes.graphics.osd.gapi)
		scr_mgr = new wince_ppc_NES_screen_mgr(parent_wnd_handle);
	else
		scr_mgr = new wince_GDI_NES_screen_mgr(parent_wnd_handle);
	scr_mgr->setParentNES((NES*)emu);
#endif
	scr_mgr->assert_palette();
}

void wince_emu::enable_sound(boolean enable)
{
	freeze();

	if (snd_mgr != &local_null_snd_mgr)
	{
		delete snd_mgr;
		snd_mgr = &local_null_snd_mgr;
	}

	if (enable)
	{
		// try to init dsound
		PN_TRY {
			snd_mgr = new wince_sound_mgr(NESTER_settings.nes.sound.sample_rate, NESTER_settings.nes.sound.sample_size,
				NESTER_settings.nes.sound.buffer_len, (int)ceil(getFrameRate()));
		}
		PN_CATCH {
			errorlog("wince_sound_mgr creation error\n");
			snd_mgr = &local_null_snd_mgr;
		}
	}
	((NES*)emu)->new_snd_mgr(snd_mgr);
	thaw();
}

boolean wince_emu::sound_enabled()
{
	return !snd_mgr->IsNull();
}

boolean wince_emu::set_sample_rate(int sample_rate)
{
	if (!sound_enabled()) return FALSE;
	if (get_sample_rate() == sample_rate) return TRUE;
	return TRUE;
}

int wince_emu::get_sample_rate()
{
	return snd_mgr->get_sample_rate();
}

// STATIC FUNCTIONS
static inline void SleepUntil(unsigned long time)
{
	long timeleft;

	while(1) {
		timeleft = time - SYS_TimeInMilliseconds();
		if (timeleft <= 0) break;

		if (timeleft > 2) {
			Sleep((timeleft) - 1);
		}
	}
}

#ifdef REPORT_FPS
int gFPS;
int g_update_fps;
#endif

void wince_emu::do_frame()
{
	int skip = 0;
#ifdef REPORT_FPS
	static unsigned long last_time;
	static unsigned long rendered_frames;
#endif

	if (frozen()) return;

	if (reset_timer) {
		reset_last_frame_time();
		reset_timer = 0;
#ifdef REPORT_FPS
		last_time = SYS_TimeInMilliseconds();
		rendered_frames = 0;
#endif
	}

	double FRAME_PERIOD = 1000.0 / getFrameRate();

	cur_time = SYS_TimeInMilliseconds();
	
	if (THROTTLE_SPEED) {
		unsigned long sleep_to = (unsigned long)(last_frame_time + FRAME_PERIOD);
		if (abs(sleep_to - (unsigned long)cur_time) > 500) {
			// this is impossible. let's reset frame time
			reset_timer = 1;
		} else
			SleepUntil(sleep_to);
	}
	if (AUTO_SKIP_FRAMES) {
		//double frames_since_last = (uint32)((cur_time - last_frame_time) / FRAME_PERIOD);

		// are there extra frames?
		//if (frames_since_last > 1 && auto_skip_count < MAX_FRAME_SKIP) {
		if (cur_time - last_frame_time > FRAME_PERIOD && auto_skip_count < MAX_FRAME_SKIP) {
			skip = 1;
			auto_skip_count += 1;
			// Rick
			if (auto_skip_count >= MAX_FRAME_SKIP) {
				reset_timer = 1;
			}
		}
		else
			auto_skip_count = 0;
	}
	else if (SKIP_FRAMES) {
		if (skip_count)
			skip = 1;
		skip_count = (skip_count + 1) % SKIP_FRAMES;
	}

	// emulate current frame
	PollInput();
	if (skip) {
		emulate_frame(FALSE);
	}
	else {
		if (emulate_frame(TRUE))	{
#ifdef REPORT_FPS
			rendered_frames++;
			if (cur_time - last_time > 1000) {
				gFPS = (int)((rendered_frames * 1000 + 500) / (cur_time - last_time));
				last_time = (unsigned long)cur_time;
				rendered_frames = 0;
				g_update_fps = 1;
			} else if (cur_time <= last_time) {
				// something goes wrong
				last_time = (unsigned long)cur_time;
				rendered_frames = 0;
			}
#endif
			// display frame
            BEGIN_INSTRUMENTATION(blt);
			blt();
            END_INSTRUMENTATION(blt);
			//flip();
		}
	}
	last_frame_time += FRAME_PERIOD;
}
