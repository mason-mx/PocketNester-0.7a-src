#ifndef __WINCE_SOUND_MGR_H__
#define __WINCE_SOUND_MGR_H__

#include <windows.h>
#include "sound_mgr.h"

class wince_sound_mgr : public sound_mgr
{
public:
	wince_sound_mgr(int sample_rate, int sample_size, int buffer_length_in_frames, int frame_rate);
	~wince_sound_mgr();

	void reset();

	// lock down for a period of inactivity
	void freeze();
	void thaw();

	void clear_buffer();

	boolean lock(sound_buf_pos which, void** buf, uint32* buf_len);
	void unlock();

	int get_buffer_len()  { return frame * 2;}

	// returns SOUND_BUF_LOW or SOUND_BUF_HIGH
	sound_mgr::sound_buf_pos get_currently_playing_half();

	boolean IsNull() { return FALSE; }

protected:
	HWAVEOUT hwo;
	HANDLE hsem;

	int frame;
	int buffer_count;
	WAVEHDR* hdr;
	uint8* buffer;

	sound_mgr::sound_buf_pos pos;
	int current;

	int buffered;
	int first;

	static void CALLBACK waveout_callback(HANDLE hwo, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
};

#endif //__WINCE_SOUND_MGR_H__