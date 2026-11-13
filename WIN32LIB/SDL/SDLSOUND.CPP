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
**	SDLSOUND.CPP -- a DirectSound work-alike built on SDL2 audio (see compat/include/dsound.h).
**
**	All "secondary buffers" created through any IDirectSound object are mixed into a single
**	SDL audio device. Buffers are circular: the game streams decompressed audio into them
**	ahead of the play cursor from its sound maintenance timer, just as it did with DirectSound.
**	Each buffer has its own sample format and is resampled (linear interpolation) to the
**	device rate. Volume and pan use DirectSound's units (hundredths of a decibel).
**
**	Set RA_NO_SOUND=1 to run silently.
*/

#include "dsound.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
extern "C" void Win32_Web_Poll(void);		// compat/win32.cpp
#endif
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

class SDLSoundBuffer;

static SDL_AudioDeviceID			Device = 0;
static int								DeviceRate = 44100;
/*
**	The mixer's state is never destroyed: SDL's audio thread can still be mixing while the
**	program exits and runs static destructors.
*/
static std::vector<SDLSoundBuffer *> &	Buffers = *new std::vector<SDLSoundBuffer *>;
static int								DeviceUsers = 0;

static float Decibels_To_Gain(LONG centibels)
{
	if (centibels >= 0) return(1.0f);
	if (centibels <= DSBVOLUME_MIN) return(0.0f);
	return((float)pow(10.0, (double)centibels / 2000.0));
}

class SDLSoundBuffer : public IDirectSoundBuffer
{
	public:
		SDLSoundBuffer(bool primary, DWORD bytes, LPWAVEFORMATEX format) :
			IsPrimary(primary),
			IsPlaying(false),
			IsLooping(false),
			Position(0),
			Frequency(0),
			Volume(0),
			Pan(0),
			Gain(1.0f),
			LeftGain(1.0f),
			RightGain(1.0f)
		{
			memset(&Format, 0, sizeof(Format));
			if (format) {
				Format = *format;
			} else {
				Format.wFormatTag = WAVE_FORMAT_PCM;
				Format.nChannels = 2;
				Format.nSamplesPerSec = 22050;
				Format.wBitsPerSample = 16;
				Format.nBlockAlign = 4;
				Format.nAvgBytesPerSec = 22050 * 4;
			}
			if (Format.nBlockAlign == 0) Format.nBlockAlign = (WORD)(Format.nChannels * (Format.wBitsPerSample / 8));
			if (!primary) Data.resize(bytes ? bytes : 1, (Format.wBitsPerSample == 8) ? 0x80 : 0);
		}

		virtual ~SDLSoundBuffer() {}

		/*
		**	IDirectSoundBuffer
		*/
		virtual HRESULT GetStatus(LPDWORD status)
		{
			if (status) {
				*status = 0;
				if (IsPrimary || IsPlaying) *status |= DSBSTATUS_PLAYING;
				if (IsPlaying && IsLooping) *status |= DSBSTATUS_LOOPING;
			}
			return(DS_OK);
		}

		virtual HRESULT GetCaps(LPDSBCAPS caps)
		{
			if (caps) {
				caps->dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
				caps->dwBufferBytes = (DWORD)Data.size();
			}
			return(DS_OK);
		}

		virtual HRESULT GetCurrentPosition(LPDWORD play_cursor, LPDWORD write_cursor)
		{
#ifdef __EMSCRIPTEN__
			Win32_Web_Poll();		// In a browser the sound plays only while the page runs.
#endif
			if (IsPrimary || Data.empty()) {
				if (play_cursor) *play_cursor = 0;
				if (write_cursor) *write_cursor = 0;
				return(DS_OK);
			}
			SDL_LockAudioDevice(Device);
			DWORD frames = Frames();
			DWORD frame = (DWORD)(Position >> 32);
			SDL_UnlockAudioDevice(Device);
			if (frames) frame %= frames;
			DWORD play = frame * Format.nBlockAlign;
			/*
			**	DirectSound's write cursor runs a little ahead of the play cursor (data between
			**	them is committed to the hardware). Use about 10 milliseconds.
			*/
			DWORD lead = ((Format.nAvgBytesPerSec / 100) / Format.nBlockAlign) * Format.nBlockAlign;
			if (play_cursor) *play_cursor = play;
			if (write_cursor) *write_cursor = (DWORD)((play + lead) % Data.size());
			return(DS_OK);
		}

		virtual HRESULT SetCurrentPosition(DWORD position)
		{
			if (IsPrimary || Data.empty()) return(DS_OK);
			SDL_LockAudioDevice(Device);
			DWORD frame = (position % (DWORD)Data.size()) / Format.nBlockAlign;
			Position = (Uint64)frame << 32;
			SDL_UnlockAudioDevice(Device);
			return(DS_OK);
		}

