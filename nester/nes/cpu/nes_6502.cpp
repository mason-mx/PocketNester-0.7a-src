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

#include "NES_6502.h"
#include "NES.h"
#include <stdio.h>
#include "debug.h"

// NOT SAFE FOR MULTIPLE NES_6502'S
static NES_6502 *NES_6502_nes = NULL;

static void NES_write(uint32 address, uint8 value)
{
  NES_6502_nes->MemoryWrite(address, value);
}

static uint8 NES_read(uint32 address)
{
  return NES_6502_nes->MemoryRead(address);
}

static nes6502_memread NESReadHandler[] =
{
   /* $0 - $7FF is RAM */
   { 0x0800, 0xFFFF, NES_read },
   { -1,     -1,     NULL }
};

static nes6502_memwrite NESWriteHandler[] =
{
   /* $0 - $7FF is RAM */
   { 0x0800, 0xFFFF, NES_write },
   { -1,     -1,     NULL}
};


NES_6502::NES_6502(NES* parent) : ParentNES(parent)
{
  //if(NES_6502_nes) throw "error: multiple NES_6502's";
  if(NES_6502_nes) THROW_EXCEPTION;

  //try {
  PN_TRY {
    NES_6502_nes = this;

    Init();
  //} catch(...) {
  } PN_CATCH {
    NES_6502_nes = NULL;
    //throw;
	THROW_EXCEPTION;
  }
}

NES_6502::~NES_6502()
{
  NES_6502_nes = NULL;
}

// Context get/set
void NES_6502::SetContext(Context *cpu)
{
  ASSERT(0x00000000 == (cpu->pc_reg & 0xFFFF0000));
  cpu->read_handler = NESReadHandler;
  cpu->write_handler = NESWriteHandler;
  nes6502_setcontext(cpu);
}

void NES_6502::GetContext(Context *cpu)
{
  nes6502_getcontext(cpu);
  cpu->read_handler = NESReadHandler;
  cpu->write_handler = NESWriteHandler;
}


void NES_6502::Set_CPU_banks(uint8 * bank4, uint8 * bank5, uint8 * bank6, uint8 * bank7)
{
	nes6502_context *cpu = nes6502_getcontextptr();
	cpu->mem_page[4] = bank4;
	cpu->mem_page[5] = bank5;
	cpu->mem_page[6] = bank6;
	cpu->mem_page[7] = bank7;
	nes6502_update_fast_pc();
}

void NES_6502::Set_CPU_banks(uint8 * bank3, uint8 * bank4, uint8 * bank5, uint8 * bank6, uint8 * bank7)
{
	nes6502_context *cpu = nes6502_getcontextptr();
	cpu->mem_page[3] = bank3;
	cpu->mem_page[4] = bank4;
	cpu->mem_page[5] = bank5;
	cpu->mem_page[6] = bank6;
	cpu->mem_page[7] = bank7;
	nes6502_update_fast_pc();
}

void NES_6502::Set_CPU_bank(uint32 num, uint8 *bank)
{
	nes6502_context *cpu = nes6502_getcontextptr();
	cpu->mem_page[num] = bank;
	nes6502_update_fast_pc();
}

uint8 * NES_6502::Get_DMA_mem_ptr(uint32 address)
{
	nes6502_context *cpu = nes6502_getcontextptr();
	return &(cpu->mem_page[address >> NES6502_BANKSHIFT][address & NES6502_BANKMASK]);
}

uint8 NES_6502::MemoryRead(uint32 addr)
{
  return ParentNES->MemoryRead(addr);
}

void NES_6502::MemoryWrite(uint32 addr, uint8 data)
{
  ParentNES->MemoryWrite(addr, data);
}
