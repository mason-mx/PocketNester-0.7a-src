/* $Id: wince_ppc_NES_screen_mgr.cpp,v 1.6 2003/10/28 12:56:03 Rick Exp $ */
#include "wince_ppc_NES_screen_mgr.h"
#include "debug.h"
#include "resource.h"
#include "settings.h"

#ifdef _USES_GAPI_DISPLAY

#include "DecmpImage.h"
extern DecmpImage * g_skin;

wince_ppc_NES_screen_mgr::wince_ppc_NES_screen_mgr(HWND wnd_handle)
{
	// initialize GX
	window_handle = wnd_handle;
	if (!GXOpenDisplay(wnd_handle, GX_FULLSCREEN))
		THROW_EXCEPTION;
	gxdp = GXGetDisplayProperties();

	if (gxdp.cxWidth != 240 && gxdp.cyHeight != 320)
		THROW_EXCEPTION;

	memset(nes_palette, 0, sizeof(nes_palette));
	buffer = new uint8[get_width() * get_height()];
	nes_pal_start = 0;

	struct {
		BITMAPINFOHEADER bi;
		DWORD quad[3];
	} bminfo;
		
	memset(&bminfo, 0, sizeof(bminfo));
	BITMAPINFOHEADER * pbi = &bminfo.bi;
	pbi->biSize = sizeof(BITMAPINFOHEADER);
	pbi->biWidth = get_width() - NES_PPU::SIDE_MARGIN * 2;
	pbi->biHeight = 0 - get_height();
	pbi->biPlanes = 1;
	pbi->biBitCount = 16;
	pbi->biCompression = BI_BITFIELDS;

	DWORD * quad = bminfo.quad;
	if (gxdp.ffFormat & kfDirect555) {
		quad[0] = 0x7C00; quad[1] = 0x03E0; quad[2] = 0x001F; // RGB555
	} else {
		quad[0] = 0xF800; quad[1] = 0x07E0; quad[2] = 0x001F; // RGB565
	}

	paused_bmp = CreateDIBSection(NULL, (LPBITMAPINFO)pbi, DIB_RGB_COLORS, (void**)&bmp_bits, NULL, 0);
	if (!paused_bmp)
		THROW_EXCEPTION;
	/*
	if (GXIsDisplayDRAMBuffer() == TRUE) {
		scr_ptr = 0;
	} else {
		scr_ptr = (uint16 *)GXBeginDraw();
	}
	*/

	need_update_pad = 1;
}

wince_ppc_NES_screen_mgr::~wince_ppc_NES_screen_mgr()
{
	// delete GX
	GXCloseDisplay();
	if (paused_bmp)
		DeleteObject(paused_bmp);
	delete [] buffer;
}

boolean wince_ppc_NES_screen_mgr::lock(pixmap& p)
{
	p.data = buffer;
	p.width = get_width();
	p.height = get_height();
	p.pitch = get_width();
	return TRUE;
}

boolean wince_ppc_NES_screen_mgr::unlock()
{
	return TRUE;
}

#ifdef REPORT_FPS
#define PT 1

uint8 numbers[] = {
	PT, PT, PT,
	PT,  0, PT,
	PT,  0, PT,
	PT,  0, PT,
	PT, PT, PT,

	 0, PT,  0,
	 0, PT,  0,
	 0, PT,  0,
	 0, PT,  0,
	 0, PT,  0,

	PT, PT, PT,
	 0,  0, PT,
	PT, PT, PT,
	PT,  0,  0,
	PT, PT, PT,

	PT, PT, PT,
	 0,  0, PT,
	PT, PT, PT,
	 0,  0, PT,
	PT, PT, PT,

	PT,  0, PT,
	PT,  0, PT,
	PT, PT, PT,
	 0,  0, PT,
	 0,  0, PT,

	PT, PT, PT,
	PT,  0,  0,
	PT, PT, PT,
	 0,  0, PT,
	PT, PT, PT,

	PT, PT, PT,
	PT,  0,  0,
	PT, PT, PT,
	PT,  0, PT,
	PT, PT, PT,

	PT, PT, PT,
	 0,  0, PT,
	 0,  0, PT,
	 0,  0, PT,
	 0,  0, PT,

	PT, PT, PT,
	PT,  0, PT,
	PT, PT, PT,
	PT,  0, PT,
	PT, PT, PT,

	PT, PT, PT,
	PT,  0, PT,
	PT, PT, PT,
	 0,  0, PT,
	PT, PT, PT
};

extern int gFPS;
extern int g_update_fps;

void draw_digit(unsigned short *p, int xpitch, int ypitch, int d)
{
	if (d < 0 || d > 9)
		return;
	uint8 *data = &(numbers[15 * d]);
	for (int y = 0; y < 5; y++) {
		*p = (*data++) ? 0xffff : 0;
		*(p+xpitch) = (*data++) ? 0xffff : 0;
		*(p+xpitch*2) = (*data++) ? 0xffff : 0;
		p += ypitch;
	}
}

