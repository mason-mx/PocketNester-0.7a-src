/*
** nester - NES emulator
** Copyright (C) 2000  Darren Ranalli
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
/* $Id: nes_ppu.h,v 1.9 2003/10/28 12:53:35 Rick Exp $ */

#ifndef NESPPU_H_
#define NESPPU_H_

#include <stdio.h>
#include "pixmap.h"
#include "types.h"
#include "settings.h"

#include "libsnss.h"

class NES;  // class prototype

#define BYTES_PER_MERGED_TILE 16  // 8x8, 4 pixels per byte

#if BYTES_PER_MERGED_TILE != 16
#error Invalid BYTES_PER_MERGED_TILE
#endif

class CompiledTiles
{
public:
    CompiledTiles(int maxTiles);
    ~CompiledTiles();

    void compile(int start, int count, uint8 *data);
	static void compile(int count, uint8 *src, uint8 *dest);
	//void updateTile(int byteOffset, uint8 data);

    const uint8 * operator [](int index) const
	{
	    //assert(index < _maxTiles);
	    return _tiles + index * BYTES_PER_MERGED_TILE;
	}

private:
    int _maxTiles;
    uint8 * _tiles;
};

class NES_PPU
{
  // SNSS functions
  friend void adopt_BASR(SnssBaseBlock* block, NES* nes);
  friend void adopt_VRAM(SnssVramBlock* block, NES* nes);
  friend int extract_BASR(SnssBaseBlock* block, NES* nes);

public:
  // SIDE_MARGIN allocates 2 8 pixel columns on the left and the right
  // for optimized NES background drawing
  enum { 
    NES_SCREEN_WIDTH  = 256,
    NES_SCREEN_HEIGHT = 240,

    SIDE_MARGIN = 8,

    NES_SCREEN_WIDTH_VIEWABLE  = NES_SCREEN_WIDTH,

    NES_BACKBUF_WIDTH = NES_SCREEN_WIDTH + (2*SIDE_MARGIN)
  };

  static int getTopMargin()      { return NESTER_settings.nes.graphics.show_all_scanlines ? 0 : 8; }
  static int getViewableHeight() { return NES_SCREEN_HEIGHT-(2*getTopMargin()); }

  enum mirroring_type
  {
    MIRROR_HORIZ,
    MIRROR_VERT,
    MIRROR_FOUR_SCREEN
  };

  NES_PPU(NES* parent);
  ~NES_PPU() {}

  void reset();

  void set_mirroring(uint32 nt0, uint32 nt1, uint32 nt2, uint32 nt3);
  void set_mirroring(mirroring_type m);

  uint32 vblank_NMI_enabled();

  uint8 ReadLowRegs(uint32 addr);
  void  WriteLowRegs(uint32 addr, uint8 data);

  uint8 Read0x4014();
  void  Write0x4014(uint8 data);

  // these are the rendering functions
  // screen is drawn a line at a time
  void start_frame();
  void do_scanline_and_draw(uint8* buf);
  void do_scanline_and_dont_draw();
  void end_frame();

  // Rick
  void start_frame(uint8 *buf, int ypitch);
  void end_frame(uint8 *buf);
  void set_tile_banks(uint8 *bank0, uint8 *bank1, uint8 *bank2, uint8 *bank3,
      uint8 *bank4, uint8 *bank5, uint8 *bank6, uint8 *bank7)
  {
      PPU_tile_banks[0] = bank0;
      PPU_tile_banks[1] = bank1;
      PPU_tile_banks[2] = bank2;
      PPU_tile_banks[3] = bank3;
      PPU_tile_banks[4] = bank4;
      PPU_tile_banks[5] = bank5;
      PPU_tile_banks[6] = bank6;
      PPU_tile_banks[7] = bank7;
      tile_bank_switched = TRUE;
  }
  void set_tile_bank(int i, uint8 *bank)
  {
	  // VROM bank switching
      PPU_tile_banks[i] = bank;
      tile_bank_switched = TRUE;
  }
  void set_VRAM_bank(int i, int bank_num)
  {
	  // VRAM bank switching
	  // for mapper 1,4,5,6,13,19,80,85,96,119
	  PPU_VRAM_banks[i] = PPU_patterntables + ((bank_num & 0x0f) << 10);
	  PPU_tile_banks[i] = PPU_tile_tables + (bank_num & 0x0f)*0x400/16*BYTES_PER_MERGED_TILE;
	  tile_bank_switched = TRUE;
  }
  // for mapper 1,4,5,6,13,19,80,85,96,119
  // change mirroring status
  void set_name_table(int bank, int bank_num)
  {
	  set_name_table(bank, PPU_nametables + ((bank_num & 0x03) << 10));
  }
  // for mapper 19,68,90
  // set name table to VROM/VRAM
  void set_name_table(int bank, uint8 *addr)
  {
	  PPU_VRAM_banks[bank] = addr;
	  name_table_switched = TRUE;
  }
  void lazy_do_scanline();
  void lazy_do_scanline_dont_draw();

  void start_vblank();
  void end_vblank();

  // 0x2000
  uint32 NMI_enabled()  { return LowRegs[0] & 0x80; }
  uint32 sprites_8x16() { return LowRegs[0] & 0x20; }

