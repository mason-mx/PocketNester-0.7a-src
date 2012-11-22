// $Id: profile.h,v 1.4 2003/01/27 13:50:20 Rick Exp $
#ifndef _PROFILE_H_
#define _PROFILE_H_

#ifdef PROFILING

#include "wince_timing.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int cpu_time;
extern int cpu_called_times;
extern int ppu_time;
extern int ppu_called_times;
extern int apu_time;
extern int apu_called_times;
extern int blt_time;
extern int blt_called_times;

#ifdef __cplusplus
}
#endif

#define RESET_PROFILE_DATA() \
	cpu_time = cpu_called_times = 0; \
	apu_time = apu_called_times = 0; \
	ppu_time = ppu_called_times = 0; \
	blt_time = blt_called_times = 0;

#define BEGIN_INSTRUMENTATION(item) \
{								\
	int _##item##_before = SYS_TimeInMilliseconds();

#define END_INSTRUMENTATION(item) \
	item##_time += SYS_TimeInMilliseconds() - _##item##_before; \
	item##_called_times++; \
}

#else

#define RESET_PROFILE_DATA()
#define BEGIN_INSTRUMENTATION(item)
#define END_INSTRUMENTATION(item)

#endif

#endif
