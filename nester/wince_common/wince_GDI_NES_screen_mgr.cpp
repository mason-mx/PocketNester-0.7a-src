#include "wince_GDI_NES_screen_mgr.h"
#include "debug.h"


wince_GDI_NES_screen_mgr::wince_GDI_NES_screen_mgr(HWND wnd_handle)
{	
	buffer = NULL;
	pbi = NULL;
	hbmp = NULL;
	screen = NULL;

	PN_TRY {
		memset(nes_palette, 0, sizeof(nes_palette));
		buffer = new uint8[get_width() * get_height()];
		nes_pal_start = 0;
		window_handle = wnd_handle;

		BITMAPINFOHEADER* bi;
		DWORD* quad;
		
		pbi = (BITMAPINFOHEADER *)malloc(sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD));
		bi = pbi;
		memset(pbi, 0, sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD));
		bi->biSize = sizeof(BITMAPINFOHEADER);
		bi->biWidth = get_width() - NES_PPU::SIDE_MARGIN * 2;
		bi->biHeight = 0 - get_height();
		bi->biPlanes = 1;
		bi->biBitCount = 16;
		bi->biCompression = BI_BITFIELDS;

		quad = (DWORD*)&((LPBITMAPINFO)pbi)->bmiColors[0];
		quad[0] = 0xF800; quad[1] = 0x07E0; quad[2] = 0x001F; // RGB565
		hbmp = CreateDIBSection(NULL, (LPBITMAPINFO)pbi, DIB_RGB_COLORS, (void**)&screen, NULL, 0);
		if (!hbmp)
			THROW_EXCEPTION;
	}
	PN_CATCH {
		if (hbmp) DeleteObject(hbmp);
		if (pbi) free(pbi);
		if (buffer) delete [] buffer;
		THROW_EXCEPTION;
	}
}

wince_GDI_NES_screen_mgr::~wince_GDI_NES_screen_mgr()
{
	if (hbmp) DeleteObject(hbmp);
	if (pbi) free(pbi);
	if (buffer) delete [] buffer;
}

boolean wince_GDI_NES_screen_mgr::lock(pixmap& p)
{
	p.data = buffer;
	p.width = get_width();
	p.height = get_height();
	p.pitch = get_width();
	return TRUE;
}

boolean wince_GDI_NES_screen_mgr::unlock()
{
	return TRUE;
}

void wince_GDI_NES_screen_mgr::blt()
{
	int x, y;
	
	HDC hDC = GetDC(window_handle);
	if (!hDC) return;

	HDC hmemDC = CreateCompatibleDC(hDC);
	if (!hmemDC) return;

	if (hbmp && screen) {
		SelectObject(hmemDC, hbmp);
		uint8* src = buffer;
		unsigned short* dest = screen;

		int width = get_width() - NES_PPU::SIDE_MARGIN * 2, height = get_height();
		for (y = 0; y < height; y++) {
			src += NES_PPU::SIDE_MARGIN;
			for (x = 0; x < width ; x++) {
				*dest++ = (WORD)nes_palette[*src++ - nes_pal_start];
			}
			src += NES_PPU::SIDE_MARGIN;
		}
		
		RECT rc;
		GetClientRect(window_handle, &rc);
		x = ((rc.right - rc.left) - width) / 2;
#ifdef _WINCE_HPC
		y = ((rc.bottom - rc.top) - height) / 2;
#else
		y = ((rc.bottom - rc.top) - height - 26) / 2;
#endif
		BitBlt(hDC, x, y, width, height, hmemDC, 0, 0, SRCCOPY);
	}
	DeleteDC(hmemDC);
	ReleaseDC(window_handle, hDC);
}

void wince_GDI_NES_screen_mgr::flip()
{
}

void wince_GDI_NES_screen_mgr::clear(PIXEL color)
{
}

boolean wince_GDI_NES_screen_mgr::set_palette(const uint8 pal[256][3])
{
	return FALSE;
}

boolean wince_GDI_NES_screen_mgr::get_palette(uint8 pal[256][3])
{
	return FALSE;
}

boolean wince_GDI_NES_screen_mgr::set_palette_section(uint8 start, uint8 len, const uint8 pal[][3])
{
	memset(nes_palette, 0, sizeof(nes_palette));
	nes_pal_start = start;
	
	// convert palette
	for (int i = 0; i < len; i++) {
		// for 16bits RGB565
		nes_palette[i] = (((DWORD)pal[i][0] << 8) & 0xf800) | (((DWORD)pal[i][1] << 3) & 0x07e0) | ((DWORD)pal[i][2] >> 3);

		// for 16bits RGB555
		//nes_palette[i] = (((DWORD)pal[i][0] << 7) & 0x7C00) | (((DWORD)pal[i][1] << 2) & 0x03e0) | ((DWORD)pal[i][2] >> 3);
		// for 24bits RGB
		//nes_palette[i] = (DWORD)pal[i][0] << 16 | (DWORD)pal[i][1] << 8 | (DWORD)pal[i][2];
	}
	return TRUE;
}

boolean wince_GDI_NES_screen_mgr::get_palette_section(uint8 start, uint8 len, uint8 pal[][3])
{
	return FALSE;
}

void wince_GDI_NES_screen_mgr::assert_palette()
{
	set_NES_palette();
}

void wince_GDI_NES_screen_mgr::freeze()
{
}

void wince_GDI_NES_screen_mgr::thaw()
{
}