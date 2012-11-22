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
/* $Id: Nes_ppu.cpp,v 1.9 2003/10/28 12:53:35 Rick Exp $ */

#include <string.h>
#include "NES_PPU.h"
#include "NES.h"
#include "pixmap.h"
#include "settings.h"

#include "debug.h"
#include "profile.h"

CompiledTiles::CompiledTiles(int maxTiles) : _maxTiles(maxTiles)
{
    _tiles = new uint8[maxTiles * BYTES_PER_MERGED_TILE];
    //memset(_tiles, 0, maxTiles * BYTES_PER_MERGED_TILE);
}

CompiledTiles::~CompiledTiles()
{
    delete _tiles;
}

#define EXTRACT_4_PIXELS() \
	col = 0; \
	if(pattern_lo & pattern_mask) col |= (0x01 << 6); \
	if(pattern_hi & pattern_mask) col |= (0x02 << 6); \
	pattern_mask >>= 1; \
    if(pattern_lo & pattern_mask) col |= (0x01 << 4); \
	if(pattern_hi & pattern_mask) col |= (0x02 << 4); \
	pattern_mask >>= 1; \
    if(pattern_lo & pattern_mask) col |= (0x01 << 2); \
	if(pattern_hi & pattern_mask) col |= (0x02 << 2); \
	pattern_mask >>= 1; \
    if(pattern_lo & pattern_mask) col |= (0x01 << 0); \
	if(pattern_hi & pattern_mask) col |= (0x02 << 0); \
	*p++= col;

void CompiledTiles::compile(int start, int count, uint8 *data)
{
    //assert(start >= 0 && start + count <= _maxTiles);

    uint8 *p = _tiles + start * BYTES_PER_MERGED_TILE;
	CompiledTiles::compile(count, data, p);
}

void CompiledTiles::compile(int count, uint8 *src, uint8 *dest)
{
    uint8 *p = dest;
    uint8 col;

    for (int i = 0; i < count; i++) {
        for (int line = 0; line < 8; line++) {
            uint8 pattern_lo = *src;
            uint8 pattern_hi = *(src + 8);

            int pattern_mask = 0x80;

			EXTRACT_4_PIXELS();
            pattern_mask >>= 1;
			EXTRACT_4_PIXELS();

            src++;
        }
		src += 8;
    }
}

#define UPDATE_PIXEL() \
  if(data & pattern_mask) *p |= bit; \
  p++;

#define VRAM(addr) \
  PPU_VRAM_banks[(addr) >> 10][(addr) & 0x3FF]

#define TILE(addr) \
  (PPU_tile_banks[(addr) >> 10] + ((addr) & 0x3FF) / 16 * BYTES_PER_MERGED_TILE)

#define TILE_OFFSET(line) \
  ((line) * (BYTES_PER_MERGED_TILE / 8))

/*
scanline start (if background or sprites are enabled):
	v:0000010000011111=t:0000010000011111
*/
#define LOOPY_SCANLINE_START(v,t) \
  { \
    v = (v & 0xFBE0) | (t & 0x041F); \
  }

/*
bits 12-14 are the tile Y offset.
you can think of bits 5,6,7,8,9 as the "y scroll"(*8).  this functions
slightly different from the X.  it wraps to 0 and bit 11 is switched when
it's incremented from _29_ instead of 31.  there are some odd side effects
from this.. if you manually set the value above 29 (from either 2005 or
2006), the wrapping from 29 obviously won't happen, and attrib data will be
used as name table data.  the "y scroll" still wraps to 0 from 31, but
without switching bit 11.  this explains why writing 240+ to 'Y' in 2005
appeared as a negative scroll value.
*/
#define LOOPY_NEXT_LINE(v) \
  { \
    if((v & 0x7000) == 0x7000) /* is subtile y offset == 7? */ \
    { \
      v &= 0x8FFF; /* subtile y offset = 0 */ \
      if((v & 0x03E0) == 0x03A0) /* name_tab line == 29? */ \
      { \
        v ^= 0x0800;  /* switch nametables (bit 11) */ \
        v &= 0xFC1F;  /* name_tab line = 0 */ \
      } \
      else \
      { \
        if((v & 0x03E0) == 0x03E0) /* line == 31? */ \
        { \
          v &= 0xFC1F;  /* name_tab line = 0 */ \
        } \
        else \
        { \
          v += 0x0020; \
        } \
      } \
    } \
    else \
    { \
      v += 0x1000; /* next subtile y offset */ \
    } \
  }

/*
you can think of bits 0,1,2,3,4 of the vram address as the "x scroll"(*8)
that the ppu increments as it draws.  as it wraps from 31 to 0, bit 10 is
switched.  you should see how this causes horizontal wrapping between name
tables (0,1) and (2,3).
*/
#define LOOPY_NEXT_TILE(v) \
  { \
    if((v & 0x001F) == 0x001F) \
    { \
      v ^= 0x0400; /* switch nametables (bit 10) */ \
      v &= 0xFFE0; /* tile x = 0 */ \
    } \
    else \
    { \
      v++; /* next tile */ \
    } \
  }

#define LOOPY_NEXT_PIXEL(v,x) \
  { \
    if(x == 0x07) \
    { \
      LOOPY_NEXT_TILE(v); \
      x = 0x00; \
    } \
    else \
    { \
      x++; \
    } \
  }

#define CHECK_MMC2(addr) \
  if(((addr) & 0x0FC0) == 0x0FC0) \
  { \
    if((((addr) & 0x0FF0) == 0x0FD0) || (((addr) & 0x0FF0) == 0x0FE0)) \
    { \
      parent_NES->mapper->PPU_Latch_FDFE(addr); \
    } \
  }


NES_PPU::NES_PPU(NES* parent)
{
  parent_NES = parent;
}