		virtual HRESULT Lock(DWORD offset, DWORD bytes, LPVOID * ptr1, LPDWORD bytes1, LPVOID * ptr2, LPDWORD bytes2, DWORD flags)
		{
			if (IsPrimary || Data.empty()) return(DSERR_INVALIDCALL);
			DWORD size = (DWORD)Data.size();
			if (flags & DSBLOCK_FROMWRITECURSOR) {
				DWORD write;
				GetCurrentPosition(NULL, &write);
				offset = write;
			}
			if (flags & DSBLOCK_ENTIREBUFFER) {
				offset = 0;
				bytes = size;
			}
			offset %= size;
			if (bytes > size) bytes = size;

			DWORD first = SDL_min(bytes, size - offset);
			if (ptr1) *ptr1 = &Data[offset];
			if (bytes1) *bytes1 = first;
			if (ptr2) *ptr2 = (bytes > first) ? &Data[0] : NULL;
			if (bytes2) *bytes2 = bytes - first;
			return(DS_OK);
		}

		virtual HRESULT Unlock(LPVOID , DWORD , LPVOID , DWORD )
		{
			return(DS_OK);
		}

		virtual HRESULT Play(DWORD , DWORD , DWORD flags)
		{
			if (IsPrimary) return(DS_OK);
			SDL_LockAudioDevice(Device);
			IsPlaying = true;
			IsLooping = (flags & DSBPLAY_LOOPING) != 0;
			SDL_UnlockAudioDevice(Device);
			return(DS_OK);
		}

		virtual HRESULT Stop(void)
		{
			if (IsPrimary) return(DS_OK);
			SDL_LockAudioDevice(Device);
			IsPlaying = false;
			SDL_UnlockAudioDevice(Device);
			return(DS_OK);
		}

		virtual HRESULT Restore(void)
		{
			return(DS_OK);
		}

		virtual HRESULT SetFormat(LPCWAVEFORMATEX format)
		{
			if (format == NULL) return(DSERR_INVALIDPARAM);
			if (IsPrimary) {
				Format = *format;
				return(DS_OK);
			}
			return(DSERR_INVALIDCALL);
		}

		virtual HRESULT GetFormat(LPWAVEFORMATEX format, DWORD size, LPDWORD written)
		{
			if (format && size >= sizeof(WAVEFORMATEX)) *format = Format;
			if (written) *written = sizeof(WAVEFORMATEX);
			return(DS_OK);
		}

		virtual HRESULT SetVolume(LONG volume)
		{
			Volume = volume;
			Gain = Decibels_To_Gain(volume);
			return(DS_OK);
		}

		virtual HRESULT GetVolume(LPLONG volume)
		{
			if (volume) *volume = Volume;
			return(DS_OK);
		}

		virtual HRESULT SetPan(LONG pan)
		{
			Pan = pan;
			LeftGain = (pan > 0) ? Decibels_To_Gain(-pan) : 1.0f;
			RightGain = (pan < 0) ? Decibels_To_Gain(pan) : 1.0f;
			return(DS_OK);
		}

		virtual HRESULT SetFrequency(DWORD frequency)
		{
			Frequency = frequency;
			return(DS_OK);
		}

		virtual ULONG Release(void);

		/*
		**	Mixing (called on the audio thread with the device locked).
		*/
		DWORD Frames(void) const {return(Format.nBlockAlign ? (DWORD)(Data.size() / Format.nBlockAlign) : 0);}

		inline void Sample(DWORD frame, float & left, float & right) const
		{
			unsigned char const * p = &Data[frame * Format.nBlockAlign];
			if (Format.wBitsPerSample == 16) {
				Sint16 l, r;
				memcpy(&l, p, 2);
				if (Format.nChannels > 1) {
					memcpy(&r, p + 2, 2);
				} else {
					r = l;
				}
				left = l / 32768.0f;
				right = r / 32768.0f;
			} else {
				left = ((int)p[0] - 128) / 128.0f;
				right = (Format.nChannels > 1) ? ((int)p[1] - 128) / 128.0f : left;
			}
		}

		void Mix(float * out, int frames)
		{
			if (!IsPlaying || IsPrimary) return;
			DWORD count = Frames();
			if (count == 0) {
				IsPlaying = false;
				return;
			}
			DWORD rate = Frequency ? Frequency : Format.nSamplesPerSec;
			Uint64 step = ((Uint64)rate << 32) / (Uint64)DeviceRate;
			float lg = Gain * LeftGain;
			float rg = Gain * RightGain;
			Uint64 end = (Uint64)count << 32;

			for (int i = 0; i < frames; i++) {
				if (Position >= end) {
					if (IsLooping) {
						Position %= end;
					} else {
						IsPlaying = false;
						Position = 0;
						return;
					}
				}
				DWORD frame = (DWORD)(Position >> 32);
				float frac = (float)(Position & 0xFFFFFFFF) / 4294967296.0f;
				DWORD next = frame + 1;
				if (next >= count) next = IsLooping ? 0 : frame;
				float l0, r0, l1, r1;
				Sample(frame, l0, r0);
				Sample(next, l1, r1);
				out[i*2+0] += (l0 + (l1 - l0) * frac) * lg;
				out[i*2+1] += (r0 + (r1 - r0) * frac) * rg;
				Position += step;
			}
		}

