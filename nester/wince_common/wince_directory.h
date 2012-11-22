#ifndef _WINCE_DIRECTORY_H_
#define _WINCE_DIRECTORY_H_

#include "NES_settings.h"
#include "NES_ROM.h"

extern int DIR_createPath(const char* path);
extern void DIR_createFileName(char* buf, const char* basePath,
                               const char* relativePath,
                               const char* fileName, const char* fileExtension);
extern void DIR_createNesFileName(NES_ROM* rom, char* buf, NES_preferences_settings::SAVE_DIR_TYPE dirType,
                                  const char* relativePath, const char* fileName,
                                  const char* fileExtension);


#endif
