/* $Id: wince_ppc_NES_screen_mgr.h,v 1.4 2003/03/24 14:31:28 Rick Exp $ */
#ifndef __WINCE_PPC_NES_SCREEN_MGR_H__
#define __WINCE_PPC_NES_SCREEN_MGR_H__

#include "nesterce.h"
#include "types.h"
#include "NES_screen_mgr.h"

#ifdef _USES_GAPI_DISPLAY

class wince_ppc_NES_screen_mgr : public NES_screen_mgr
{
public:
	wince_ppc_NES_screen_mgr(HWND wnd_handle);
	~wince_ppc_NES_screen_mgr();

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

	HBITMAP get_paused_bitmap()
	{
		return paused_bmp;
	}

protected:
	void draw_game_pad(uint16 *scr);
#ifdef REPORT_FPS
	void show_fps(uint16 *scr);
#endif

	HWND window_handle;
	GXDisplayProperties gxdp;

	DWORD nes_palette[128];
	int	nes_pal_start;
	uint8* buffer;
	HBITMAP paused_bmp;
	uint8* bmp_bits;
	uint16* scr_ptr;
	int need_update_pad;
};

#endif // _USES_GAPI_DISPLAY

#endif //!__WINCE_PPC_NES_SCREEN_MGR_H__