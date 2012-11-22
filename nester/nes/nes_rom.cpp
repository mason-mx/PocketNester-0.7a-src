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
/* $Id: nes_rom.cpp,v 1.5 2003/10/28 12:58:06 Rick Exp $ */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "NES_ROM.h"

#include "debug.h"
#include "unzip.h"

NES_ROM::NES_ROM(const char* fn)
{
	FILE* fp;
	unzFile zip_file = 0;    
	
	fp         = NULL;
	
	trainer    = NULL;
	ROM_banks  = NULL;
	VROM_banks = NULL;
	VROM_tiles = NULL;
	
	rom_name = NULL;
	rom_path = NULL;
	
	//try 
	PN_TRY
	{
		// store filename and path
		rom_name = (char*)malloc(strlen(fn)+1);
		rom_name_ext = (char*)malloc(strlen(fn)+1);
		rom_path = (char*)malloc(strlen(fn)+1);
		if(!rom_name || !rom_name_ext || !rom_path)
			//throw "Error loading ROM: out of memory";
			THROW_EXCEPTION;
		
		GetPathInfo(fn);
		
		char ext[5] = "";
		char *p;
		p = strrchr(rom_name_ext, '.');
		if (p) {
			strncpy(ext, p, sizeof(ext));
		}
#ifdef _WIN32_WCE
         _strlwr(ext);
#else
		strlwr(ext);
#endif

		if (strcmp(ext, ".zip") != 0) {
			// ".NES"
			
			fp = fopen(fn, "rb");
			if(fp == NULL)
				//throw "Error opening ROM file";
				THROW_EXCEPTION;
			
			if(fread((void*)&header, sizeof(struct NES_header), 1, fp) != 1)
				//throw "Error reading from NES ROM";
				THROW_EXCEPTION;
			
			if((strncmp((const char*)header.id, "NES", 3) && strncmp((const char*)header.id, "NEZ", 3)) || (header.ctrl_z != 0x1A))
				//throw "Invalid NES file";
				THROW_EXCEPTION;
			
			// allocate memory
			ROM_banks = (uint8*)malloc(header.num_16k_rom_banks * (16*1024));
			//if(!ROM_banks) throw "Out of memory";
			if(!ROM_banks) THROW_EXCEPTION;
			
			VROM_banks = (uint8*)malloc(header.num_8k_vrom_banks * (8*1024));
			//if(!VROM_banks) throw "Out of memory";
			if(!VROM_banks) THROW_EXCEPTION;
			
			// load trainer if present
			if(has_trainer())
			{
				trainer = (uint8*)malloc(TRAINER_LEN);
				//if(!trainer) throw "Out of memory";
				if(!trainer) THROW_EXCEPTION;
				
				if(fread(trainer, TRAINER_LEN, 1, fp) != 1)
					//throw "Error reading trainer from NES ROM";
					THROW_EXCEPTION;
			}
			
			if(fread(ROM_banks,(16*1024),header.num_16k_rom_banks,fp) != header.num_16k_rom_banks) 
				//throw "Error reading ROM banks from NES ROM";
				THROW_EXCEPTION;
			
			if(fread(VROM_banks,(8*1024),header.num_8k_vrom_banks,fp) != header.num_8k_vrom_banks) 
				//throw "Error reading VROM banks from NES ROM";
				THROW_EXCEPTION;
			
			VROM_tiles = (uint8 *)malloc(header.num_8k_vrom_banks * 8 * 1024 / 16 * BYTES_PER_MERGED_TILE);
			if (!VROM_tiles) THROW_EXCEPTION;
			
			CompiledTiles::compile(header.num_8k_vrom_banks * 8 * 1024 / 16, VROM_banks, VROM_tiles);
			
			fclose(fp);
		} else {
            // ".ZIP"
            char nes_file[MAX_PATH];
            //unz_file_info dummy;

            /* Open the ZIP file. */
            zip_file = unzOpen(fn);
            
            if (!zip_file)
                //throw "Error opening ROM zip file";
				THROW_EXCEPTION;

            unzGoToFirstFile (zip_file);
            
            for (;;) {
                if (unzGetCurrentFileInfo(zip_file, NULL, nes_file, sizeof(nes_file), NULL, NULL, NULL, NULL) != UNZ_OK)
                    //throw "Error opening ROM ZIP file";
					THROW_EXCEPTION;
#ifdef _WIN32_WCE
                _strlwr(nes_file);
#else
				strlwr(nes_file);
#endif
                char *p = strrchr(nes_file, '.');
                if (strcmp(p, ".nes") == 0) {
                    break;
                }
                if (unzGoToNextFile(zip_file) != UNZ_OK) {
                    //throw "No ROM found in zip file";
					THROW_EXCEPTION;
                }
            }

            unzOpenCurrentFile (zip_file);

            if(unzReadCurrentFile(zip_file, &header, sizeof(struct NES_header)) != sizeof(struct NES_header))
                //throw "Error reading from NES ROM ZIP";
				THROW_EXCEPTION;
            
            if(strncmp((const char*)header.id, "NES", 3) || (header.ctrl_z != 0x1A))
                //throw "Invalid NES file";
				THROW_EXCEPTION;
            
            // allocate memory
            ROM_banks = (uint8*)malloc(header.num_16k_rom_banks * (16*1024));
            if(!ROM_banks)
				//throw "Out of memory";
				THROW_EXCEPTION;
            
            VROM_banks = (uint8*)malloc(header.num_8k_vrom_banks * (8*1024));
            if(!VROM_banks)
				//throw "Out of memory";
				THROW_EXCEPTION;
            
            // load trainer if present
            if(has_trainer())
            {
                trainer = (uint8*)malloc(TRAINER_LEN);
                if(!trainer)
					//throw "Out of memory";
					THROW_EXCEPTION;
                
                if(unzReadCurrentFile(zip_file, trainer, TRAINER_LEN) != TRAINER_LEN)
                    //throw "Error reading trainer from NES ROM ZIP";
					THROW_EXCEPTION;
            }
            
            if(unzReadCurrentFile(zip_file, ROM_banks,(16*1024)*header.num_16k_rom_banks) != (16*1024)*header.num_16k_rom_banks) 
                //throw "Error reading ROM banks from NES ROM ZIP";
				THROW_EXCEPTION;
            
            if(unzReadCurrentFile(zip_file, VROM_banks,(8*1024)*header.num_8k_vrom_banks) != (8*1024)*header.num_8k_vrom_banks) 
                //throw "Error reading VROM banks from NES ROM ZIP";
				THROW_EXCEPTION;
            
            VROM_tiles = (uint8 *)malloc(header.num_8k_vrom_banks * 8 * 1024 / 16 * BYTES_PER_MERGED_TILE);
            //if (!VROM_tiles) THROW_EXCEPTION;
            CompiledTiles::compile(header.num_8k_vrom_banks * 8 * 1024 / 16, VROM_banks, VROM_tiles);
            
            unzCloseCurrentFile (zip_file);
            unzClose (zip_file);
		}
		
	//} catch(...) {
	} PN_CATCH {
		if(fp)          fclose(fp);
		
		if(VROM_banks)  free(VROM_banks);
		if(VROM_tiles)  free(VROM_tiles);
		if(ROM_banks)   free(ROM_banks);
		if(trainer)     free(trainer);
		
		if(rom_name)     free(rom_name);
		if(rom_name_ext) free(rom_name_ext);
		if(rom_path)     free(rom_path);
		//throw;
		THROW_EXCEPTION;
	}
	
	unsigned long c, crctable[256];
	uint32 i, j;
	crc = fds = 0;
	uint32 crc_l = 0;	// keep crc local for faster calc
	
	for(i = 0; i < 256; i++)
	{
		c = i;
		for (j = 0; j < 8; j++)
		{
			if (c & 1)
				c = (c >> 1) ^ 0xedb88320;
			else
				c >>= 1;
		}
		crctable[i] = c;
	}
	
	for(i = 0; i < header.num_16k_rom_banks; i++)
	{
		uint8 *bank = &(ROM_banks[i * 0x4000]);
		c = ~crc_l;
		for(j = 0; j < 0x4000; j++)
			c = crctable[(c ^ bank[j]) & 0xff] ^ (c >> 8);
		crc_l = ~c;
	}
	crc = crc_l;
	
	// figure out mapper number
	mapper = (header.flags_1 >> 4);

	#include "NES_rom_Correct.cpp"
	
	// if there is anything in the reserved bytes,
	// don't trust the high nybble of the mapper number
	for(i = 0; i < sizeof(header.reserved); i++)
	{
		if(header.reserved[i] != 0x00) return;
	}
	mapper |= (header.flags_2 & 0xF0);
}