void NES_PPU::reset()
{
  // reset registers
  memset(LowRegs, 0x00, sizeof(LowRegs));
  HighReg0x4014 = 0x00;

  // clear sprite RAM
  memset(spr_ram, 0x00, sizeof(spr_ram));

  // clear palettes
  memset(bg_pal,  0x00, sizeof(bg_pal));
  memset(spr_pal, 0x00, sizeof(spr_pal));

  // clear solid buffer
  memset(solid_buf, 0x00, sizeof(solid_buf));

  // clear pattern tables
  /*
  memset(PPU_patterntable0, 0x00, sizeof(PPU_patterntable0));
  memset(PPU_patterntable1, 0x00, sizeof(PPU_patterntable1));
  */
  memset(PPU_patterntables, 0x00, sizeof(PPU_patterntables));
  memset(PPU_patterntype, 0x00, sizeof(PPU_patterntype));

  memset(PPU_tile_tables, 0, sizeof(PPU_tile_tables));

  // clear internal name tables
  memset(PPU_nametables, 0x00, sizeof(PPU_nametables));

  // clear VRAM page table
  memset(PPU_VRAM_banks, 0x00, sizeof(PPU_VRAM_banks));

  memset(PPU_tile_banks, 0, sizeof(PPU_tile_banks));

  // set up PPU memory space table
  /*
  PPU_VRAM_banks[0x00] = PPU_patterntable0 + (0*0x400);
  PPU_VRAM_banks[0x01] = PPU_patterntable0 + (1*0x400);
  PPU_VRAM_banks[0x02] = PPU_patterntable0 + (2*0x400);
  PPU_VRAM_banks[0x03] = PPU_patterntable0 + (3*0x400);

  PPU_VRAM_banks[0x04] = PPU_patterntable1 + (0*0x400);
  PPU_VRAM_banks[0x05] = PPU_patterntable1 + (1*0x400);
  PPU_VRAM_banks[0x06] = PPU_patterntable1 + (2*0x400);
  PPU_VRAM_banks[0x07] = PPU_patterntable1 + (3*0x400);
  */

  PPU_VRAM_banks[0x00] = PPU_patterntables + (0*0x400);
  PPU_VRAM_banks[0x01] = PPU_patterntables + (1*0x400);
  PPU_VRAM_banks[0x02] = PPU_patterntables + (2*0x400);
  PPU_VRAM_banks[0x03] = PPU_patterntables + (3*0x400);
    
  PPU_VRAM_banks[0x04] = PPU_patterntables + (4*0x400);
  PPU_VRAM_banks[0x05] = PPU_patterntables + (5*0x400);
  PPU_VRAM_banks[0x06] = PPU_patterntables + (6*0x400);
  PPU_VRAM_banks[0x07] = PPU_patterntables + (7*0x400);

  // point nametables at internal name table 0
  PPU_VRAM_banks[0x08] = PPU_nametables;
  PPU_VRAM_banks[0x09] = PPU_nametables;
  PPU_VRAM_banks[0x0A] = PPU_nametables;
  PPU_VRAM_banks[0x0B] = PPU_nametables;

  PPU_tile_banks[0x00] = PPU_tile_tables + (0*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x01] = PPU_tile_tables + (1*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x02] = PPU_tile_tables + (2*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x03] = PPU_tile_tables + (3*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x04] = PPU_tile_tables + (4*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x05] = PPU_tile_tables + (5*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x06] = PPU_tile_tables + (6*0x400/16*BYTES_PER_MERGED_TILE);
  PPU_tile_banks[0x07] = PPU_tile_tables + (7*0x400/16*BYTES_PER_MERGED_TILE);

  read_2007_buffer = 0x00;
  in_vblank = 0;
  bg_pattern_table_addr = 0;
  spr_pattern_table_addr = 0;
  ppu_addr_inc = 0;
  loopy_v = 0;
  loopy_t = 0;
  loopy_x = 0;
  toggle_2005_2006 = 0;
  spr_ram_rw_ptr = 0;
  read_2007_buffer = 0;
  current_frame_line = 0;

  // set mirroring
  set_mirroring(parent_NES->ROM->get_mirroring());

  // Rick
  ppu_buf = 0;
  tile_bank_switched = 0;
  memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(PPU_tile_banks));
  name_table_switched = FALSE;
  memcpy(lazy_VRAM_banks, PPU_VRAM_banks, sizeof(PPU_VRAM_banks));
  lazy_v = lazy_t = lazy_x = 0;
}

void NES_PPU::set_mirroring(uint32 nt0, uint32 nt1, uint32 nt2, uint32 nt3)
{
  ASSERT(nt0 < 4); ASSERT(nt1 < 4); ASSERT(nt2 < 4); ASSERT(nt3 < 4);
  PPU_VRAM_banks[0x08] = PPU_nametables + (nt0 << 10); // * 0x0400
  PPU_VRAM_banks[0x09] = PPU_nametables + (nt1 << 10);
  PPU_VRAM_banks[0x0A] = PPU_nametables + (nt2 << 10);
  PPU_VRAM_banks[0x0B] = PPU_nametables + (nt3 << 10);
  name_table_switched = TRUE;
}

void NES_PPU::set_mirroring(mirroring_type m)
{
  if(MIRROR_FOUR_SCREEN == m)
  {
    set_mirroring(0,1,2,3);
  }
  else if(MIRROR_HORIZ == m)
  {
    set_mirroring(0,0,1,1);
  }
  else if(MIRROR_VERT == m)
  {
    set_mirroring(0,1,0,1);
  }
  else
  {
    LOG("Invalid mirroring type" << endl);
    set_mirroring(MIRROR_FOUR_SCREEN);
  }
}

void NES_PPU::start_frame()
{
    current_frame_line = 0;
    
    if(spr_enabled() || bg_enabled())
    {
        loopy_v = loopy_t;
    }
    
    mapper_num = parent_NES->ROM->get_mapper_num();
}

uint8 NES_PPU::getBGColor() { return NES::NES_COLOR_BASE + bg_pal[0]; }

void NES_PPU::do_scanline_and_draw(uint8* buf)
{
    if(!bg_enabled())
    {
        // set to background color
        memset(buf, NES::NES_COLOR_BASE + bg_pal[0], NES_BACKBUF_WIDTH);
    }
    
    if(spr_enabled() || bg_enabled())
    {
        LOOPY_SCANLINE_START(loopy_v, loopy_t);
        
        if(bg_enabled())
        {
            // draw background
            render_bg(buf);
        }
        else
        {
            // clear out solid buffer
            memset(solid_buf, 0x00, sizeof(solid_buf));
        }
        
        if(spr_enabled())
        {
            // draw sprites
            render_spr(buf);
        }
        
        LOOPY_NEXT_LINE(loopy_v);
    }
    
    current_frame_line++;
}

void NES_PPU::do_scanline_and_dont_draw()
{
	// mmc2 / punchout -- we must simulate the ppu for every line
	/*if(parent_NES->ROM->get_mapper_num() == 9)
	{
		do_scanline_and_draw(dummy_buffer);
	}
	else*/
	{
		// if sprite 0 flag not set and sprite 0 on current line
		if((!sprite0_hit()) && 
			(current_frame_line >= ((uint32)(spr_ram[0]+1))) && 
			(current_frame_line <  ((uint32)(spr_ram[0]+1+(sprites_8x16()?16:8))))
			)
		{
			// render line to dummy buffer
			do_scanline_and_draw(dummy_buffer);
		}
		else
		{
			if(spr_enabled() || bg_enabled())
			{
				LOOPY_SCANLINE_START(loopy_v, loopy_t);
				LOOPY_NEXT_LINE(loopy_v);
			}
			current_frame_line++;
		}
	}
}

void NES_PPU::end_frame()
{
}

