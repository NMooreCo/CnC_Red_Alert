/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	Minimal stand-in for <mmsystem.h>: the multimedia timer API used by the sound, timer and
**	movie code, implemented on top of SDL timers (compat/compat.cpp). As with Windows, the
**	callbacks run on a separate thread.
*/
#ifndef RA_COMPAT_MMSYSTEM_H
#define RA_COMPAT_MMSYSTEM_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()

#include "windows.h"

typedef UINT MMRESULT;
typedef void (CALLBACK *LPTIMECALLBACK)(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2);

#define TIMERR_NOERROR		0
#define TIMERR_NOCANDO		97
#define TIME_ONESHOT			0x0000
#define TIME_PERIODIC		0x0001
#define TIME_CALLBACK_FUNCTION	0x0000

typedef struct timecaps_tag {
	UINT wPeriodMin;
	UINT wPeriodMax;
} TIMECAPS, *LPTIMECAPS;

#ifdef __cplusplus
extern "C" {
#endif
UINT		timeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback, DWORD user, UINT flags);
MMRESULT	timeKillEvent(UINT id);
MMRESULT	timeBeginPeriod(UINT period);
MMRESULT	timeEndPeriod(UINT period);
MMRESULT	timeGetDevCaps(LPTIMECAPS caps, UINT size);
#ifdef __cplusplus
}
#endif

#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
