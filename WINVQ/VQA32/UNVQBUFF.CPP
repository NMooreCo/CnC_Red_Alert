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

/****************************************************************************
*
*  FILE
*     unvqbuff.cpp
*
*  DESCRIPTION
*     C++ replacement for UNVQBUFF.ASM (only the 4x2 block size is used by
*     the game, see VQAPLAY.H), plus the VGA specific helpers of VB.ASM and
*     MCGABUF.ASM. The game plays movies in buffered mode with its own draw
*     callback, so the VGA routines are never reached; they are kept as
*     harmless equivalents.
*
****************************************************************************/

#include <string.h>
#include "VQAPLAYP.H"
#include <VQM32/VIDEO.H>

extern "C" {

/****************************************************************************
*
* NAME
*     UnVQ_4x2 - Draw 4x2 block VQ frame to a buffer.
*
* FUNCTION
*     The pointer data holds the low bytes of all the block pointers first,
*     followed by all the high bytes. A high byte of 0x0F marks a block of
*     a single colour (the low byte). Otherwise the pointer is the index of
*     an 8 byte codeword: the 4 pixels of the first row followed by the 4
*     pixels of the second row.
*
* INPUTS
*     Codebook - Pointer to codebook used to draw image.
*     Pointers - Pointer to vector pointer data.
*     Buffer   - Pointer to buffer to draw image into.
*     BPR      - Number of blocks per row.
*     Rows     - Number of rows.
*     BufWidth - Width of destination buffer in pixels.
*
****************************************************************************/
void __cdecl UnVQ_4x2(unsigned char *codebook, unsigned char *pointers,
		unsigned char *buffer, unsigned int blocksperrow,
		unsigned int numrows, unsigned int bufwidth)
{
	unsigned int entries = numrows * blocksperrow;
	unsigned char const * lo = pointers;
	unsigned char const * hi = pointers + entries;

	for (unsigned int row = 0; row < numrows; row++) {
		unsigned char * dst = buffer + row * bufwidth * 2;

		for (unsigned int block = 0; block < blocksperrow; block++) {
			unsigned int low = *lo++;
			unsigned int high = *hi++;

			if (high == 0x0F) {
				memset(dst, (int)low, 4);
				memset(dst + bufwidth, (int)low, 4);
			} else {
				unsigned char const * codeword = codebook + (((high << 8) | low) << 3);
				memcpy(dst, codeword, 4);
				memcpy(dst + bufwidth, codeword + 4, 4);
			}
			dst += 4;
		}
	}
}


/*
**	Vertical blank synchronisation: the presentation layer takes care of that now.
*/
void __cdecl WaitNoVB(short)
{
}


void __cdecl WaitVB(short)
{
}


/*
**	MCGA (mode 13h) output. The "screen" is just a 320x200 buffer.
*/
void __cdecl MCGA_BufferCopy(unsigned char *, unsigned char *)
{
}


void __cdecl MCGA_Blit(unsigned char *buffer, unsigned char *screen, int imgwidth, int imgheight)
{
	if (buffer == NULL || screen == NULL) return;
	for (int y = 0; y < imgheight; y++) {
		memcpy(screen + y * 320, buffer + y * imgwidth, imgwidth);
	}
}

}


int TestVBIBit(void)
{
	return(0);
}
