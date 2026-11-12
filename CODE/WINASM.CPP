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
 *                 Project Name : Command & Conquer                                            *
 *                    File Name : WINASM.CPP                                                   *
 *                                                                                             *
 *   C++ replacement for WINASM.ASM (320x200 -> 640x400 scaling of movies and cut scenes,     *
 *   see Interpolate_2X_Scale) and CPUID.ASM.                                                 *
 *                                                                                             *
 *   Every source line becomes a line of twice the width: each pixel is followed by the       *
 *   colour half way between it and the next pixel (from PaletteInterpolationTable). The last *
 *   pair of pixels is followed by a black pixel. The three variants differ in what goes     *
 *   into the odd output lines:                                                               *
 *                                                                                             *
 *     Asm_Interpolate                  -- nothing (interlaced look).                          *
 *     Asm_Interpolate_Line_Double      -- a copy of the line above.                           *
 *     Asm_Interpolate_Line_Interpolate -- a blend of the lines above and below.               *
 *                                                                                             *
 *   "dest_width" is twice the destination pitch, as Interpolate_2X_Scale passes it.          *
 *---------------------------------------------------------------------------------------------*/

#include	"FUNCTION.H"

extern "C" unsigned char PaletteInterpolationTable[256][256];


/*
**	Horizontally doubles one line of "width" pixels into "dest" (2 * width pixels).
*/
static void Interpolate_Single_Line(unsigned char const * source, unsigned char * dest, int width)
{
	int x;
	for (x = 0; x + 2 < width; x++) {
		*dest++ = source[x];
		*dest++ = PaletteInterpolationTable[source[x + 1]][source[x]];
	}

	/*
	**	The last two pixels.
	*/
	if (width >= 2) {
		*dest++ = source[x];
		*dest++ = PaletteInterpolationTable[source[x + 1]][source[x]];
		*dest++ = source[x + 1];
		*dest++ = 0;
	}
}


/*
**	Blends two (already doubled) lines of 2 * width pixels.
*/
static void Interpolate_Between_Lines(unsigned char const * source1, unsigned char const * source2, unsigned char * dest, int width)
{
	for (int x = 0; x < width * 2; x++) {
		dest[x] = PaletteInterpolationTable[source2[x]][source1[x]];
	}
}


extern "C" {

void __cdecl Asm_Interpolate(unsigned char * src_ptr, unsigned char * dest_ptr, int lines, int src_width, int dest_width)
{
	for (int y = 0; y < lines; y++) {
		Interpolate_Single_Line(src_ptr, dest_ptr, src_width);
		src_ptr += src_width;
		dest_ptr += dest_width;
	}
}


void __cdecl Asm_Interpolate_Line_Double(unsigned char * src_ptr, unsigned char * dest_ptr, int lines, int src_width, int dest_width)
{
	int pitch = dest_width / 2;
	for (int y = 0; y < lines; y++) {
		Interpolate_Single_Line(src_ptr, dest_ptr, src_width);
		memcpy(dest_ptr + pitch, dest_ptr, src_width * 2);
		src_ptr += src_width;
		dest_ptr += dest_width;
	}
}


void __cdecl Asm_Interpolate_Line_Interpolate(unsigned char * src_ptr, unsigned char * dest_ptr, int lines, int src_width, int dest_width)
{
	static unsigned char TopLine[640 * 4];
	static unsigned char BottomLine[640 * 4];
	static unsigned char LineBuffer[640 * 4];

	if (lines <= 0 || src_width * 2 > (int)sizeof(LineBuffer)) return;

	int pitch = dest_width / 2;
	unsigned char * last = TopLine;
	unsigned char * next = BottomLine;

	Interpolate_Single_Line(src_ptr, last, src_width);
	src_ptr += src_width;

	for (int y = 1; y < lines; y++) {
		Interpolate_Single_Line(src_ptr, next, src_width);
		Interpolate_Between_Lines(last, next, LineBuffer, src_width);
		memcpy(dest_ptr, last, src_width * 2);
		dest_ptr += pitch;
		memcpy(dest_ptr, LineBuffer, src_width * 2);
		dest_ptr += pitch;
		src_ptr += src_width;

		unsigned char * temp = last;
		last = next;
		next = temp;
	}

	/*
	**	The final line. (The assembly version interpolated the line after the end of the
	**	source buffer here and drew that.)
	*/
	memcpy(dest_ptr, last, src_width * 2);
}


/*
**	Win95 low resolution (320x200 ModeX) display. The port always runs the 640x400/480 display,
**	where GScreenClass::Blit_Display uses a regular blit instead.
*/
void ModeX_Blit(GraphicBufferClass * source)
{
	if (source != NULL) {
		source->Blit(SeenBuff);
	}
}


/*
**	CPUID.ASM: processor identification, only reported in the (defunct) game statistics.
*/
char CPUType = 6;
char VendorID[13] = "PortableCPU";

int __cdecl Detect_MMX_Availability(void)		// (GETCPU.CPP declares it with "bool" defined as int.)
{
	return(0);
}

}
