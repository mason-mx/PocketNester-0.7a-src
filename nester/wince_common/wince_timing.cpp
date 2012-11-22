#include "wince_timing.h"
#include "debug.h"

// private variables for the timer functions
static _int64 ticksPerSec, startTime;
static double secsPerTick, millisecsPerTick;
static unsigned long startTimeOldMachine;
static int isOldMachine;
static int initialized = 0;

void SYS_TimeInit()
{
	if(initialized) return;

	if(!QueryPerformanceFrequency((LARGE_INTEGER *)&ticksPerSec))
	{
		// old machine, no performance counter available
		startTimeOldMachine = GetTickCount();
		secsPerTick = ((double)1.0) / ((double)1000.0);
		isOldMachine = 1;

		LOG("Timer Init: QueryPerformanceCounter unavailable" << endl);
	}
	else
	{
		// newer machine, use performance counter
		QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
		secsPerTick = ((double)1.0) / ((double)ticksPerSec);
		millisecsPerTick = ((double)1000.0) / ((double)ticksPerSec);
		isOldMachine = 0;

		LOG("Timer init: " << (double)ticksPerSec << " (" 
					   << (uint32)ticksPerSec << ") ticks per second" << endl);
	}

	initialized = 1;
}

DWORD SYS_TimeInMilliseconds()
{
	_int64 temptime;
	if(!isOldMachine)
	{
		// use performance counter
		QueryPerformanceCounter((LARGE_INTEGER *)&temptime);
		return (DWORD)(((float)(temptime - startTime)) * millisecsPerTick);
	}
	else
	{
		// fall back to timeGetTime
		return (DWORD)(GetTickCount() - startTimeOldMachine);
	}
}