void NES_PPU::start_frame(uint8 *buf, int ypitch)
{
	ppu_buf = buf;
	ppu_ypitch = ypitch;
	current_frame_line = 0;
	lazy_bg_start_line = 0;
	lazy_spr_start_line = 0;
	lazy_bg_need_update = 0;
	lazy_spr_need_update = 0;
	if (tile_bank_switched) {
		memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(PPU_tile_banks));
		tile_bank_switched = FALSE;
	}
	if (name_table_switched) {
		lazy_VRAM_banks[8] = PPU_VRAM_banks[8];
		lazy_VRAM_banks[9] = PPU_VRAM_banks[9];
		lazy_VRAM_banks[10] = PPU_VRAM_banks[10];
		lazy_VRAM_banks[11] = PPU_VRAM_banks[11];
		name_table_switched = FALSE;
	}
	lazy_v = loopy_v;
	lazy_t = loopy_t;
	lazy_x = loopy_x;
	lazy_bg_pattern_table_addr = bg_pattern_table_addr;
	lazy_spr_pattern_table_addr = spr_pattern_table_addr;

	memcpy(lazy_bg_pal, bg_pal, sizeof(lazy_bg_pal));
	lazy_bg_pal[4] = lazy_bg_pal[8] = lazy_bg_pal[12] = lazy_bg_pal[0];
	memcpy(lazy_spr_pal, spr_pal, sizeof(lazy_spr_pal));
	lazy_spr_pal[4] = lazy_spr_pal[8] = lazy_spr_pal[12] = lazy_spr_pal[0];

	lazy_bg_enabled = bg_enabled();
	lazy_spr_enabled = spr_enabled();

	if(lazy_bg_enabled || lazy_spr_enabled) {
		loopy_v = loopy_t;
		lazy_v = lazy_t;
	}

	mapper_num = parent_NES->ROM->get_mapper_num();
}

void NES_PPU::lazy_do_scanline()
{
	if (!lazy_bg_enabled) {
		memset(ppu_buf + current_frame_line * ppu_ypitch,
			NES::NES_COLOR_BASE + lazy_bg_pal[0], NES_BACKBUF_WIDTH);
	}

	if (tile_bank_switched) {
		if (memcmp(lazy_tile_banks, PPU_tile_banks, sizeof(PPU_tile_banks))) {
			lazy_bg_need_update = TRUE;
			lazy_spr_need_update = TRUE;
		} else {
			tile_bank_switched = FALSE;
		}
	}
	
	if (name_table_switched) {
		if (memcmp(&lazy_VRAM_banks[8], &PPU_VRAM_banks[8], sizeof(lazy_VRAM_banks[0]) * 4)) {
			lazy_bg_need_update = TRUE;
		}
	}

	//if (lazy_bg_need_update) {
	// we should always update BG before SPR
	if (lazy_bg_need_update || lazy_spr_need_update) {
		if (lazy_bg_enabled) {
			lazy_update_bg();
		}
		lazy_bg_start_line = current_frame_line;
		lazy_bg_need_update = FALSE;
		lazy_bg_enabled = bg_enabled();
		lazy_bg_pattern_table_addr = bg_pattern_table_addr;
		lazy_v = loopy_v;
		lazy_t = loopy_t;
		lazy_x = loopy_x;
		memcpy(lazy_bg_pal, bg_pal, sizeof(lazy_bg_pal));
		lazy_bg_pal[4] = lazy_bg_pal[8] = lazy_bg_pal[12] = lazy_bg_pal[0];
	}
	
	if (lazy_spr_need_update) {
		if (lazy_spr_enabled) {
			lazy_update_spr();
		}
		lazy_spr_start_line = current_frame_line;
		lazy_spr_need_update = FALSE;
		lazy_spr_enabled = spr_enabled();
		lazy_spr_pattern_table_addr = spr_pattern_table_addr;
		memcpy(lazy_spr_pal, spr_pal, sizeof(lazy_spr_pal));
		lazy_spr_pal[4] = lazy_spr_pal[8] = lazy_spr_pal[12] = lazy_spr_pal[0];
	}
	
	if (tile_bank_switched) {
		memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(PPU_tile_banks));
		tile_bank_switched = FALSE;
	}
	
	if (name_table_switched) {
		lazy_VRAM_banks[8] = PPU_VRAM_banks[8];
		lazy_VRAM_banks[9] = PPU_VRAM_banks[9];
		lazy_VRAM_banks[10] = PPU_VRAM_banks[10];
		lazy_VRAM_banks[11] = PPU_VRAM_banks[11];
		name_table_switched = FALSE;
	}

	// if sprite 0 flag not set and sprite 0 on current line
	/*
	if((!sprite0_hit()) && 
		(current_frame_line >= ((uint32)(spr_ram[0]+1))) && 
		(current_frame_line <  ((uint32)(spr_ram[0]+1+(sprites_8x16()?16:8))))
		)
	{
		// render line to dummy buffer
		do_scanline_and_draw(dummy_buffer);
	}
	else
	{*/
	if(spr_enabled() || bg_enabled())
	{
		LOOPY_SCANLINE_START(loopy_v, loopy_t);
		// if sprite 0 flag not set and sprite 0 on current line
		if(!sprite0_hit()
			&& spr_enabled()
			//&& bg_enabled()
			&& (current_frame_line >= ((uint32)(spr_ram[0]+1)))
			&& (current_frame_line <  ((uint32)(spr_ram[0]+1+(sprites_8x16()?16:8))))
			)
		{
			fast_test_sprite0_hit();
		}

		LOOPY_NEXT_LINE(loopy_v);
	}
	current_frame_line++;
	//}
}

void NES_PPU::lazy_do_scanline_dont_draw()
{
	// mmc2 / punchout -- we must simulate the ppu for every line
	/*if(parent_NES->ROM->get_mapper_num() == 9)
	{
		do_scanline_and_draw(dummy_buffer);
	}
	else*/

	if(spr_enabled() || bg_enabled())
	{
		LOOPY_SCANLINE_START(loopy_v, loopy_t);
		// if sprite 0 flag not set and sprite 0 on current line
		if(!sprite0_hit()
			&& spr_enabled()
			//&& bg_enabled()
			&& (current_frame_line >= ((uint32)(spr_ram[0]+1)))
			&& (current_frame_line <  ((uint32)(spr_ram[0]+1+(sprites_8x16()?16:8))))
			)
		{
			fast_test_sprite0_hit();
		}

		LOOPY_NEXT_LINE(loopy_v);
	}
	current_frame_line++;
}

void NES_PPU::end_frame(uint8 *buf)
{
	if (lazy_bg_enabled) {
		lazy_update_bg();
	}
	if (lazy_spr_enabled) {
		lazy_update_spr();
	}
	ppu_buf = 0;
}

void NES_PPU::start_vblank()
{
  in_vblank = 1;

  // set vblank register flag
  LowRegs[2] |= 0x80;
}

void NES_PPU::end_vblank()
{
  in_vblank = 0;

  // reset vblank register flag and sprite0 hit flag1
  LowRegs[2] &= 0x3F;
}


// these functions read from/write to VRAM using loopy_v
uint8 NES_PPU::read_2007()
{
  uint16 addr;
  uint8 temp;

  addr = loopy_v;
  loopy_v += ppu_addr_inc;

  ASSERT(addr < 0x4000);
  addr &= 0x3FFF;

  if(addr >= 0x3000)
  {
    // is it a palette entry?
    if(addr >= 0x3F00)
    {
      // palette

      // handle palette mirroring
      if(0x0000 == (addr & 0x0010))
      {
        // background palette
        return bg_pal[addr & 0x000F];
      }
      else
      {
        // sprite palette
        return spr_pal[addr & 0x000F];
      }
    }

    // handle mirroring
    addr &= 0xEFFF;
  }

  temp = read_2007_buffer;
  read_2007_buffer = VRAM(addr);

  return temp;
}

