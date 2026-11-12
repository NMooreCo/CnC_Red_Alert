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

/***********************************************************************************************
 *                 Project Name : Westwood 32 bit Library                                      *
 *                    File Name : AUDCODEC.CPP                                                 *
 *                                                                                             *
 *   C++ replacement for SOSCODEC.ASM, OLSOSDEC.ASM and AUDUNCMP.ASM: the audio decompressors *
 *   used by the sound system (see AUDCODEC.H for the actual decoders).                       *
 *---------------------------------------------------------------------------------------------*/

#include	<string.h>
#include	"SOSCOMP.H"
#include	"AUDCODEC.H"

extern "C" {

void __cdecl sosCODECInitStream(_SOS_COMPRESS_INFO * info)
{
	SOS_Init_Stream(info);
}


/*
**	The assembly had a fast path for 16 bit mono data and a general one for everything else.
**	Both produce the same samples.
*/
unsigned int __cdecl sosCODECDecompressData(_SOS_COMPRESS_INFO * info, unsigned int bytes)
{
	return(SOS_Decompress(info, bytes));
}


unsigned int __cdecl General_sosCODECDecompressData(_SOS_COMPRESS_INFO * info, unsigned int bytes)
{
	return(SOS_Decompress(info, bytes));
}


int __cdecl Decompress_Frame(void * source, void * dest, int size)
{
	return(WW_Audio_Unzap(source, dest, size));
}


/*
**	Memory locking was only needed for the DOS interrupt driven sound drivers.
*/
int __cdecl sosCODEC_Lock(void) {return(1);}
int __cdecl sosCODEC_Unlock(void) {return(1);}
int __cdecl Decompress_Frame_Lock(void) {return(1);}
int __cdecl Decompress_Frame_Unlock(void) {return(1);}

}
