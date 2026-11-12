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

/***************************************************************************
 *                 Project Name : Westwood 32 bit Library                  *
 *                    File Name : MISCASM.CPP                              *
 *                                                                         *
 *  C++ versions of the library's assembly language helpers:               *
 *     MISC/CLIPRECT.ASM   Clip_Rect, Confine_Rect                         *
 *     MISC/FADING.ASM     Build_Fading_Table                              *
 *     MISC/REVERSE.ASM    Reverse_Long, Reverse_Short, Swap_Long          *
 *     MEM/MEM_COPY.ASM    Mem_Copy                                        *
 *     MEM/VMPAGEIN.ASM    Force_VM_Page_In                                *
 *     SHAPE/SETSHAPE.ASM  Set_Shape_Buffer                                *
 *     PALETTE/PAL.ASM     Set_Palette_Range, CurrentPalette               *
 *     FONT/SETFPAL.ASM    Set_Font_Palette_Range                          *
 *     DRAWBUFF/TXTPRNT.ASM (and CODE/2TXTPRNT.ASM) Buffer_Print           *
 *     WSA/XORDELTA.ASM    Apply_XOR_Delta(_To_Page_Or_Viewport)           *
 *     IFF/LCWUNCMP.ASM    LCW_Uncompress                                  *
 *     IFF/LCWCOMP.ASM, CODE/LCWCOMP.ASM   LCW_Compress, LCW_Comp          *
 *-------------------------------------------------------------------------*/

#include "GBUFFER.H"
#include "MISC.H"
#include <string.h>