NES_ROM::~NES_ROM()
{
  if(VROM_banks)  free(VROM_banks);
  if(VROM_tiles)  free(VROM_tiles);
  if(ROM_banks)   free(ROM_banks);
  if(trainer)     free(trainer);
  if(rom_name)     free(rom_name);
  if(rom_name_ext) free(rom_name_ext);
  if(rom_path)     free(rom_path);
}

void NES_ROM::GetPathInfo(const char* fn)
{
  // find index of first letter of actual ROM file name (after path)
  uint32 i = strlen(fn); // start at end of string

  while(1)
  {
    // look for directory delimiter
    if((fn[i] == '\\') || (fn[i] == '/'))
    {
      i++;
      break;
    }

    i--;
    if(!i) break;
  }

  // copy rom name w/o extension
  {
    uint32 j = i;
    uint32 a = 0;

    // copy up to period
    while(1)
    {
      if(!fn[j]) break;
      if(fn[j] == '.') break;

      rom_name[a] = fn[j];

      a++;
      j++;
    }

    // terminate rom name string
    rom_name[a] = '\0';
  }

  // copy rom name w/ extension
  {
    uint32 j = i;
    uint32 a = 0;

    // copy up to period
    while(1)
    {
      if(!fn[j]) break;

      rom_name_ext[a] = fn[j];

      a++;
      j++;
    }

    // terminate rom name string
    rom_name_ext[a] = '\0';
  }

  // copy rom path
  {
    uint32 j = 0;

    // copy up to rom file name
    while(j < i)
    {
      rom_path[j] = fn[j];
      j++;
    }

    // terminate rom path string
    rom_path[i] = '\0';
  }

}