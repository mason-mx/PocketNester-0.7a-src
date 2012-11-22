/* $Id: wince_sound_mgr.cpp,v 1.4 2003/10/28 13:00:49 Rick Exp $ */
#include "wince_sound_mgr.h"
#include "debug.h"

wince_sound_mgr::wince_sound_mgr(int sample_rate, int sample_size, int buffer_length_in_frames, int frame_rate) :
sound_mgr(sample_rate, sample_size, 2)
{
	// open waveout device
	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = get_sample_rate();
	wfx.wBitsPerSample = get_sample_size(); 
	wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8; 
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0; //sizeof(WAVEFORMATEX);
	if (waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD) waveout_callback, (DWORD)this, CALLBACK_FUNCTION)
		!= MMSYSERR_NOERROR)
		THROW_EXCEPTION;

	buffered = 0;
	first = 1;
	waveOutPause(hwo);

	frame = get_sample_rate() * (get_sample_size() / 8) / frame_rate;
	if (frame & 0x1) frame += 1;
	int buffer_len = (int)(frame * buffer_length_in_frames);

	// prepare bufffer
	buffer = new uint8[buffer_len];
	memset(buffer, 0, buffer_len);
	hdr = new WAVEHDR[buffer_length_in_frames];
	for (int i = 0; i < buffer_length_in_frames; i++) {
		memset(&hdr[i], 0, sizeof(WAVEHDR));
		hdr[i].lpData = (LPSTR)(buffer + frame * i);
		hdr[i].dwBufferLength = hdr[i].dwBytesRecorded = frame;
		waveOutPrepareHeader(hwo, &hdr[i], sizeof(WAVEHDR));
	}
	pos = SOUND_BUF_LOW;
	hsem = CreateSemaphore(NULL, buffer_length_in_frames, buffer_length_in_frames, NULL);
	current = 0;
	buffer_count = buffer_length_in_frames;
}

wince_sound_mgr::~wince_sound_mgr()
{
	thaw();

	// Rick
	waveOutReset(hwo);
	for (int i = 0; i < buffer_count; i++) {
		waveOutUnprepareHeader(hwo, &hdr[i], sizeof(WAVEHDR));
	}

	waveOutClose(hwo);
	CloseHandle(hsem);
	delete [] buffer;
	delete [] hdr;
}

void wince_sound_mgr::reset()
{
}

void wince_sound_mgr::freeze()
{
	waveOutPause(hwo);
	first = 1;
}

void wince_sound_mgr::thaw()
{
	waveOutRestart(hwo);
}

void wince_sound_mgr::clear_buffer()
{
	waveOutReset(hwo);
	current = 0;
	first = 1;
	buffered = 0;
	pos = SOUND_BUF_LOW;
}

boolean wince_sound_mgr::lock(sound_buf_pos which, void** buf, uint32* buf_len)
{
	*buf = hdr[current].lpData;
	*buf_len = hdr[current].dwBufferLength;
	return TRUE;
}

void wince_sound_mgr::unlock()
{
	buffered += 1;
	waveOutWrite(hwo, &hdr[current], sizeof(WAVEHDR));
	current = (current + 1) % buffer_count;
	
	if (first) {
		if (buffered == buffer_count) {
			waveOutRestart(hwo);
			first = 0;
		}
	}

	WaitForSingleObject(hsem, INFINITE);
}

sound_mgr::sound_buf_pos wince_sound_mgr::get_currently_playing_half()
{
	pos = (pos == SOUND_BUF_LOW) ? SOUND_BUF_HIGH : SOUND_BUF_LOW;
	return pos;
}

void CALLBACK wince_sound_mgr::waveout_callback(HANDLE hwo, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	wince_sound_mgr* mgr = (wince_sound_mgr*)dwUser;
	if (mgr && uMsg == WOM_DONE) {
		mgr->buffered -= 1;
		ReleaseSemaphore(mgr->hsem, 1, NULL);
	}
}