extern "C" {

/***************************************************************************
 * Clip_Rect -- Clips a rectangle to the area 0,0 - width,height.          *
 *                                                                         *
 * OUTPUT:  -1 if entirely outside, 0 if no clipping was needed, otherwise *
 *          1 (the rectangle has been clipped in place).                   *
 *=========================================================================*/
int __cdecl Clip_Rect(int * x, int * y, int * w, int * h, int width, int height)
{
	int x0 = *x;
	int y0 = *y;
	int x1 = *x + *w;
	int y1 = *y + *h;
	int c0 = (x0 < 0 ? 8 : 0) | (x0 > width ? 4 : 0) | (y0 < 0 ? 2 : 0) | (y0 > height ? 1 : 0);
	int c1 = (x1 < 0 ? 8 : 0) | (x1 > width ? 4 : 0) | (y1 < 0 ? 2 : 0) | (y1 > height ? 1 : 0);

	if (c0 & c1) return(-1);
	if ((c0 | c1) == 0) return(0);

	if (c0 & 8) {
		*w += *x;
		*x = 0;
	}
	if (c0 & 2) {
		*h += *y;
		*y = 0;
	}
	if (c1 & 4) {
		*w = width - *x;
		if (*w <= 0) return(-1);
	}
	if (c1 & 1) {
		*h = height - *y;
		if (*h <= 0) return(-1);
	}
	return(1);
}


/***************************************************************************
 * Confine_Rect -- Moves a rectangle so that it fits inside 0,0-width,height*
 *                                                                         *
 * OUTPUT:  1 if the rectangle was moved, else 0.                          *
 *=========================================================================*/
int __cdecl Confine_Rect(int * x, int * y, int w, int h, int width, int height)
{
	int moved = 0;

	int neg = -*x;
	int over = *x + w - width - 1;
	if (!((neg & over) < 0)) {
		moved = 1;
		if (neg < 0) {
			*x -= over + 1;
		} else {
			*x = 0;
		}
	}

	neg = -*y;
	over = *y + h - height - 1;
	if (!((neg & over) < 0)) {
		moved = 1;
		if (neg < 0) {
			*y -= over + 1;
		} else {
			*y = 0;
		}
	}
	return(moved);
}


/***************************************************************************
 * Build_Fading_Table -- Builds a remap table that fades toward a colour.  *
 *                                                                         *
 *    For every colour (except 0) finds the closest palette colour to one  *
 *    "frac/256" of the way toward the target colour. Arithmetic matches   *
 *    the original 8 bit register math exactly.                            *
 *=========================================================================*/
void * __cdecl Build_Fading_Table(void const * palette, void const * dest, int color, int frac)
{
	if (palette == NULL || dest == NULL) return((void *)dest);

	unsigned char const * pal = (unsigned char const *)palette;
	unsigned char * table = (unsigned char *)dest;
	if ((unsigned)frac >= 0x100) frac = 0xFF;

	unsigned char targetred = pal[color*3+0];
	unsigned char targetgreen = pal[color*3+1];
	unsigned char targetblue = pal[color*3+2];
	int half = (frac >> 1) & 0xFF;

	table[0] = 0;
	for (int index = 1; index < 256; index++) {
		unsigned char ideal[3];
		unsigned char const target[3] = {targetred, targetgreen, targetblue};
		for (int gun = 0; gun < 3; gun++) {
			unsigned char value = pal[index*3+gun];
			signed char diff = (signed char)(unsigned char)(value - target[gun]);
			short product = (short)(diff * (signed char)half);
			product = (short)(product << 1);
			ideal[gun] = (unsigned char)(value - (unsigned char)((unsigned short)product >> 8));
		}

		unsigned int matchvalue = 0xFFFFFFFFU;
		unsigned char matchcolor = (unsigned char)color;
		for (int scan = 1; scan < 256; scan++) {
			if (scan == index) continue;
			unsigned int distance = 0;
			for (int gun = 0; gun < 3; gun++) {
				signed char d = (signed char)(unsigned char)(pal[scan*3+gun] - ideal[gun]);
				distance += (unsigned short)(short)(d * d);
			}
			if (distance == 0) {
				matchcolor = (unsigned char)scan;
				break;
			}
			if (distance <= matchvalue) {
				matchvalue = distance;
				matchcolor = (unsigned char)scan;
			}
		}
		table[index] = matchcolor;
	}
	return((void *)dest);
}


/***************************************************************************
 * Byte order helpers.                                                     *
 *=========================================================================*/
int __cdecl Reverse_Long(int number)
{
	unsigned int n = (unsigned int)number;
	return((int)((n >> 24) | ((n >> 8) & 0xFF00) | ((n << 8) & 0xFF0000) | (n << 24)));
}

short __cdecl Reverse_Short(short number)
{
	unsigned short n = (unsigned short)number;
	return((short)((n >> 8) | (n << 8)));
}

int __cdecl Swap_Long(int number)
{
	unsigned int n = (unsigned int)number;
	return((int)((n >> 16) | (n << 16)));
}


/***************************************************************************
 * Memory helpers.                                                         *
 *=========================================================================*/
void __cdecl Mem_Copy(void const * source, void * dest, unsigned int bytes)
{
	if (source == dest || source == NULL || dest == NULL) return;
	memmove(dest, source, bytes);
}

void __cdecl Force_VM_Page_In(void * , int )
{
	/*
	**	Used to touch memory so Windows 95 would page it in before time critical code.
	*/
}


/***************************************************************************
 * Set_Shape_Buffer -- Sets the work buffer used to uncompress shapes.     *
 *=========================================================================*/
void * _ShapeBuffer = NULL;
int _ShapeBufferSize = 0;

void __cdecl Set_Shape_Buffer(void const * buffer, int size)
{
	_ShapeBufferSize = size;
	_ShapeBuffer = (void *)buffer;
}


/***************************************************************************
 * Set_Palette_Range -- Sets the whole hardware palette.                   *
 *=========================================================================*/
unsigned char CurrentPalette[768] = {0};

void __cdecl Set_Palette_Range(void * palette)
{
	memcpy(CurrentPalette, palette, 768);
	Set_DD_Palette(palette);
}


/***************************************************************************
 * Fonts: colour translation and the text printer.                         *
 *                                                                         *
 *   Fonts store 4 bit pixels. ColorXlat[n] (and ColorXlat[n * 16] for the *
 *   high nibble) gives the palette index for font colour n; 0 means       *
 *   "transparent". Colour 0 is the background, 1 the foreground.          *
 *=========================================================================*/
unsigned char ColorXlat[256] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x01,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x03,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x04,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x05,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x06,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x07,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x08,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x09,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0A,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0C,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0D,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0E,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x0F,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

void __cdecl Set_Font_Palette_Range(void const * palette, int start, int endval)
{
	unsigned char const * pal = (unsigned char const *)palette;
	start &= 0x0F;
	endval &= 0x0F;
	if (endval < start) return;
	for (int index = start; index <= endval; index++) {
		unsigned char c = *pal++;
		ColorXlat[index] = c;
		ColorXlat[index << 4] = c;
	}
}

} // extern "C"