void NES_PPU::write_2007(uint8 data)
{
  uint16 addr;

  addr = loopy_v;
  loopy_v += ppu_addr_inc;

  addr &= 0x3FFF;

//  LOG("PPU 2007 WRITE: " << HEX(addr,4) << " " << HEX(data,2) << endl);

  if(addr >= 0x3000)
  {
    // is it a palette entry?
    if(addr >= 0x3F00)
    {
      // palette
      data &= 0x3F;

      if(0x0000 == (addr & 0x000F)) // is it THE 0 entry?
      {
        bg_pal[0] = spr_pal[0] = data;
		lazy_bg_need_update = lazy_spr_need_update = TRUE;
      }
      else if(0x0000 == (addr & 0x0010))
      {
        // background palette
		if (bg_pal[addr & 0x000F] != data) {
			bg_pal[addr & 0x000F] = data;
			lazy_bg_need_update = TRUE;
		}
      }
      else
      {
        // sprite palette
		if (spr_pal[addr & 0x000F] != data) {
			spr_pal[addr & 0x000F] = data;
			lazy_spr_need_update = TRUE;
		}
      }

      return;
    }

    // handle mirroring
    addr &= 0xEFFF;
  }

  if(!(vram_write_protect && addr < 0x2000))
  {
    VRAM(addr) = data;
  }
  if (!vram_write_protect && addr < 0x2000) {
    //tiles.updateTile(addr, data);
	update_tile(addr, data);
  }
}

#define UPDATE_4_PIXELS() \
	col = 0; \
	if(data & pattern_mask) col |= (bit << 6); \
	pattern_mask >>= 1; \
	if(data & pattern_mask) col |= (bit << 4); \
	pattern_mask >>= 1; \
	if(data & pattern_mask) col |= (bit << 2); \
	pattern_mask >>= 1; \
	if(data & pattern_mask) col |= (bit << 0); \
	*p++ |= col;

void NES_PPU::update_tile(int byte_offset, uint8 data)
{
	//int tileNum = byte_offset >> 4;
	int line = byte_offset & 0xf;

	uint8 *p = TILE(byte_offset) + TILE_OFFSET(line & 0x7);
	uint8 bit;

	if (line < 8) {
		// low pattern
		bit = 0x01;
		*(uint16 *)p &= 0xaaaa;
	} else {
		// high pattern
		bit = 0x02;
		*(uint16 *)p &= 0x5555;
	}

	uint8 pattern_mask = 0x80;
	int col;

	UPDATE_4_PIXELS();
	pattern_mask >>= 1;
	UPDATE_4_PIXELS();
}

uint8 NES_PPU::ReadLowRegs(uint32 addr)
{
  ASSERT((addr >= 0x2000) && (addr < 0x2008));

//  LOG("PPU Read " << HEX(addr,4) << endl);

  //switch(addr)
  switch(addr & 0x7)
  {
    //case 0x2002:
    case 0x2:
      {
        uint8 temp;

        // clear toggle
        toggle_2005_2006 = 0;

        temp = LowRegs[2];

        // clear v-blank flag
        LowRegs[2] &= 0x7F;

        return temp;
      }
      break;

    //case 0x2007:
    case 0x7:
      return read_2007();
      break;

  }

  return LowRegs[addr & 0x0007];
}

void  NES_PPU::WriteLowRegs(uint32 addr, uint8 data)
{
  ASSERT((addr >= 0x2000) && (addr < 0x2008));

//  LOG("PPU Write " << HEX(addr,4) << " = " << HEX(data,2) << endl);

  LowRegs[addr & 0x0007] = data;

  //switch(addr)
  switch(addr & 0x7)
  {
    //case 0x2000:
    case 0:
	{
      bg_pattern_table_addr  = (data & 0x10) ? 0x1000 : 0x0000;
      spr_pattern_table_addr = (data & 0x08) ? 0x1000 : 0x0000;
      ppu_addr_inc = (data & 0x04) ? 32 : 1;

      uint32 t = loopy_t;
	  // t:0000110000000000=d:00000011
      loopy_t = (loopy_t & 0xF3FF) | (((uint16)(data & 0x03)) << 10);

	  //if (ppu_buf && t != loopy_t)
	  if (bg_pattern_table_addr != lazy_bg_pattern_table_addr || t != loopy_t) {
		  lazy_bg_need_update = TRUE;
	  }
	  if (spr_pattern_table_addr != lazy_spr_pattern_table_addr) {
		  lazy_spr_need_update = TRUE;
	  }

      break;
	}

	// Rick, lazy updating stuff
	//case 0x2001:
    case 1:
		/* if (ppu_buf) {
		{
		  int bgenabled = bg_enabled();
		  if (bgenabled && !lazy_bg_enabled) {
			  lazy_bg_start_line = current_frame_line;
			  lazy_bg_enabled = TRUE;
		  } else if (!bgenabled && lazy_bg_enabled) {
			  lazy_bg_need_update = TRUE;
		  }

		  int sprenabled = spr_enabled();
		  if (sprenabled && !lazy_spr_enabled) {
			  lazy_spr_start_line = current_frame_line;
			  lazy_spr_enabled = TRUE;
		  } else if (!sprenabled && lazy_spr_enabled) {
			  lazy_spr_need_update = TRUE;
		  }
		} */
		if (bg_enabled() != lazy_bg_enabled) {
			lazy_bg_need_update = TRUE;
		}
		if (spr_enabled() != lazy_spr_enabled) {
			lazy_spr_need_update = TRUE;
		}
	  break;

    //case 0x2003:
    case 3:
      spr_ram_rw_ptr = data;
      break;

    //case 0x2004:
    case 4:
      spr_ram[spr_ram_rw_ptr++] = data;
      break;

    //case 0x2005:
    case 5:
      toggle_2005_2006 = !toggle_2005_2006;

      if(toggle_2005_2006)
      {
		uint16 t = loopy_t;
		uint8 x = loopy_x;
        // first write
        
        // t:0000000000011111=d:11111000
        loopy_t = (loopy_t & 0xFFE0) | (((uint16)(data & 0xF8)) >> 3);

        // x=d:00000111
        loopy_x = data & 0x07;

        //if (ppu_buf && (t != loopy_t || x != loopy_x))
		if (t != loopy_t || x != loopy_x)
		  lazy_bg_need_update = TRUE;
      }
      else
      {
        uint16 t = loopy_t;
        // second write

        // t:0000001111100000=d:11111000
        loopy_t = (loopy_t & 0xFC1F) | (((uint16)(data & 0xF8)) << 2);
	      
        // t:0111000000000000=d:00000111
        loopy_t = (loopy_t & 0x8FFF) | (((uint16)(data & 0x07)) << 12);
        //if (ppu_buf && t != loopy_t)
		if (t != loopy_t)
		  lazy_bg_need_update = TRUE;
      }

      break;

    //case 0x2006:
    case 6:
      toggle_2005_2006 = !toggle_2005_2006;

      if(toggle_2005_2006)
      {
        // first write

	      // t:0011111100000000=d:00111111
	      // t:1100000000000000=0
        loopy_t = (loopy_t & 0x00FF) | (((uint16)(data & 0x3F)) << 8);
      }
      else
      {
        // second write
        // t:0000000011111111=d:11111111
        loopy_t = (loopy_t & 0xFF00) | ((uint16)data);

	      // v=t
        loopy_v = loopy_t;
      }
	  //if (ppu_buf)
		  lazy_bg_need_update = 1;
      break;

    //case 0x2007:
    case 7:
      write_2007(data);
      break;
  }
}

