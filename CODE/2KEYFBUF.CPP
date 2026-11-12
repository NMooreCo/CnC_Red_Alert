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
 *                    File Name : 2KEYFBUF.CPP                                                 *
 *                                                                                             *
 *   C++ version of Buffer_Frame_To_Page (2KEYFBUF.ASM): draws an uncompressed shape frame    *
 *   (as produced by Build_Frame) into a graphic view port, with the shape drawing effects:   *
 *                                                                                             *
 *      SHAPE_CENTER    -- x,y is the centre of the shape.                                     *
 *      SHAPE_TRANS     -- colour 0 is transparent.                                            *
 *      SHAPE_GHOST     -- translucency: an "is translucent" table (256 bytes, 0xFF = opaque)  *
 *                         followed by translucency tables [n][background] (args: table).       *
 *      SHAPE_FADING    -- remap through a fading table N times (args: table, N).               *
 *      SHAPE_PREDATOR  -- "cloaked" shimmer: pixels are replaced by background pixels at a    *
 *                         small offset (args: offset).                                         *
 *      SHAPE_PARTIAL   -- only some pixels shimmer (args: fraction out of 256).                *
 *                                                                                             *
 *   The extra arguments follow "flags" in the order listed above, one group per flag set.    *
 *   Pixel results match the original assembly, including its quirks (noted below). The      *
 *   assembly also had a fast path for frames cached by the "big shape buffer"; the port does  *
 *   not use that cache (see Check_Use_Compressed_Shapes) and always takes the normal path.    *
 *---------------------------------------------------------------------------------------------*/

#include	"FUNCTION.H"
#include	<stdarg.h>

#define	SHAPE_CENTER_FLAG		0x0020
#define	SHAPE_TRANS_FLAG		0x0040
#define	SHAPE_FADING_FLAG		0x0100
#define	SHAPE_PREDATOR_FLAG	0x0200
#define	SHAPE_GHOST_FLAG		0x1000
#define	SHAPE_PARTIAL_FLAG	0x4000

/*
**	Predator effect state. It persists between calls, as in the original.
*/
static const unsigned short BFPredTable[8] = {1, 3, 2, 5, 2, 3, 4, 1};
static const short BFPredNegTable[8] = {-1, -3, -2, -5, -2, -4, -3, -1};
static unsigned short BFPredNegAdjusted[8];		// (pitch + BFPredNegTable[n]) & 0xFFFF
static bool BFPredNegative = false;				// Use the "negative" (next row) offsets.
static unsigned int BFPredIndex = 0;				// Byte offset into the table (0..14, even).
static unsigned int BFPartialPred = 0x100;
static unsigned int BFPartialCount = 0;

static inline int Predator_Offset(void)
{
	unsigned int index = (BFPredIndex & 0x0E) >> 1;
	BFPredIndex = (BFPredIndex + 2) & 0x0E;
	return(BFPredNegative ? BFPredNegAdjusted[index] : BFPredTable[index]);
}