  // 0x2001
  uint32 spr_enabled()    { return LowRegs[1] & 0x10; }
  uint32 bg_enabled()     { return LowRegs[1] & 0x08; }
  uint32 spr_clip_left8() { return !(LowRegs[1] & 0x04); }
  uint32 bg_clip_left8()  { return !(LowRegs[1] & 0x02); }
//  uint32 rgb_pal()        { return LowRegs[1] & 0xE0;}
//  uint8  rgb_bak;

  // 0x2002
  uint32 sprite0_hit()                     { return LowRegs[2] & 0x40; }
  uint32 more_than_8_sprites_on_cur_line() { return LowRegs[2] & 0x20; }
  uint32 VRAM_accessible()                 { return LowRegs[2] & 0x10; }

  // by rinao
  uint8* get_patt() { return PPU_patterntables; }
  uint8* get_namt() { return PPU_nametables; }
  uint8 get_pattype(uint8 bank) { return PPU_patterntype[bank]; }
  void set_pattype(uint8 bank, uint8 data) { PPU_patterntype[bank] = data; }

  // Rick
  //void compile_bank_tiles(int bank) { tiles.compile(bank * 64, 64, PPU_VRAM_banks[bank]); }

  // vram / PPU ram

  // bank ptr table
  // 0,1,2,3 = pattern table 0
  // 4,5,6,7 = pattern table 1
  // 8       = name table 0
  // 9       = name table 1
  // A       = name table 2
  // B       = name table 3
  // THE FOLLOWING IS SPECIAL-CASED AND NOT PHYSICALLY IN THE BANK TABLE
  // C       = mirror of name table 0
  // D       = mirror of name table 1
  // E       = mirror of name table 2
  // F       = mirror of name table 3 (0x3F00-0x3FFF are palette info)
  uint8* PPU_VRAM_banks[12];

  uint8 getBGColor();

  uint8 bg_pal[0x10];
  uint8 spr_pal[0x10];

  // sprite ram
  uint8 spr_ram[0x100];

  uint8 vram_write_protect;

protected:
  NES* parent_NES;
  //CompiledTiles tiles;

  // internal registers
  uint8 LowRegs[0x08];
  uint8 HighReg0x4014;

  // 2 VRAM pattern tables
  uint8 PPU_patterntype[8];
  uint8 PPU_patterntables[0x2000];
  // some games have 16K(or more...) VRAM
  //uint8 PPU_patterntables[0x4000];

  /*
  uint8 PPU_patterntable0[0x1000];
  uint8 PPU_patterntable1[0x1000];
  */

  // 4 internal name tables (2 of these really are in the NES)
  uint8 PPU_nametables[4*0x400];

  // these functions read from/write to VRAM using loopy_v
  uint8 read_2007();
  void write_2007(uint8 data);

  uint32  in_vblank;

  uint16  bg_pattern_table_addr;
  uint16  spr_pattern_table_addr;

  uint16  ppu_addr_inc;

  // loopy's internal PPU variables
  uint16  loopy_v;  // vram address -- used for reading/writing through $2007
                    // see loopy-2005.txt
  uint16  loopy_t;  // temp vram address
  uint8   loopy_x;  // 3-bit subtile x-offset

  uint8   toggle_2005_2006;

  uint8 spr_ram_rw_ptr;  // sprite ram read/write pointer

  uint8 read_2007_buffer;

  // rendering stuff
  uint32 current_frame_line;

  // Rick
  uint8* PPU_tile_banks[8];
  uint8 PPU_tile_tables[0x2000 / 16 * BYTES_PER_MERGED_TILE];
  // some games have 16K(or more...) VRAM
  //uint8 PPU_tile_tables[0x4000 / 16 * BYTES_PER_MERGED_TILE];

  // lazy updating stuff
  uint8* lazy_VRAM_banks[12];
  uint8* lazy_tile_banks[8];
  uint32 lazy_bg_start_line, lazy_spr_start_line;
  uint32 lazy_bg_pattern_table_addr;
  uint32 lazy_spr_pattern_table_addr;
  uint16 lazy_v, lazy_t;
  uint8  lazy_x;
  uint8  mapper_num;
  uint8  lazy_bg_enabled, lazy_spr_enabled;
  uint8  tile_bank_switched, name_table_switched;
  uint8  lazy_bg_need_update, lazy_spr_need_update;
  uint8  lazy_bg_pal[16], lazy_spr_pal[16];

  uint8 * ppu_buf;
  int ppu_ypitch;

  void lazy_update_bg();
  void lazy_update_spr();
  void fast_test_sprite0_hit();
  void fast_mmc5_test();

  enum { BG_WRITTEN_FLAG = 0x01, SPR_WRITTEN_FLAG = 0x02 };
  //uint32 solid_buf[NES_BACKBUF_WIDTH];    // bit flags for pixels of current line
  uint8 solid_buf[NES_BACKBUF_WIDTH];    // bit flags for pixels of current line
  uint8 dummy_buffer[NES_BACKBUF_WIDTH]; // used to do sprite 0 hit detection when we aren't supposed to draw

  void render_bg(uint8* buf);
  void render_spr(uint8* buf);

  void update_tile(int byteOffset, uint8 data);
};

#endif