#ifdef REPORT_FPS
void wince_ppc_NES_screen_mgr::show_fps(uint16 *pscr)
{
	if (g_update_fps) {
		int xpitch = gxdp.cbxPitch >> 1;
		int ypitch = gxdp.cbyPitch >> 1;

		pscr += ypitch * 241;
		draw_digit(pscr, xpitch, ypitch, gFPS / 10);
		pscr += 4 * xpitch;
		draw_digit(pscr, xpitch, ypitch, gFPS % 10);
		g_update_fps = 0;
	}
}
#endif

#endif

void wince_ppc_NES_screen_mgr::blt()
{
	unsigned short* pscr;
	//if (scr_ptr == 0)
		pscr = (unsigned short*)GXBeginDraw();
	//else
	//	pscr = scr_ptr;

	uint16 * scr = pscr;
	if (pscr == NULL)
		return;

	unsigned char* pbuf = buffer;
	int x, y;

	int xpitch = gxdp.cbxPitch >> 1;
	int ypitch = gxdp.cbyPitch >> 1;

	if (!NESTER_settings.nes.graphics.osd.fullscreen) {
		pscr += ypitch << 5; // top position
	}

#ifdef REPORT_FPS
	if (NESTER_settings.nes.graphics.osd.show_fps)
		show_fps(pscr);
#endif

	// show all 240 lines?
	int yoffset = get_viewable_area_y_offset();
	pscr += yoffset * ypitch;
	pbuf += yoffset * get_width();

	if (xpitch == 1) {
		// do fast blit
		uint32 * dest;
		//for (y = 240; y > 0; y--) {
		for (y = get_viewable_height(); y > 0; y--) {
			pbuf += 16;
			dest = (uint32 *)pscr;
			for (x = 40; x > 0; x--) {
				// 6 pixels per loop
				/*
				uint32 data = nes_palette[*pbuf++];
				data |= nes_palette[*pbuf++] << 16;
				*dest++ = data;
				
				data = nes_palette[*pbuf++];
				data |= nes_palette[*pbuf++] << 16;
				*dest++ = data;

				data = nes_palette[*pbuf++];
				data |= nes_palette[*pbuf++] << 16;
				*dest++ = data;
				*/
				uint32 data = nes_palette[pbuf[0]];
				data |= nes_palette[pbuf[1]] << 16;
				dest[0] = data;
				
				data = nes_palette[pbuf[2]];
				data |= nes_palette[pbuf[3]] << 16;
				dest[1] = data;

				data = nes_palette[pbuf[4]];
				data |= nes_palette[pbuf[5]] << 16;
				dest[2] = data;

				pbuf += 6;
				dest += 3;
			}
			pbuf += 16;
			pscr += ypitch;
		}
	} else {
		uint16 * dest;
		//for (y = 0; y < 240; y++) {
		for (y = get_viewable_height(); y > 0; y--) {
			pbuf += 16;
			dest = pscr;
			for (x = 40; x > 0; x--) {
				// 6 pixels per loop
#if defined(SH3) || defined(MIPS)
				// faster for M$ SH3/MIPS compiler
				*dest = (unsigned short)nes_palette[pbuf[0]];
				dest += xpitch;
				*dest = (unsigned short)nes_palette[pbuf[1]];
				dest += xpitch;
				*dest = (unsigned short)nes_palette[pbuf[2]];
				dest += xpitch;
				*dest = (unsigned short)nes_palette[pbuf[3]];
				dest += xpitch;
				*dest = (unsigned short)nes_palette[pbuf[4]];
				dest += xpitch;
				*dest = (unsigned short)nes_palette[pbuf[5]];
				dest += xpitch;
				pbuf += 6;
#else
				// faster for M$ ARM compiler
				dest[0*xpitch] = (uint16)nes_palette[pbuf[0]];
				dest[1*xpitch] = (uint16)nes_palette[pbuf[1]];
				dest[2*xpitch] = (uint16)nes_palette[pbuf[2]];
				dest[3*xpitch] = (uint16)nes_palette[pbuf[3]];
				dest[4*xpitch] = (uint16)nes_palette[pbuf[4]];
				dest[5*xpitch] = (uint16)nes_palette[pbuf[5]];
				
				dest += 6*xpitch;
				pbuf += 6;
#endif
			}
			pbuf += 16;
			pscr += ypitch;
		}
	}

	if (NESTER_settings.nes.graphics.osd.fullscreen && need_update_pad) {
		draw_game_pad(scr);
	}

	//if (scr_ptr == 0)
		GXEndDraw();
}

void wince_ppc_NES_screen_mgr::flip()
{
}

void wince_ppc_NES_screen_mgr::clear(PIXEL color)
{
}

boolean wince_ppc_NES_screen_mgr::set_palette(const uint8 pal[256][3])
{
	return FALSE;
}

boolean wince_ppc_NES_screen_mgr::get_palette(uint8 pal[256][3])
{
	return FALSE;
}

