/*
** PocketNester - NES emulator for Pocket PC
** Copyright (C) 2003 Rick Lei
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
// $Id: wince_OSD_pad.cpp,v 1.2 2003/03/24 14:30:45 Rick Exp $

#include "NES_pad.h"
#include "nes_settings.h"
#include "wince_OSD_pad.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/*
OSD button map by Lancelot

#009900    #FF0000    #000099               #666699 (A+B)
#FFFF00               #00FF00
#990000    #0000FF    #999900            #00FFFF (B)    #FF00FF (A)
*/

static int32 color_map[] = 
{
	0xFF0000, NES_UP,
	0x0000FF, NES_DOWN,
	0xFFFF00, NES_LEFT,
	0x00FF00, NES_RIGHT,

	0x009900, NES_LEFT | NES_UP,
	0x000099, NES_RIGHT | NES_UP,
	0x990000, NES_LEFT | NES_DOWN,
	0x999900, NES_RIGHT | NES_DOWN,

	0x00FFFF, NES_B,
	0xFF00FF, NES_A,
	0x666699, NES_B | NES_A,

	-1	/* end */
};

#define TURBO_FRAMES 1	// autofire every 1 frames

wince_OSD_pad::wince_OSD_pad(NES_controller_input_settings* settings, NES_pad *pad)
{
	_pad = pad;
	_osd_button_state = 0;
	_color_map = color_map;
	_last_color = -1;
	_turbo_b = settings->btnB.turbo;
	_turbo_a = settings->btnA.turbo;
	_turbo_b_counter = _turbo_a_counter = TURBO_FRAMES;
}

wince_OSD_pad::~wince_OSD_pad()
{

}

void wince_OSD_pad::Poll()
{
	if (!_pad)
		return;

	_output_button_state = _osd_button_state;
	// check autofire
	CheckTurbo(_turbo_b, _turbo_b_counter, NES_B);
	CheckTurbo(_turbo_a, _turbo_a_counter, NES_A);

	// cooperate with wince_NES_pad. we do not clear pressed bit.
	_pad->set_inp_state(_pad->get_inp_state() | _output_button_state);
}

void wince_OSD_pad::CheckTurbo(BOOL & turbo, int32 & counter, int32 mask)
{
	if (turbo && (_osd_button_state & mask)) {
		if (counter) {
			counter--;
			_output_button_state &= ~mask;
		} else {
			// reset counter
			counter = TURBO_FRAMES;
		}
	}
}

void wince_OSD_pad::OnStylusDown(int32 color)
{
	int32 * map = _color_map;
	while (*map != -1) {
		if (*map == color) {
			_last_color = color;
			_osd_button_state = *(map + 1);
			break;
		}
		map += 2;
	}
}

void wince_OSD_pad::OnStylusMove(int32 color)
{
	if (color == _last_color) {
		// no change
		return;
	} else {
		// stylus moved to another button
		_osd_button_state = 0;
		OnStylusDown(color);
	}
}

void wince_OSD_pad::OnStylusUp(int32 color)
{
	_osd_button_state = 0;
	_last_color = -1;
}