		bool				IsPrimary;
		bool				IsPlaying;
		bool				IsLooping;
		Uint64			Position;		// 32.32 fixed point frame position
		DWORD				Frequency;
		LONG				Volume;
		LONG				Pan;
		float				Gain;
		float				LeftGain;
		float				RightGain;
		WAVEFORMATEX	Format;
		std::vector<unsigned char> Data;
};

static void SDLCALL Mix_Callback(void * , Uint8 * stream, int len)
{
	static std::vector<float> & mix = *new std::vector<float>;		// Never destroyed (see Buffers).
	int frames = len / (int)(2 * sizeof(Sint16));
	mix.assign(frames * 2, 0.0f);

	for (size_t i = 0; i < Buffers.size(); i++) {
		Buffers[i]->Mix(&mix[0], frames);
	}

	Sint16 * out = (Sint16 *)stream;
	for (int i = 0; i < frames * 2; i++) {
		float v = mix[i] * 32767.0f;
		if (v > 32767.0f) v = 32767.0f;
		if (v < -32768.0f) v = -32768.0f;
		out[i] = (Sint16)v;
	}
}

ULONG SDLSoundBuffer::Release(void)
{
	SDL_LockAudioDevice(Device);
	std::vector<SDLSoundBuffer *>::iterator it = std::find(Buffers.begin(), Buffers.end(), this);
	if (it != Buffers.end()) Buffers.erase(it);
	SDL_UnlockAudioDevice(Device);
	delete this;
	return(0);
}

class SDLSound : public IDirectSound
{
	public:
		SDLSound() {}
		virtual ~SDLSound() {}

		virtual HRESULT SetCooperativeLevel(HWND , DWORD )
		{
			return(DS_OK);
		}

		virtual HRESULT CreateSoundBuffer(LPCDSBUFFERDESC desc, LPDIRECTSOUNDBUFFER * buffer, void * )
		{
			if (desc == NULL || buffer == NULL) return(DSERR_INVALIDPARAM);
			bool primary = (desc->dwFlags & DSBCAPS_PRIMARYBUFFER) != 0;
			SDLSoundBuffer * sb = new SDLSoundBuffer(primary, desc->dwBufferBytes, desc->lpwfxFormat);
			if (!primary) {
				SDL_LockAudioDevice(Device);
				Buffers.push_back(sb);
				SDL_UnlockAudioDevice(Device);
			}
			*buffer = sb;
			return(DS_OK);
		}

		virtual HRESULT GetCaps(LPDSCAPS caps)
		{
			if (caps) caps->dwFlags = 0;
			return(DS_OK);
		}

		virtual ULONG Release(void)
		{
			delete this;
			if (--DeviceUsers <= 0) {
				DeviceUsers = 0;
				/*
				**	Keep the device open: the movie player and the game share it and buffers
				**	may outlive the object that created them.
				*/
			}
			return(0);
		}
};

HRESULT DirectSoundCreate(void * , LPDIRECTSOUND * ds, void * )
{
	if (ds == NULL) return(DSERR_INVALIDPARAM);
	*ds = NULL;

	if (getenv("RA_NO_SOUND") && atoi(getenv("RA_NO_SOUND")) != 0) return(DSERR_NODRIVER);

	if (Device == 0) {
		if (!SDL_WasInit(SDL_INIT_AUDIO)) {
			if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
				SDL_Log("Unable to initialise SDL audio: %s", SDL_GetError());
				return(DSERR_NODRIVER);
			}
		}
		SDL_AudioSpec want, have;
		SDL_zero(want);
		want.freq = 44100;
		want.format = AUDIO_S16SYS;
		want.channels = 2;
#ifdef __EMSCRIPTEN__
		want.samples = 4096;		// In a browser the mixer runs only when the game lets the page run.
#else
		want.samples = 1024;
#endif
		want.callback = Mix_Callback;
		Device = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
		if (Device == 0) {
			SDL_Log("Unable to open audio device: %s", SDL_GetError());
			return(DSERR_NODRIVER);
		}
		DeviceRate = have.freq;
		SDL_PauseAudioDevice(Device, 0);
	}

	DeviceUsers++;
	*ds = new SDLSound;
	return(DS_OK);
}
