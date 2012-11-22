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
// $Id: wince_OSD_pad.h,v 1.2 2003/03/24 14:30:45 Rick Exp $

#ifndef WINCE_OSD_PAD_H
#define WINCE_OSD_PAD_H

class NES_pad;

class wince_OSD_pad  
{
public:
	wince_OSD_pad(NES_controller_input_settings* settings, NES_pad * pad);
	~wince_OSD_pad();

	void Poll();
	void OnStylusDown(int32 color);
	void OnStylusMove(int32 color);
	void OnStylusUp(int32 color);

protected:
	void CheckTurbo(BOOL & turbo, int32 & counter, int32 mask);
	NES_pad * _pad;

private:
	uint32 _osd_button_state;
	uint32 _output_button_state;	// after autofire processing

	int32 * _color_map;
	int32 _last_color;
	BOOL _turbo_b;
	int32 _turbo_b_counter;
	BOOL _turbo_a;
	int32 _turbo_a_counter;

};

#endif