uint8 NES_PPU::Read0x4014()
{
  return HighReg0x4014;
}

void NES_PPU::Write0x4014(uint8 data)
{
  uint32 addr;

//  LOG("PPU Write 0x4014 = " << HEX(data,2) << endl);

  HighReg0x4014 = data;

  addr = ((uint32)data) << 8;

  // do SPR-RAM DMA
  /*
  for(uint32 i = 0; i < 256; i++)
  {
    spr_ram[i] = parent_NES->cpu->GetByte(addr++);
  }
  */
  memcpy(spr_ram, parent_NES->cpu->Get_DMA_mem_ptr(addr), 256);
}


#define NEW_BG_PIXEL() \
	if (col) {\
		col |= attrib_bits; \
		*solid = BG_WRITTEN_FLAG; \
	} else { \
		*solid = 0; \
	} \
    *p = NES::NES_COLOR_BASE + bg_pal[col]; \
	solid++; \
	p++; \


void NES_PPU::render_bg(uint8* buf)
{
    uint8 *p;
    uint32 i;
    
    //uint16 *solid;
    uint8 *solid;
    
    //uint8 col;
    uint32 col;
    
    uint32 tile_x; // pixel coords within nametable
    uint32 tile_y;
    uint32 name_addr;
    
    uint32 pattern_addr;
    /*
    uint8  pattern_lo;
    uint8  pattern_hi;
    uint8  pattern_mask;*/
    
    uint32 attrib_addr;
    //uint8  attrib_bits;
    uint32 attrib_bits;
    
    tile_x = (loopy_v & 0x001F);
    tile_y = (loopy_v & 0x03E0) >> 5;
    
    name_addr = 0x2000 + (loopy_v & 0x0FFF);
    
    attrib_addr = 0x2000 + (loopy_v & 0x0C00) + 0x03C0 + ((tile_y & 0xFFFC)<<1) + (tile_x>>2);
    if(0x0000 == (tile_y & 0x0002)) {
        if(0x0000 == (tile_x & 0x0002))
            attrib_bits = (VRAM(attrib_addr) & 0x03) << 2;
        else
            attrib_bits = (VRAM(attrib_addr) & 0x0C);
    } else {
        if(0x0000 == (tile_x & 0x0002))
            attrib_bits = (VRAM(attrib_addr) & 0x30) >> 2;
        else
            attrib_bits = (VRAM(attrib_addr) & 0xC0) >> 4;
    }
    
    p     = buf       + (SIDE_MARGIN - loopy_x);
    solid = solid_buf + (SIDE_MARGIN - loopy_x); // set "solid" buffer ptr
    
    uint32 line = (loopy_v & 0x7000) >> 12;
    
    // draw 33 tiles
    for(i = 33; i; i--)
    {
        //uint32 this_tile = VRAM(name_addr);
        pattern_addr = bg_pattern_table_addr + ((int32)VRAM(name_addr) << 4) + line;
        //this_tile |= (attrib_bits << 8);
        
        if (mapper_num == 5) {
            if(uint8 MMC5_pal = parent_NES->mapper->PPU_Latch_RenderScreen(1,name_addr & 0x03FF))	{
                attrib_bits = MMC5_pal & 0x0C;
            }
        }
        
        //const uint8 *data = tile_data + (pattern_addr >> 4) * BYTES_PER_MERGED_TILE + line * 8;
        uint8 *data = TILE(pattern_addr) + TILE_OFFSET(line);
        
        CHECK_MMC2(pattern_addr);
        
        int col2;
        
        col2 = *data++;
        col = col2 >> 6;
        NEW_BG_PIXEL();
        col = (col2 >> 4) & 0x03;
        NEW_BG_PIXEL();
        col = (col2 >> 2) & 0x03;
        NEW_BG_PIXEL();
        col = col2 & 0x03;
        NEW_BG_PIXEL();
        
        col2 = *data++;
        col = col2 >> 6;
        NEW_BG_PIXEL();
        col = (col2 >> 4) & 0x03;
        NEW_BG_PIXEL();
        col = (col2 >> 2) & 0x03;
        NEW_BG_PIXEL();
        col = col2 & 0x03;
        NEW_BG_PIXEL();
        
        tile_x++;
        name_addr++;
        
        // are we crossing a dual-tile boundary?
        if(0x0000 == (tile_x & 0x0001))
        {
            // are we crossing a quad-tile boundary?
            if(0x0000 == (tile_x & 0x0003))
            {
                // are we crossing a name table boundary?
                if(0x0000 == (tile_x & 0x001F))
                {
                    name_addr ^= 0x0400; // switch name tables
                    attrib_addr ^= 0x0400;
                    name_addr -= 0x0020;
                    attrib_addr -= 0x0008;
                    tile_x -= 0x0020;
                }
                
                attrib_addr++;
            }
            
            if(0x0000 == (tile_y & 0x0002)) {
                if(0x0000 == (tile_x & 0x0002))
                    attrib_bits = (VRAM(attrib_addr) & 0x03) << 2;
                else
                    attrib_bits = (VRAM(attrib_addr) & 0x0C);
            } else {
                if(0x0000 == (tile_x & 0x0002))
                    attrib_bits = (VRAM(attrib_addr) & 0x30) >> 2;
                else
                    attrib_bits = (VRAM(attrib_addr) & 0xC0) >> 4;
            }
        }
    }
    
    if(bg_clip_left8())
    {
        // clip left 8 pixels
        memset(buf + SIDE_MARGIN, NES::NES_COLOR_BASE + bg_pal[0], 8);
        memset(solid + SIDE_MARGIN, 0, sizeof(solid[0])*8);
    }
}