extern "C" int __cdecl Buffer_Frame_To_Page(int x_pixel, int y_pixel, int pixel_width, int pixel_height, void * src, GraphicViewPortClass & dest, int flags, ...)
{
	if (src == NULL) return(0);

	unsigned char const * is_translucent = NULL;
	unsigned char const * translucent = NULL;
	unsigned char const * fading_table = NULL;
	int fading_num = 0;

	va_list args;
	va_start(args, flags);

	if (flags & SHAPE_CENTER_FLAG) {
		x_pixel -= pixel_width >> 1;
		y_pixel -= pixel_height >> 1;
	}

	bool trans = (flags & SHAPE_TRANS_FLAG) != 0;
	bool ghost = (flags & SHAPE_GHOST_FLAG) != 0;
	if (ghost) {
		is_translucent = (unsigned char const *)va_arg(args, void *);
		translucent = is_translucent + 256;
		if (is_translucent == NULL) ghost = false;
	}

	bool fading = (flags & SHAPE_FADING_FLAG) != 0;
	if (fading) {
		fading_table = (unsigned char const *)va_arg(args, void *);
		fading_num = va_arg(args, int) & 0x3F;
		/*
		**	(The original would loop "forever" with a count of zero; one pass is the sane
		**	equivalent.)
		*/
		if (fading_num == 0) fading_num = 1;
		if (fading_table == NULL) fading = false;
	}

	int stride = dest.Get_Width() + dest.Get_XAdd() + dest.Get_Pitch();

	bool predator = (flags & SHAPE_PREDATOR_FLAG) != 0;
	if (predator) {
		int value = va_arg(args, int) << 1;
		if (value < 0) {
			BFPredNegative = true;
			BFPredIndex = (unsigned int)(-value) & 0x0E;
		} else {
			BFPredNegative = false;
			BFPredIndex = (unsigned int)value & 0x0E;
		}
		BFPartialCount = 0;
		BFPartialPred = 0x100;
		for (int i = 0; i < 8; i++) {
			BFPredNegAdjusted[i] = (unsigned short)(BFPredNegTable[i] + stride);
		}
	}
	if (flags & SHAPE_PARTIAL_FLAG) {
		BFPartialPred = va_arg(args, int) & 0xFF;
	}
	va_end(args);

	/*
	**	Clip to the destination view port.
	*/
	int vpw = dest.Get_Width();
	int vph = dest.Get_Height();
	int x1_pixel = x_pixel + pixel_width;
	int y1_pixel = y_pixel + pixel_height;
	int scr_x = 0;
	int scr_y = 0;

	int c0 = (x_pixel < 0 ? 8 : 0) | (x_pixel > vpw ? 4 : 0) | (y_pixel < 0 ? 2 : 0) | (y_pixel > vph ? 1 : 0);
	int c1 = (x1_pixel < 0 ? 8 : 0) | (x1_pixel > vpw ? 4 : 0) | (y1_pixel < 0 ? 2 : 0) | (y1_pixel > vph ? 1 : 0);
	if (c0 & c1) return(0);
	if (c0 & 8) {
		scr_x = -x_pixel;
		x_pixel = 0;
	}
	if (c0 & 2) {
		scr_y = -y_pixel;
		y_pixel = 0;
	}
	if (c1 & 4) x1_pixel = vpw;
	if (c1 & 1) y1_pixel = vph;

	int rows = y1_pixel - y_pixel;
	int cols = x1_pixel - x_pixel;
	if (rows <= 0 || cols <= 0) return(0);

	unsigned char * dst = (unsigned char *)dest.Get_Offset() + y_pixel * stride + x_pixel;
	unsigned char const * source = (unsigned char const *)src + scr_y * pixel_width + scr_x;

	/*
	**	Predator pixels read the background at an offset that can reach past the bottom of
	**	the view port; keep such reads inside the buffer.
	*/
	GraphicBufferClass * gbuffer = dest.Get_Graphic_Buffer();
	unsigned char const * buffer_end = NULL;
	if (gbuffer != NULL) {
		buffer_end = (unsigned char const *)gbuffer->Get_Offset() + (gbuffer->Get_Width() + gbuffer->Get_Pitch()) * gbuffer->Get_Height();
	}

	/*
	**	Simple (and by far the most common) cases.
	*/
	if (!ghost && !fading && !predator) {
		for (int y = 0; y < rows; y++) {
			if (trans) {
				for (int x = 0; x < cols; x++) {
					if (source[x]) dst[x] = source[x];
				}
			} else {
				memcpy(dst, source, cols);
			}
			source += pixel_width;
			dst += stride;
		}
		return(0);
	}

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			unsigned char c = source[x];
			if (trans && c == 0) continue;

			if (predator) {
				unsigned int count = BFPartialCount + BFPartialPred;
				if (count & 0xFF00) {
					BFPartialCount = count & 0xFF;
					int offset = Predator_Offset();
					unsigned char const * p = &dst[x] + offset;
					if (buffer_end == NULL || p < buffer_end) c = *p;
				} else {
					BFPartialCount = count;
					/*
					**	Quirk of the original: the plain predator effect leaves these pixels
					**	alone, all the other combinations draw the shape pixel.
					*/
					if (!trans && !ghost && !fading) continue;
				}
			}

			if (ghost) {
				unsigned char table = is_translucent[c];
				if (table != 0xFF) {
					c = translucent[(table << 8) + dst[x]];
				}
			}

			if (fading) {
				for (int n = 0; n < fading_num; n++) {
					c = fading_table[c];
				}
			}

			dst[x] = c;
		}
		source += pixel_width;
		dst += stride;
	}
	return(0);
}
