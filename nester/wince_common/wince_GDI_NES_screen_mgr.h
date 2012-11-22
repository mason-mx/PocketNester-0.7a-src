#ifndef __WINCE_GDI_NES_SCREEN_MGR_H__
#define __WINCE_GDI_NES_SCREEN_MGR_H__

#include <windows.h>
#include "types.h"
#include "NES_screen_mgr.h"

class wince_GDI_NES_screen_mgr : public NES_screen_mgr
{
public:
	wince_GDI_NES_screen_mgr(HWND wnd_handle);
	~wince_GDI_NES_screen_mgr();

	boolean lock(pixmap& p);
	boolean unlock();

	void blt();
	void flip();

	void clear(PIXEL color);

	boolean set_palette(const uint8 pal[256][3]);
	boolean get_palette(uint8 pal[256][3]);
	boolean set_palette_section(uint8 start, uint8 len, const uint8 pal[][3]);
	boolean get_palette_section(uint8 start, uint8 len, uint8 pal[][3]);

	void assert_palette();

	void freeze();
	void thaw();

protected:
	HWND window_handle;

	DWORD nes_palette[64];
	int	nes_pal_start;
	uint8* buffer;

	HBITMAP hbmp;
	BITMAPINFOHEADER* pbi;

	unsigned short* screen;
};

#endif //__WINCE_HPC_NES_SCREEN_MGR_H__