extern void const * FontPtr;
extern int FontXSpacing;
extern int FontYSpacing;

#define	FONTINFOBLOCK		4
#define	FONTOFFSETBLOCK	6
#define	FONTWIDTHBLOCK		8
#define	FONTHEIGHTBLOCK	12
#define	FONTINFOMAXHEIGHT	4

extern "C" int __cdecl Buffer_Print(void * thisptr, const char * string, int x_pixel, int y_pixel, int fcolor, int bcolor)
{
	if (string == NULL) return(0);

	GraphicViewPortClass * vp = (GraphicViewPortClass *)thisptr;
	int vpheight = vp->Get_Height();
	int vpwidth = vp->Get_Width();
	int bufferwidth = vpwidth + vp->Get_XAdd() + vp->Get_Pitch();
	unsigned char * curline = (unsigned char *)vp->Get_Offset() + y_pixel * bufferwidth;
	unsigned char * startdraw = curline + x_pixel;
	int original_x = x_pixel;

	unsigned char const * font = (unsigned char const *)FontPtr;
	if (font == NULL) return(0);

	unsigned short const * offsetblock = (unsigned short const *)(font + *(unsigned short const *)(font + FONTOFFSETBLOCK));
	unsigned char const * infoblock = font + *(unsigned short const *)(font + FONTINFOBLOCK);
	unsigned char const * widthblock = font + *(unsigned short const *)(font + FONTWIDTHBLOCK);
	unsigned char const * heightblock = font + *(unsigned short const *)(font + FONTHEIGHTBLOCK);

	int maxheight = infoblock[FONTINFOMAXHEIGHT];
	if (maxheight + y_pixel > vpheight) return(0);
	y_pixel += maxheight;

	ColorXlat[1] = (unsigned char)fcolor;
	ColorXlat[16] = (unsigned char)fcolor;
	ColorXlat[0] = (unsigned char)bcolor;

	for (;;) {
		unsigned char c = (unsigned char)*string;
		if (c == 0) break;
		string++;
		unsigned char * dst = startdraw;

		if (c == 10 || c == 13) {
			/*
			**	Line feed returns to column 0; carriage return to the original column.
			*/
line_feed:
			int advance = maxheight + FontYSpacing;
			if (y_pixel + advance > vpheight) return(0);
			curline += advance * bufferwidth;
			y_pixel += advance;
			x_pixel = (c == 10) ? 0 : original_x;
			startdraw = curline + x_pixel;
			continue;
		}

		int width = widthblock[c];
		int next_x = x_pixel + width + FontXSpacing;
		if (next_x > vpwidth) {
			/*
			**	Doesn't fit: wrap to the next line (back at the original column) and draw
			**	this character again there.
			*/
			string--;
			c = 0;
			goto line_feed;
		}
		startdraw += width + FontXSpacing;
		x_pixel = next_x;
		int nextdraw = bufferwidth - width;

		unsigned char const * data = font + offsetblock[c];
		int top = heightblock[c*2];
		int charheight = heightblock[c*2+1];
		int bottomblank = maxheight - (charheight + top);

		/*
		**	Blank rows above the character (only drawn when the background is not
		**	transparent).
		*/
		if (top) {
			if (ColorXlat[0]) {
				for (int row = 0; row < top; row++) {
					memset(dst, ColorXlat[0], width);
					dst += bufferwidth;
				}
			} else {
				dst += top * bufferwidth;
			}
		}

		/*
		**	The character data: 4 bit pixels, low nibble first, each row byte aligned.
		*/
		for (int row = 0; row < charheight; row++) {
			int remaining = width;
			while (remaining > 0) {
				unsigned char byte = *data++;
				unsigned char pixel = ColorXlat[byte & 0x0F];
				if (pixel) *dst = pixel;
				dst++;
				if (--remaining == 0) break;
				pixel = ColorXlat[byte & 0xF0];
				if (pixel) *dst = pixel;
				dst++;
				remaining--;
			}
			dst += nextdraw;
		}

		/*
		**	Blank rows below the character.
		*/
		if (bottomblank > 0 && ColorXlat[0]) {
			for (int row = 0; row < bottomblank; row++) {
				memset(dst, ColorXlat[0], width);
				dst += bufferwidth;
			}
		}
	}
	return((int)(intptr_t)startdraw);
}