void NES_PPU::render_spr(uint8* buf)
{
	int32 s;              // sprite #
	int32  spr_x;         // sprite coordinates
	uint32 spr_y;
	uint8* spr;           // pointer to sprite RAM entry
	uint8* p;             // draw pointer
	
	//uint32 *solid;
	uint8 *solid;
	uint32 priority;
	
	int32 inc_x;           // drawing vars
	int32 start_x, end_x;
	int32 x,y;             // in-sprite coords
	
	uint32 num_sprites = 0;
	
	uint32 spr_height;
	
	spr_height = sprites_8x16() ? 16 : 8;

    // for MMC5 VROM switch
    parent_NES->mapper->PPU_Latch_RenderScreen(0,0);

	//for(s = 0; s < 64; s++)
	for(s = 0, spr = spr_ram; s < 64; s++, spr+=4)
	{
		//spr = &spr_ram[s<<2];
		
		// get y coord
		spr_y = spr[0]+1;
		
		// on current scanline?
		if((spr_y > current_frame_line) || ((spr_y+(spr_height)) <= current_frame_line))
			continue;
		
		num_sprites++;
		if(num_sprites > 8)
		{
			if(!NESTER_settings.nes.graphics.show_more_than_8_sprites) break;
		}
		
		// get x coord
		spr_x = spr[3];
		
		start_x = 0;
		end_x = 8;
		
		// clip right
		if((spr_x + 7) > 255)
		{
			end_x -= ((spr_x + 7) - 255);
		}
		
		// clip left
		if((spr_x < 8) && (spr_clip_left8()))
		{
			if(0 == spr_x) continue;
			start_x += (8 - spr_x);
		}
		
		y = current_frame_line - spr_y;
		
		CHECK_MMC2(spr[1] << 4);
		
		// calc offsets into buffers
		p = &buf[SIDE_MARGIN + spr_x + start_x];
		solid = &solid_buf[SIDE_MARGIN + spr_x + start_x];
		
		// flip horizontally?
		if(spr[2] & 0x40) // yes
		{
			inc_x = -1;
			start_x = (8-1) - start_x;
			end_x = (8-1) - end_x;
		}
		else
		{
			inc_x = 1;
		}
		
		// flip vertically?
		if(spr[2] & 0x80) // yes
		{
			y = (spr_height-1) - y;
		}
		int line = y & 7;
		
		// get priority bit
		priority = spr[2] & 0x20;
		
		uint32 tile_addr = spr[1] << 4;
		if(sprites_8x16()) {
			if(spr[1] & 0x01) {
				tile_addr += 0x1000;
				if(y < 8) tile_addr -= 16;
			} else {
				if(y >= 8) tile_addr += 16;
			}
		} else {
			tile_addr += spr_pattern_table_addr;
		}
		
		// read 16bits = 2bits x 8pixels
		uint8 *t = TILE(tile_addr) + TILE_OFFSET(line);
		uint32 pattern = ((uint32)*t << 8) | *(t + 1);
		uint32 attrib_bits = (spr[2] & 0x03) << 2;
		
		for(x = start_x; x != end_x; x += inc_x)
		{
			//uint8 col = 0x00;
			uint32 col;
			
			// if a sprite has drawn on this pixel, don't draw anything
			if(!((*solid) & SPR_WRITTEN_FLAG))
			{
				//col = *(TILE(tile_addr) + TILE_OFFSET(line) + ((x & 0x7) >> 2));
				col = pattern >> ((7 - (x & 7)) * 2);
				col &= 0x03;
				if (col) {
					col |= attrib_bits;
					
					// set sprite 0 hit flag
					if(!s)
					{
						if((*solid) & BG_WRITTEN_FLAG)
						{
							LowRegs[2] |= 0x40;
						}
					}
					
					(*solid) |= SPR_WRITTEN_FLAG;
					if(priority)
					{
						//(*solid) |= SPR_WRITTEN_FLAG;
						if(!((*solid) & BG_WRITTEN_FLAG))
						{
							*p = NES::NES_COLOR_BASE + spr_pal[col];
						}
					}
					else
					{
						//if(!((*solid) & SPR_WRITTEN_FLAG))
						//{
						*p = NES::NES_COLOR_BASE + spr_pal[col];
						//(*solid) |= SPR_WRITTEN_FLAG;
						//}
					}
				}
			}
			
			p++;
			solid++;
		}
	}

	if(num_sprites >= 8)
	{
		LowRegs[2] |= 0x20;
	}
	else
	{
		LowRegs[2] &= 0xDF;
	}
}

#undef NEW_BG_PIXEL

// fast sprite 0 hit test, no real rendering
void NES_PPU::fast_test_sprite0_hit()
{
	uint8 *buf = dummy_buffer;
	uint8 *p;
	uint32 i;
	
	if (bg_enabled()) {
		
		uint32 col;
		
		uint32 tile_x; // pixel coords within nametable
		uint32 tile_y;
		uint32 name_addr;
		
		uint32 pattern_addr;
		
		tile_x = (loopy_v & 0x001F);
		tile_y = (loopy_v & 0x03E0) >> 5;
		
		name_addr = 0x2000 + (loopy_v & 0x0FFF);
		
		p = buf + (SIDE_MARGIN - loopy_x);
		
		uint32 line = (loopy_v & 0x7000) >> 12;
		
		// draw 33 tiles
		for(i = 33; i; i--)
		{
			uint32 this_tile = VRAM(name_addr);
			pattern_addr = bg_pattern_table_addr + ((int32)this_tile << 4) + line;
			
			uint8 *data = TILE(pattern_addr) + TILE_OFFSET(line);
			//CHECK_MMC2(pattern_addr);
			
			col = *(uint16 *)data;
			
			p[0] = (col >> 6) & 0x03;
			p[1] = (col >> 4) & 0x03;
			p[2] = (col >> 2) & 0x03;
			p[3] = col & 0x03;
			p[4] = col >> 14;
			p[5] = (col >> 12) & 0x03;
			p[6] = (col >> 10) & 0x03;
			p[7] = (col >> 8) & 0x03;
			
			p += 8;
			
			tile_x++;
			name_addr++;
			
			// are we crossing a dual-tile boundary?
			if(0x0000 == (tile_x & 0x0001))
			{
				// are we crossing a quad-tile boundary?
				if(0x0000 == (tile_x & 0x0003))
				{
					// are we crossing a name table boundary?
					if(0x0000 == (tile_x & 0x001F))
					{
						name_addr ^= 0x0400; // switch name tables
						name_addr -= 0x0020;
						tile_x -= 0x0020;
					}
				}
			}
		}
		
		if(bg_clip_left8())
		{
			// clip left 8 pixels
			memset(buf + SIDE_MARGIN, 0, 8);
		}
	} else {	// not bg_enabled()
		memset(buf, 0, NES_BACKBUF_WIDTH);
	}

	int32  spr_x;         // sprite coordinates
	uint32 spr_y;
	uint8* spr = spr_ram; // pointer to sprite 0
	
	int32 inc_x;           // drawing vars
	int32 start_x, end_x;
	int32 x,y;             // in-sprite coords
	
	uint32 spr_height = sprites_8x16() ? 16 : 8;

	// get y coord
	spr_y = spr[0]+1;
		
	// get x coord
	spr_x = spr[3];
		
	start_x = 0;
	end_x = 8;
		
	// clip right
	if((spr_x + 7) > 255)
	{
		end_x -= ((spr_x + 7) - 255);
	}
		
	// clip left
	if((spr_x < 8) && (spr_clip_left8()))
	{
		if(0 == spr_x) return;
		start_x += (8 - spr_x);
	}
		
	y = current_frame_line - spr_y;
		
	//CHECK_MMC2(spr[1] << 4);
		
	// calc offsets into buffers
	p = &buf[SIDE_MARGIN + spr_x + start_x];
		
	// flip horizontally?
	if(spr[2] & 0x40) // yes
	{
		inc_x = -1;
		start_x = (8-1) - start_x;
		end_x = (8-1) - end_x;
	}
	else
	{
		inc_x = 1;
	}
		
	// flip vertically?
	if(spr[2] & 0x80) // yes
	{
		y = (spr_height-1) - y;
	}
	uint32 line = y & 7;
		
	uint32 tile_addr = spr[1] << 4;
	if(sprites_8x16()) {
		if(spr[1] & 0x01) {
			tile_addr += 0x1000;
			if(y < 8) tile_addr -= 16;
		} else {
			if(y >= 8) tile_addr += 16;
		}
	} else {
		tile_addr += spr_pattern_table_addr;
	}
		
	// read 16bits = 2bits x 8pixels
	uint8 *t = TILE(tile_addr) + TILE_OFFSET(line);
	uint32 pattern = ((uint32)*t << 8) | *(t + 1);

	for(x = start_x; x != end_x; x += inc_x)
	{
		//uint8 col = 0x00;
		uint32 col;
		
		col = pattern >> ((7 - (x & 7)) * 2);
		col &= 0x03;

		if (col && *p) {
			// set sprite 0 hit flag
			LowRegs[2] |= 0x40;
			return;
		}
		p++;
	}
}

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#undef TILE