boolean wince_ppc_NES_screen_mgr::set_palette_section(uint8 start, uint8 len, const uint8 pal[][3])
{
	memset(nes_palette, 0, sizeof(nes_palette));
	nes_pal_start = start;
	ASSERT((start + len) <= sizeof(nes_palette) / sizeof(nes_palette[0]));
	
	// convert palette for GX
	for (int i = 0; i < len; i++) {
		if (gxdp.ffFormat & kfDirect555)
			nes_palette[i] = (((DWORD)pal[i][0] << 7) & 0x7C00) | (((DWORD)pal[i][1] << 2) & 0x03e0) | ((DWORD)pal[i][2] >> 3);
		else// if (gxdp.ffFormat & kfDirect565)
			nes_palette[i] = (((DWORD)pal[i][0] << 8) & 0xf800) | (((DWORD)pal[i][1] << 3) & 0x07e0) | ((DWORD)pal[i][2] >> 3);
		//else if (gxdp.ffFormat & kfDirect888)
		//	nes_palette[i] = (DWORD)pal[i][0] << 16 | (DWORD)pal[i][1] << 8 | (DWORD)pal[i][2];
	}

	return TRUE;
}

boolean wince_ppc_NES_screen_mgr::get_palette_section(uint8 start, uint8 len, uint8 pal[][3])
{
	return FALSE;
}

void wince_ppc_NES_screen_mgr::assert_palette()
{
	set_NES_palette();
}

void wince_ppc_NES_screen_mgr::freeze()
{
	//if (scr_ptr)
	//	GXEndDraw();

	GXSuspend();
	//GXCloseDisplay();

	uint8* src = buffer;
	unsigned short* dest = (unsigned short *)bmp_bits;

	int width = get_width() - NES_PPU::SIDE_MARGIN * 2;

	int yoffset = get_viewable_area_y_offset();
	if (yoffset) {
		memset(dest, 0, yoffset * width * sizeof(*dest));
		dest += yoffset * width;
		src += yoffset * get_width();
	}

	for (int y = get_viewable_height(); y > 0; y--) {
		src += NES_PPU::SIDE_MARGIN;
		for (int x = 0; x < width ; x++) {
			*dest++ = (WORD)nes_palette[*src++ - nes_pal_start];
		}
		src += NES_PPU::SIDE_MARGIN;
	}

	if (yoffset) {
		memset(dest, 0, (get_height() - yoffset - get_viewable_height()) * width * sizeof(*dest));
	}
}

void wince_ppc_NES_screen_mgr::thaw()
{
	GXResume();
	//GXOpenDisplay(window_handle, GX_FULLSCREEN);

	/*
	if (GXIsDisplayDRAMBuffer() == TRUE) {
		scr_ptr = 0;
	} else {
		scr_ptr = (uint16*)GXBeginDraw();
	}
	*/

	need_update_pad = 3;
}

#define RGB_TO_555(r, g, b) \
	(uint16)((((DWORD)(r) << 7) & 0x7C00) | (((DWORD)(g) << 2) & 0x03e0) | ((DWORD)(b) >> 3))

#define RGB_TO_565(r, g, b) \
	(uint16)((((DWORD)(r) << 8) & 0xf800) | (((DWORD)(g) << 3) & 0x07e0) | ((DWORD)(b) >> 3))

void wince_ppc_NES_screen_mgr::draw_game_pad(uint16 *scr)
{
	int xpitch = gxdp.cbxPitch >> 1;
	int ypitch = gxdp.cbyPitch >> 1;

	if (g_skin == NULL)
		return;

	scr += ypitch * 240;
	//uint8 * pad = g_skin->GetBits();
	// from bottom to top
	uint8 * pad = g_skin->GetBits() + 159 * 240 * 3;

	if (gxdp.ffFormat & kfDirect555) {
		for (int y = 80; y > 0; y--) {
			uint16 * dest = scr;
			uint8 * pad_bits = pad;
			for (int x = 240; x > 0; x--) {
				*dest = RGB_TO_555(pad_bits[2], pad_bits[1], pad_bits[0]);
				//*dest = RGB_TO_555(pad_bits[0], pad_bits[1], pad_bits[2]);
				pad_bits += 3;
				dest += xpitch;
			}
			//pad += 240 * 3;
			// from bottom to top
			pad -= 240 * 3;
			scr += ypitch;
		}
	} else {
		for (int y = 80; y > 0; y--) {
			uint16 * dest = scr;
			uint8 * pad_bits = pad;
			for (int x = 240; x > 0; x--) {
				*dest = RGB_TO_565(pad_bits[2], pad_bits[1], pad_bits[0]);
				//*dest = RGB_TO_565(pad_bits[0], pad_bits[1], pad_bits[2]);
				pad_bits += 3;
				dest += xpitch;
			}
			//pad += 240 * 3;
			// from bottom to top
			pad -= 240 * 3;
			scr += ypitch;
		}
	}
	need_update_pad--;
}

#endif //_USES_GAPI_DISPLAY