extern "C" {

/***************************************************************************
 * XOR delta (Format 40) decoding, used by WSA animations and shapes.      *
 *                                                                         *
 *   cmd  > 0          : XOR the next cmd bytes                            *
 *   cmd == 0          : XOR the next "count" bytes with "value"           *
 *   cmd  < 0 (& 0x7F) : skip that many bytes                              *
 *   0x80 + word w     : w == 0 stop; w < 0x8000 skip w;                   *
 *                       0x8000|n XOR n bytes; 0xC000|n XOR-fill n bytes   *
 *=========================================================================*/
unsigned int __cdecl Apply_XOR_Delta(char * target, char * delta)
{
	unsigned char * dst = (unsigned char *)target;
	unsigned char const * src = (unsigned char const *)delta;

	for (;;) {
		unsigned int cmd = *src++;
		if (cmd == 0) {
			unsigned int count = *src++;
			unsigned char value = *src++;
			while (count--) *dst++ ^= value;
			continue;
		}
		if (!(cmd & 0x80)) {
			while (cmd--) *dst++ ^= *src++;
			continue;
		}
		cmd -= 0x80;
		if (cmd) {
			dst += cmd;
			continue;
		}
		unsigned int word = src[0] | (src[1] << 8);
		src += 2;
		if (word == 0) break;
		if (!(word & 0x8000)) {
			dst += word;
			continue;
		}
		word -= 0x8000;
		if (word & 0x4000) {
			unsigned int count = word - 0x4000;
			unsigned char value = *src++;
			while (count--) *dst++ ^= value;
		} else {
			while (word--) *dst++ ^= *src++;
		}
	}
	return(0);
}

#define	DO_XOR	0

void __cdecl Apply_XOR_Delta_To_Page_Or_Viewport(void * target, void * delta, int width, int nextrow, int copy)
{
	unsigned char * dst = (unsigned char *)target;
	unsigned char const * src = (unsigned char const *)delta;
	int column = 0;
	bool do_xor = (copy == DO_XOR);

	/*
	**	"nextrow" is the full pitch of the destination; "width" the width of the image.
	*/
	#define PUT(v) { if (do_xor) *dst ^= (v); else *dst = (v); dst++; if (++column == width) { dst -= column; column = 0; dst += nextrow; } }

	for (;;) {
		unsigned int cmd = *src++;
		if (cmd == 0) {
			unsigned int count = *src++;
			unsigned char value = *src++;
			while (count--) PUT(value);
			continue;
		}
		if (!(cmd & 0x80)) {
			while (cmd--) {
				unsigned char v = *src++;
				PUT(v);
			}
			continue;
		}
		cmd -= 0x80;
		unsigned int skip = cmd;
		if (cmd == 0) {
			unsigned int word = src[0] | (src[1] << 8);
			src += 2;
			if (word == 0) break;
			if (word & 0x8000) {
				word -= 0x8000;
				if (word & 0x4000) {
					unsigned int count = word - 0x4000;
					unsigned char value = *src++;
					while (count--) PUT(value);
				} else {
					while (word--) {
						unsigned char v = *src++;
						PUT(v);
					}
				}
				continue;
			}
			skip = word;
		}

		/*
		**	Skip, wrapping across rows.
		*/
		dst -= column;
		column += skip;
		while (column >= width) {
			column -= width;
			dst += nextrow;
		}
		dst += column;
	}
	#undef PUT
}


/***************************************************************************
 * LCW (Format 80) decompression.                                          *
 *                                                                         *
 *   0cccpppp p      copy c+3 bytes from "p" bytes back in the output      *
 *   10cccccc        copy c literal bytes (c == 0 ends the data)           *
 *   11cccccc w      copy c+3 bytes from absolute output position w        *
 *   11111110 w b    fill w bytes with b                                   *
 *   11111111 w w    copy w bytes from absolute output position w          *
 *                                                                         *
 *   Never writes more than "length" bytes.                                *
 *=========================================================================*/
unsigned int __cdecl LCW_Uncompress(void * source, void * dest, unsigned int length)
{
	unsigned char const * src = (unsigned char const *)source;
	unsigned char * start = (unsigned char *)dest;
	unsigned char * dst = start;
	unsigned char * end = start + length;

	while (dst < end) {
		unsigned int maxlen = (unsigned int)(end - dst);
		unsigned int cmd = *src++;

		if (!(cmd & 0x80)) {
			unsigned int count = (cmd >> 4) + 3;
			unsigned int offset = ((cmd & 0x0F) << 8) | *src++;
			if (count > maxlen) count = maxlen;
			unsigned char const * from = dst - offset;
			while (count--) *dst++ = *from++;
			continue;
		}

		if (!(cmd & 0x40)) {
			if (cmd == 0x80) break;
			unsigned int count = cmd & 0x3F;
			if (count > maxlen) count = maxlen;
			memcpy(dst, src, count);
			dst += count;
			src += count;
			continue;
		}

		if (cmd == 0xFE) {
			unsigned int count = src[0] | (src[1] << 8);
			unsigned char value = src[2];
			src += 3;
			if (count > maxlen) count = maxlen;
			memset(dst, value, count);
			dst += count;
			continue;
		}

		unsigned int count;
		if (cmd == 0xFF) {
			count = src[0] | (src[1] << 8);
			src += 2;
		} else {
			count = (cmd & 0x3F) + 3;
		}
		unsigned int offset = src[0] | (src[1] << 8);
		src += 2;
		if (count > maxlen) count = maxlen;
		unsigned char const * from = start + offset;
		while (count--) *dst++ = *from++;
	}
	return((unsigned int)(dst - start));
}


/***************************************************************************
 * LCW compression.                                                        *
 *                                                                         *
 *   Produces data decodable by LCW_Uncompress / LCW_Uncomp. Matches are   *
 *   found with a hash chain over 3 byte prefixes and encoded with the     *
 *   absolute copy commands; runs use the fill command.                    *
 *                                                                         *
 * OUTPUT:  Size of the compressed data.                                   *
 *=========================================================================*/
#define	LCW_HASH_BITS	14
#define	LCW_MAX_CHAIN	64

static unsigned char * LCW_Flush_Literals(unsigned char * dst, unsigned char const * lit, unsigned int count)
{
	while (count) {
		unsigned int n = count > 63 ? 63 : count;
		*dst++ = (unsigned char)(0x80 | n);
		memcpy(dst, lit, n);
		dst += n;
		lit += n;
		count -= n;
	}
	return(dst);
}

unsigned int __cdecl LCW_Compress(void * source, void * dest, unsigned int length)
{
	unsigned char const * src = (unsigned char const *)source;
	unsigned char * dst = (unsigned char *)dest;
	if (length == 0) {
		*dst++ = 0x80;
		return(1);
	}

	/*
	**	Absolute offsets are 16 bits, so only the first 64K of the source can be referenced.
	*/
	static int head[1 << LCW_HASH_BITS];
	static int * prev = NULL;
	static unsigned int prev_size = 0;
	if (prev_size < length) {
		delete [] prev;
		prev = new int[length];
		prev_size = length;
	}
	for (int i = 0; i < (1 << LCW_HASH_BITS); i++) head[i] = -1;

	unsigned int pos = 0;
	unsigned int literal_start = 0;

	#define LCW_HASH(p) ((((unsigned)src[p] << 10) ^ ((unsigned)src[(p)+1] << 5) ^ src[(p)+2]) & ((1 << LCW_HASH_BITS) - 1))

	while (pos < length) {
		unsigned int best_len = 0;
		unsigned int best_pos = 0;

		/*
		**	Run of identical bytes?
		*/
		unsigned int run = 1;
		while (pos + run < length && run < 0xFFFF && src[pos + run] == src[pos]) run++;

		/*
		**	Longest earlier match.
		*/
		if (pos + 2 < length) {
			int candidate = head[LCW_HASH(pos)];
			int chain = 0;
			while (candidate >= 0 && chain < LCW_MAX_CHAIN) {
				if (candidate < 0x10000) {
					unsigned int len = 0;
					unsigned int limit = length - pos;
					if (limit > 0xFFFF) limit = 0xFFFF;
					while (len < limit && src[candidate + len] == src[pos + len]) len++;
					if (len > best_len) {
						best_len = len;
						best_pos = (unsigned int)candidate;
						if (len == limit) break;
					}
				}
				candidate = prev[candidate];
				chain++;
			}
		}

		unsigned int consumed = 0;
		if (run >= 4 && run >= best_len) {
			dst = LCW_Flush_Literals(dst, src + literal_start, pos - literal_start);
			*dst++ = 0xFE;
			*dst++ = (unsigned char)(run & 0xFF);
			*dst++ = (unsigned char)(run >> 8);
			*dst++ = src[pos];
			consumed = run;
		} else if (best_len >= 3) {
			dst = LCW_Flush_Literals(dst, src + literal_start, pos - literal_start);
			if (best_len <= 64) {
				*dst++ = (unsigned char)(0xC0 | (best_len - 3));
			} else {
				*dst++ = 0xFF;
				*dst++ = (unsigned char)(best_len & 0xFF);
				*dst++ = (unsigned char)(best_len >> 8);
			}
			*dst++ = (unsigned char)(best_pos & 0xFF);
			*dst++ = (unsigned char)(best_pos >> 8);
			consumed = best_len;
		}

		if (consumed) {
			for (unsigned int i = 0; i < consumed; i++) {
				if (pos + 2 < length) {
					unsigned int h = LCW_HASH(pos);
					prev[pos] = head[h];
					head[h] = (int)pos;
				}
				pos++;
			}
			literal_start = pos;
		} else {
			if (pos + 2 < length) {
				unsigned int h = LCW_HASH(pos);
				prev[pos] = head[h];
				head[h] = (int)pos;
			}
			pos++;
		}
	}
	#undef LCW_HASH

	dst = LCW_Flush_Literals(dst, src + literal_start, pos - literal_start);
	*dst++ = 0x80;
	return((unsigned int)(dst - (unsigned char *)dest));
}

int __cdecl LCW_Comp(void const * source, void * dest, int length)
{
	return((int)LCW_Compress((void *)source, dest, (unsigned int)length));
}

} // extern "C"