#define TILE(addr) \
  (lazy_tile_banks[(addr) >> 10] + ((addr) & 0x3FF) / 16 * BYTES_PER_MERGED_TILE)

#undef VRAM
#define VRAM(addr) \
	lazy_VRAM_banks[(addr) >> 10][(addr) & 0x3FF]

void NES_PPU::lazy_update_bg()
{
    LOOPY_SCANLINE_START(lazy_v, lazy_t);

	uint8 *p;
	uint32 i;
	
	uint32 tile_x; // pixel coords within nametable
	uint32 tile_y;
	uint32 name_addr;
	
	uint32 pattern_addr;
	
	uint32 attrib_addr;
	uint32 attrib_bits;

	uint8 * buf_line_start = ppu_buf + lazy_bg_start_line * ppu_ypitch;

	while (lazy_bg_start_line < current_frame_line) {
		name_addr = 0x2000 + (lazy_v & 0x0FFF);
		tile_y = (lazy_v & 0x03E0) >> 5;
		tile_x = (lazy_v & 0x001F);
		
		attrib_addr = 0x2000 + (lazy_v & 0x0C00) + 0x03C0 + ((tile_y & 0xFFFC)<<1) + (tile_x>>2);
		if(0x0000 == (tile_y & 0x0002)) {
			if(0x0000 == (tile_x & 0x0002))
				attrib_bits = (VRAM(attrib_addr) & 0x03) << 2;
			else
				attrib_bits = (VRAM(attrib_addr) & 0x0C);
		} else {
			if(0x0000 == (tile_x & 0x0002))
				attrib_bits = (VRAM(attrib_addr) & 0x30) >> 2;
			else
				attrib_bits = (VRAM(attrib_addr) & 0xC0) >> 4;
		}
		uint8 * fast_pal = &lazy_bg_pal[attrib_bits];
		
		uint8 * buf = buf_line_start + (SIDE_MARGIN - lazy_x);
		
		uint32 line = (lazy_v & 0x7000) >> 12;

		uint32 lines_to_draw = MIN(8 - line, current_frame_line - lazy_bg_start_line);

		// draw 33 tiles
		for(i = 33; i; i--) {
			uint32 this_tile = VRAM(name_addr);
			//pattern_addr = bg_pattern_table_addr + ((int32)this_tile << 4) + line;
			pattern_addr = lazy_bg_pattern_table_addr + ((int32)this_tile << 4) + line;
			
			uint8 *data;
			uint8 **tile_banks;
			if (mapper_num == 5) {
				if(uint8 MMC5_pal = parent_NES->mapper->PPU_Latch_RenderScreen(1,name_addr & 0x03FF))
				{
					attrib_bits = MMC5_pal & 0x0C;
					fast_pal = &lazy_bg_pal[attrib_bits];
					//memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(lazy_tile_banks));
				}
				tile_banks = PPU_tile_banks;
			} else {
				/*
				if(((name_addr) & 0x0FC0) == 0x0FC0) {
					if((((name_addr) & 0x0FF0) == 0x0FD0) || (((name_addr) & 0x0FF0) == 0x0FE0)) {
						parent_NES->mapper->PPU_Latch_FDFE(name_addr);
					}
				}
				memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(lazy_tile_banks));
				*/

				tile_banks = lazy_tile_banks;
			}
			data = (tile_banks[pattern_addr >> 10] + (pattern_addr & 0x3FF) / 16 * BYTES_PER_MERGED_TILE) + (line) * (BYTES_PER_MERGED_TILE / 8);
			
			p = buf;
			//uint8 *data = TILE(pattern_addr) + TILE_OFFSET(line);
			
			uint32 col;
			
			for (int j = lines_to_draw; j > 0; j--) {
				col = *(uint16 *)data;
				data += 2;
				// TODO: for little endian CPU only
				/*
				*p++ = NES::NES_COLOR_BASE + fast_pal[(col >> 6) & 0x03];
				*p++ = NES::NES_COLOR_BASE + fast_pal[(col >> 4) & 0x03];
				*p++ = NES::NES_COLOR_BASE + fast_pal[(col >> 2) & 0x03];
				*p++ = NES::NES_COLOR_BASE + fast_pal[col & 0x03];
				*p++ = NES::NES_COLOR_BASE + fast_pal[col >> 14];
				*p++ = NES::NES_COLOR_BASE + fast_pal[(col >> 12) & 0x03];
				*p++ = NES::NES_COLOR_BASE + fast_pal[(col >> 10) & 0x03];
				*p++ = NES::NES_COLOR_BASE + fast_pal[(col >> 8) & 0x03];

				p += ppu_ypitch - 8;
				*/
				p[0] = NES::NES_COLOR_BASE + fast_pal[(col >> 6) & 0x03];
				p[1] = NES::NES_COLOR_BASE + fast_pal[(col >> 4) & 0x03];
				p[2] = NES::NES_COLOR_BASE + fast_pal[(col >> 2) & 0x03];
				p[3] = NES::NES_COLOR_BASE + fast_pal[col & 0x03];
				p[4] = NES::NES_COLOR_BASE + fast_pal[col >> 14];
				p[5] = NES::NES_COLOR_BASE + fast_pal[(col >> 12) & 0x03];
				p[6] = NES::NES_COLOR_BASE + fast_pal[(col >> 10) & 0x03];
				p[7] = NES::NES_COLOR_BASE + fast_pal[(col >> 8) & 0x03];

				p += ppu_ypitch;
			}

			buf += 8;	// next tile			
			tile_x++;
			name_addr++;
			
			// are we crossing a dual-tile boundary?
			if(0x0000 == (tile_x & 0x0001)) {
				// are we crossing a quad-tile boundary?
				if(0x0000 == (tile_x & 0x0003)) {
					// are we crossing a name table boundary?
					if(0x0000 == (tile_x & 0x001F)) {
						name_addr ^= 0x0400; // switch name tables
						attrib_addr ^= 0x0400;
						name_addr -= 0x0020;
						attrib_addr -= 0x0008;
						tile_x -= 0x0020;
					}
					
					attrib_addr++;
				}
				
				if(0x0000 == (tile_y & 0x0002)) {
					if(0x0000 == (tile_x & 0x0002))
						attrib_bits = (VRAM(attrib_addr) & 0x03) << 2;
					else
						attrib_bits = (VRAM(attrib_addr) & 0x0C);
				} else {
					if(0x0000 == (tile_x & 0x0002))
						attrib_bits = (VRAM(attrib_addr) & 0x30) >> 2;
					else
						attrib_bits = (VRAM(attrib_addr) & 0xC0) >> 4;
				}
				fast_pal = &lazy_bg_pal[attrib_bits];
			}
		}

		lazy_bg_start_line += lines_to_draw;
		while (lines_to_draw) {
			if(bg_clip_left8()) {
				// clip left 8 pixels
				memset(buf_line_start + SIDE_MARGIN, NES::NES_COLOR_BASE + bg_pal[0], 8);
			}
			buf_line_start += ppu_ypitch;
			LOOPY_NEXT_LINE(lazy_v);
			lines_to_draw--;
		}
	}
}

