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
**	dsound.h -- a small DirectSound work-alike implemented with SDL2 audio.
**
**	Both the game's sound system (WIN32LIB/AUDIO/SOUNDIO.CPP) and the VQA movie player
**	(WINVQ/VQA32/AUDIO.CPP) stream audio into looping "secondary buffers" ahead of the play
**	cursor. Rather than rewrite that (well tested) streaming logic, the port emulates the
**	subset of DirectSound those modules use. See WIN32LIB/SDL/DSOUND.CPP for the mixer.
*/
#ifndef RA_COMPAT_DSOUND_H
#define RA_COMPAT_DSOUND_H
/* Platform structures always use the natural layout, whatever the includer packs to. */
#pragma pack(push)		/* RA_NATURAL_PACKING_BEGIN */
#pragma pack()

#include "windows.h"
#include "mmsystem.h"

#define WAVE_FORMAT_PCM	1

typedef struct tWAVEFORMATEX {
	WORD	wFormatTag;
	WORD	nChannels;
	DWORD	nSamplesPerSec;
	DWORD	nAvgBytesPerSec;
	WORD	nBlockAlign;
	WORD	wBitsPerSample;
	WORD	cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;

typedef struct _DSBUFFERDESC {
	DWORD				dwSize;
	DWORD				dwFlags;
	DWORD				dwBufferBytes;
	DWORD				dwReserved;
	LPWAVEFORMATEX	lpwfxFormat;
} DSBUFFERDESC, *LPDSBUFFERDESC;
typedef const DSBUFFERDESC *LPCDSBUFFERDESC;

typedef struct _DSCAPS {
	DWORD	dwSize;
	DWORD	dwFlags;
} DSCAPS, *LPDSCAPS;

typedef struct _DSBCAPS {
	DWORD	dwSize;
	DWORD	dwFlags;
	DWORD	dwBufferBytes;
	DWORD	dwUnlockTransferRate;
	DWORD	dwPlayCpuOverhead;
} DSBCAPS, *LPDSBCAPS;

/*
**	Result codes.
*/
#define DS_OK						((HRESULT)0)
#define DSERR_GENERIC			((HRESULT)0x80004005)
#define DSERR_INVALIDPARAM		((HRESULT)0x80070057)
#define DSERR_OUTOFMEMORY		((HRESULT)0x8007000E)
#define DSERR_ALLOCATED			((HRESULT)0x8878000A)
#define DSERR_BUFFERLOST		((HRESULT)0x88780096)
#define DSERR_PRIOLEVELNEEDED	((HRESULT)0x88780046)
#define DSERR_INVALIDCALL		((HRESULT)0x88780032)
#define DSERR_NODRIVER			((HRESULT)0x88780078)
#define DSERR_UNSUPPORTED		((HRESULT)0x80004001)

/*
**	Flags.
*/
#define DSSCL_NORMAL				1
#define DSSCL_PRIORITY			2
#define DSSCL_EXCLUSIVE			3
#define DSSCL_WRITEPRIMARY		4

#define DSBCAPS_PRIMARYBUFFER		0x00000001
#define DSBCAPS_STATIC				0x00000002
#define DSBCAPS_LOCHARDWARE		0x00000004
#define DSBCAPS_LOCSOFTWARE		0x00000008
#define DSBCAPS_CTRLFREQUENCY		0x00000020
#define DSBCAPS_CTRLPAN				0x00000040
#define DSBCAPS_CTRLVOLUME			0x00000080
#define DSBCAPS_CTRLDEFAULT		0x000000E0
#define DSBCAPS_STICKYFOCUS		0x00004000
#define DSBCAPS_GLOBALFOCUS		0x00008000
#define DSBCAPS_GETCURRENTPOSITION2	0x00010000

#define DSBPLAY_LOOPING				0x00000001

#define DSBSTATUS_PLAYING			0x00000001
#define DSBSTATUS_BUFFERLOST		0x00000002
#define DSBSTATUS_LOOPING			0x00000004

#define DSBLOCK_FROMWRITECURSOR	0x00000001
#define DSBLOCK_ENTIREBUFFER		0x00000002

#define DSBVOLUME_MIN				-10000
#define DSBVOLUME_MAX				0
#define DSBPAN_LEFT					-10000
#define DSBPAN_CENTER				0
#define DSBPAN_RIGHT					10000

/*
**	The "COM" interfaces. Only the methods the game calls are provided.
*/
struct IDirectSoundBuffer {
	virtual HRESULT	GetStatus(LPDWORD status) = 0;
	virtual HRESULT	GetCaps(LPDSBCAPS caps) = 0;
	virtual HRESULT	GetCurrentPosition(LPDWORD play_cursor, LPDWORD write_cursor) = 0;
	virtual HRESULT	SetCurrentPosition(DWORD position) = 0;
	virtual HRESULT	Lock(DWORD offset, DWORD bytes, LPVOID * ptr1, LPDWORD bytes1, LPVOID * ptr2, LPDWORD bytes2, DWORD flags) = 0;
	virtual HRESULT	Unlock(LPVOID ptr1, DWORD bytes1, LPVOID ptr2, DWORD bytes2) = 0;
	virtual HRESULT	Play(DWORD reserved, DWORD priority, DWORD flags) = 0;
	virtual HRESULT	Stop(void) = 0;
	virtual HRESULT	Restore(void) = 0;
	virtual HRESULT	SetFormat(LPCWAVEFORMATEX format) = 0;
	virtual HRESULT	GetFormat(LPWAVEFORMATEX format, DWORD size, LPDWORD written) = 0;
	virtual HRESULT	SetVolume(LONG volume) = 0;
	virtual HRESULT	GetVolume(LPLONG volume) = 0;
	virtual HRESULT	SetPan(LONG pan) = 0;
	virtual HRESULT	SetFrequency(DWORD frequency) = 0;
	virtual ULONG		Release(void) = 0;
	protected:
		virtual ~IDirectSoundBuffer() {};
};
typedef IDirectSoundBuffer * LPDIRECTSOUNDBUFFER;

struct IDirectSound {
	virtual HRESULT	SetCooperativeLevel(HWND window, DWORD level) = 0;
	virtual HRESULT	CreateSoundBuffer(LPCDSBUFFERDESC desc, LPDIRECTSOUNDBUFFER * buffer, void * outer) = 0;
	virtual HRESULT	GetCaps(LPDSCAPS caps) = 0;
	virtual ULONG		Release(void) = 0;
	protected:
		virtual ~IDirectSound() {};
};
typedef IDirectSound * LPDIRECTSOUND;

HRESULT DirectSoundCreate(void * guid, LPDIRECTSOUND * ds, void * outer);

#pragma pack(pop)		/* RA_NATURAL_PACKING_END */
#endif