static uint32 spr_written[NES_PPU::NES_SCREEN_HEIGHT * NES_PPU::NES_SCREEN_WIDTH / 32];

#undef SPR_WRITTEN
#undef SET_SPR_WRITTEN
#define SPR_WRITTEN(ptr) (spr_written[(ptr) / 32] & (1 << (ptr % 32)))
#define SET_SPR_WRITTEN(ptr) spr_written[(ptr) / 32] |= (1 << (ptr % 32))

void NES_PPU::lazy_update_spr()
{
	int32 s;              // sprite #
	int32  spr_x;         // sprite coordinates
	uint32 spr_y;
	uint8* spr;           // pointer to sprite RAM entry
	uint8* p;             // draw pointer
	
	//uint8  *solid;
	uint32 solid_ptr;
	uint32 priority;
	
	int32 inc_x, inc_y;   // drawing vars
	int32 start_x, end_x;
	int32 x,y;            // in-sprite coords
	
	uint32 spr_height;
	
	spr_height = sprites_8x16() ? 16 : 8;
	memset(&spr_written[NES_SCREEN_WIDTH / 32 * lazy_spr_start_line], 0,
		NES_SCREEN_WIDTH / 8 * (current_frame_line - lazy_spr_start_line));

	// for MMC5 VROM switch
	if (mapper_num == 5) {
		parent_NES->mapper->PPU_Latch_RenderScreen(0,0);
		memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(lazy_tile_banks));
	}

	for(s = 0, spr = spr_ram; s < 64; s++, spr+=4)
	{
		//spr = &spr_ram[s<<2];
		
		// get y coord
		spr_y = spr[0]+1;
		
		// on current scanline region?
		if((spr_y > current_frame_line) || ((spr_y+(spr_height)) <= lazy_spr_start_line))
			continue;
		
		// get x coord
		spr_x = spr[3];
		
		start_x = 0;
		end_x = 8;
		
		// clip right
		if((spr_x + 7) > 255)
		{
			end_x -= ((spr_x + 7) - 255);
		}
		
		// clip left
		if((spr_x < 8) && (spr_clip_left8()))
		{
			if(0 == spr_x) continue;
			start_x += (8 - spr_x);
		}

		/*
		int name_addr = spr[1] << 4;
		if(((name_addr) & 0x0FC0) == 0x0FC0) {
			if((((name_addr) & 0x0FF0) == 0x0FD0) || (((name_addr) & 0x0FF0) == 0x0FE0)) {
				parent_NES->mapper->PPU_Latch_FDFE(name_addr);
			}
		}
		memcpy(lazy_tile_banks, PPU_tile_banks, sizeof(lazy_tile_banks));
		*/
		
		// clip top
		if (spr_y < lazy_spr_start_line) {
			y = lazy_spr_start_line - spr_y;
		} else {
			y = 0;
		}
		int lines_to_draw = MIN(spr_height - y, current_frame_line - spr_y);

		// calc offsets into buffers
		p = ppu_buf + ppu_ypitch * (spr_y + y) + SIDE_MARGIN + spr_x + start_x;
		//solid = spr_written + NES_BACKBUF_WIDTH * (spr_y + y) + SIDE_MARGIN + spr_x + start_x;
		solid_ptr = NES_SCREEN_WIDTH * (spr_y + y) + spr_x + start_x;

		// flip horizontally?
		int x_len = end_x - start_x;;
		if(spr[2] & 0x40) // yes
		{
			inc_x = -1;
			start_x = (8-1) - start_x;
			end_x = (8-1) - end_x;
		}
		else
		{
			inc_x = 1;
		}

		// flip vertically?
		if(spr[2] & 0x80) // yes
		{
			y = (spr_height-1) - y;
			inc_y = -1;
		}
		else
		{
			inc_y = 1;
		}
		
		// get priority bit
		priority = spr[2] & 0x20;
		uint8 * fast_pal = &lazy_spr_pal[(spr[2] & 0x03) << 2];

		while (lines_to_draw) {

			int line = y & 7;
			uint32 tile_addr = spr[1] << 4;
			if(sprites_8x16()) {
				if(spr[1] & 0x01) {
					tile_addr += 0x1000;
					if(y < 8) tile_addr -= 16;
				} else {
					if(y >= 8) tile_addr += 16;
				}
			} else {
				tile_addr += lazy_spr_pattern_table_addr;
			}

			// read 16bits = 2bits x 8pixels
			uint8 *t = TILE(tile_addr) + TILE_OFFSET(line);
			uint32 pattern = ((uint32)*t << 8) | *(t + 1);

			if (pattern) {
				for(x = start_x; x != end_x; x += inc_x)
				{
					//uint8 col = 0x00;
					uint32 col;
					
					// if a sprite has drawn on this pixel, don't draw anything
					//if(!(*solid))
					if(!SPR_WRITTEN(solid_ptr))
					{
						//col = *(TILE(tile_addr) + TILE_OFFSET(line) + ((x & 0x7) >> 2));
						col = pattern >> ((7 - (x & 7)) * 2);
						col &= 0x03;
						if (col) {
							//col |= attrib_bits;
							
							//*solid = 1;
							SET_SPR_WRITTEN(solid_ptr);
							if(priority)
							{
								if(*p == NES::NES_COLOR_BASE + fast_pal[0]) // BG color
								{
									*p = NES::NES_COLOR_BASE + fast_pal[col];
								}
							}
							else
							{
								*p = NES::NES_COLOR_BASE + fast_pal[col];
							}
						}
					}
					
					p++;
					//solid++;
					solid_ptr++;
				}
				p += ppu_ypitch - x_len;
				//solid += NES_BACKBUF_WIDTH - x_len;
				solid_ptr += NES_SCREEN_WIDTH - x_len;
			} else {
				p += ppu_ypitch;
				solid_ptr += NES_SCREEN_WIDTH;
			}
			lines_to_draw--;
			y += inc_y;
		}		
	}
}
