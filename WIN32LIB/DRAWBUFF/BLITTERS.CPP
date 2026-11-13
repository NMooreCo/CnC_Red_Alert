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
 *                    File Name : BLITTERS.CPP                             *
 *                                                                         *
 *  C++ versions of the graphic viewport primitives that were written in  *
 *  assembly for the original (CLEAR.ASM, FILLRECT.ASM, PUTPIX.ASM,       *
 *  GETPIX.ASM, DRAWLINE.ASM, REMAP.ASM, TOBUFF.ASM, TOPAGE.ASM,          *
 *  BITBLIT.ASM, SCALE.ASM and STAMP.ASM). Clipping and edge cases follow *
 *  the assembly exactly.                                                  *
 *-------------------------------------------------------------------------*/

#include "GBUFFER.H"
#include "TILE.H"
#include <string.h>

/*
**	Accessors for the view port fields the assembly read directly.
*/
static inline unsigned char * VP_Offset(void const * vp) {return((unsigned char *)((GraphicViewPortClass *)vp)->Get_Offset());}
static inline int VP_Width(void const * vp) {return(((GraphicViewPortClass *)vp)->Get_Width());}
static inline int VP_Height(void const * vp) {return(((GraphicViewPortClass *)vp)->Get_Height());}
static inline int VP_Stride(void const * vp)
{
	GraphicViewPortClass * gvp = (GraphicViewPortClass *)vp;
	return(gvp->Get_Width() + gvp->Get_XAdd() + gvp->Get_Pitch());
}

/*
**	The assembly computed 4 bit "outcodes" for the two corners of a region:
**		bit 3 = left of 0, bit 2 = right of width, bit 1 = above 0, bit 0 = below height.
**	Note that a coordinate equal to width/height counts as inside (exclusive end points).
*/
static inline int Out_Code(int x, int y, int width, int height)
{
	return((x < 0 ? 8 : 0) | (x > width ? 4 : 0) | (y < 0 ? 2 : 0) | (y > height ? 1 : 0));
}


extern "C" {

/***************************************************************************
 * Buffer_Clear -- Fills the whole view port with a color.                 *
 *=========================================================================*/
void __cdecl Buffer_Clear(void * thisptr, unsigned char color)
{
	unsigned char * dst = VP_Offset(thisptr);
	int width = VP_Width(thisptr);
	int height = VP_Height(thisptr);
	int stride = VP_Stride(thisptr);
	for (int y = 0; y < height; y++) {
		memset(dst, color, width);
		dst += stride;
	}
}


/***************************************************************************
 * Buffer_Fill_Rect -- Fills an (inclusive) rectangle, clipped to the view *
 *=========================================================================*/
VOID __cdecl Buffer_Fill_Rect(void * thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
	int vpwidth = VP_Width(thisptr);
	int vpheight = VP_Height(thisptr);

	/*
	**	The coordinates were passed as 16 bit values.
	*/
	int x = (short)sx;
	int y = (short)sy;
	int x2 = (short)dx;
	int y2 = (short)dy;

	if (x >= x2) {int t = x; x = x2; x2 = t;}
	int w = x2 - x;
	if (y >= y2) {int t = y; y = y2; y2 = t;}
	int h = y2 - y;
	w++;
	h++;

	if (x >= vpwidth) return;
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y >= vpheight) return;
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (vpwidth - x - w < 0) w = vpwidth - x;
	if (vpheight - y - h < 0) h = vpheight - y;
	if (w <= 0 || h <= 0) return;
	if (w > vpwidth || h > vpheight) return;

	int stride = VP_Stride(thisptr);
	unsigned char * dst = VP_Offset(thisptr) + y * stride + x;
	while (h--) {
		memset(dst, color, w);
		dst += stride;
	}
}


/***************************************************************************
 * Buffer_Put_Pixel / Buffer_Get_Pixel                                     *
 *=========================================================================*/
void __cdecl Buffer_Put_Pixel(void * thisptr, int x, int y, unsigned char color)
{
	if ((unsigned)x >= (unsigned)VP_Width(thisptr)) return;
	if ((unsigned)y >= (unsigned)VP_Height(thisptr)) return;
	VP_Offset(thisptr)[y * VP_Stride(thisptr) + x] = color;
}

int __cdecl Buffer_Get_Pixel(void * thisptr, int x, int y)
{
	if ((unsigned)x >= (unsigned)VP_Width(thisptr)) return(0);
	if ((unsigned)y >= (unsigned)VP_Height(thisptr)) return(0);
	return(VP_Offset(thisptr)[y * VP_Stride(thisptr) + x]);
}


/***************************************************************************
 * Buffer_Draw_Line -- Draws a clipped line (inclusive end points).        *
 *=========================================================================*/
VOID __cdecl Buffer_Draw_Line(void * thisptr, int sx, int sy, int dx, int dy, unsigned char color)
{
	int xmax = VP_Width(thisptr) - 1;
	int ymax = VP_Height(thisptr) - 1;
	if (xmax < 0 || ymax < 0) return;

	/*
	**	Cohen-Sutherland clip to the view port.
	*/
	for (;;) {
		int c0 = (sx < 0 ? 1 : 0) | (sx > xmax ? 2 : 0) | (sy < 0 ? 4 : 0) | (sy > ymax ? 8 : 0);
		int c1 = (dx < 0 ? 1 : 0) | (dx > xmax ? 2 : 0) | (dy < 0 ? 4 : 0) | (dy > ymax ? 8 : 0);
		if ((c0 | c1) == 0) break;
		if (c0 & c1) return;
		int code = c0 ? c0 : c1;
		long long x, y;
		if (code & 8) {
			x = sx + (long long)(dx - sx) * (ymax - sy) / (dy - sy);
			y = ymax;
		} else if (code & 4) {
			x = sx + (long long)(dx - sx) * (0 - sy) / (dy - sy);
			y = 0;
		} else if (code & 2) {
			y = sy + (long long)(dy - sy) * (xmax - sx) / (dx - sx);
			x = xmax;
		} else {
			y = sy + (long long)(dy - sy) * (0 - sx) / (dx - sx);
			x = 0;
		}
		if (code == c0) {
			sx = (int)x;
			sy = (int)y;
		} else {
			dx = (int)x;
			dy = (int)y;
		}
	}

	unsigned char * base = VP_Offset(thisptr);
	int stride = VP_Stride(thisptr);
	int ddx = dx - sx;
	int ddy = dy - sy;
	int stepx = (ddx < 0) ? -1 : 1;
	int stepy = (ddy < 0) ? -1 : 1;
	ddx = (ddx < 0) ? -ddx : ddx;
	ddy = (ddy < 0) ? -ddy : ddy;

	int x = sx;
	int y = sy;
	if (ddx >= ddy) {
		int err = ddx / 2;
		for (int i = 0; i <= ddx; i++) {
			base[y * stride + x] = color;
			err -= ddy;
			if (err < 0) {
				y += stepy;
				err += ddx;
			}
			x += stepx;
		}
	} else {
		int err = ddy / 2;
		for (int i = 0; i <= ddy; i++) {
			base[y * stride + x] = color;
			err -= ddx;
			if (err < 0) {
				x += stepx;
				err += ddy;
			}
			y += stepy;
		}
	}
}


/***************************************************************************
 * Buffer_Remap -- Translates every pixel of a region through a table.     *
 *=========================================================================*/
VOID __cdecl Buffer_Remap(void * thisptr, int x0, int y0, int width, int height, void * remap)
{
	if (remap == NULL) return;
	unsigned char const * table = (unsigned char const *)remap;
	int vpw = VP_Width(thisptr);
	int vph = VP_Height(thisptr);
	int x1 = x0 + width;
	int y1 = y0 + height;

	int c0 = Out_Code(x0, y0, vpw, vph);
	int c1 = Out_Code(x1, y1, vpw, vph);
	if (c0 & c1) return;
	if (c0 & 8) x0 = 0;
	if (c0 & 2) y0 = 0;
	if (c1 & 4) x1 = vpw;
	if (c1 & 1) y1 = vph;

	int w = x1 - x0;
	int h = y1 - y0;
	if (w <= 0 || h <= 0) return;

	int stride = VP_Stride(thisptr);
	unsigned char * dst = VP_Offset(thisptr) + y0 * stride + x0;
	while (h--) {
		for (int i = 0; i < w; i++) {
			dst[i] = table[dst[i]];
		}
		dst += stride;
	}
}


/***************************************************************************
 * Buffer_To_Buffer -- Copies a view port region into a linear buffer.     *
 *                                                                         *
 *    The buffer is pixel_width pixels wide. Clipped parts of the region  *
 *    are skipped in the buffer (it keeps the requested layout).          *
 *=========================================================================*/
int __cdecl Buffer_To_Buffer(void * thisptr, int x_pixel, int y_pixel, int pixel_width, int pixel_height, void * dest, int buffer_size)
{
	int vpw = VP_Width(thisptr);
	int vph = VP_Height(thisptr);
	int dest_x1 = 0;
	int dest_y1 = 0;
	int x1_pixel = x_pixel + pixel_width;
	int y1_pixel = y_pixel + pixel_height;

	int c0 = Out_Code(x_pixel, y_pixel, vpw, vph);
	int c1 = Out_Code(x1_pixel, y1_pixel, vpw, vph);
	if (c0 & c1) return(0);
	if (c0 & 8) {
		dest_x1 = -x_pixel;
		x_pixel = 0;
	}
	if (c0 & 2) {
		dest_y1 = -y_pixel;
		y_pixel = 0;
	}
	if (c1 & 4) x1_pixel = vpw;
	if (c1 & 1) y1_pixel = vph;

	int h = y1_pixel - y_pixel;
	if (h <= 0) return(0);
	int w = x1_pixel - x_pixel;
	if (w <= 0) return(0);
	if (pixel_width * h > buffer_size) return(0);

	int stride = VP_Stride(thisptr);
	unsigned char const * src = VP_Offset(thisptr) + y_pixel * stride + x_pixel;
	unsigned char * dst = (unsigned char *)dest + dest_y1 * pixel_width + dest_x1;
	while (h--) {
		memmove(dst, src, w);
		src += stride;
		dst += pixel_width;
	}
	return(pixel_width * pixel_height);
}


/***************************************************************************
 * Buffer_To_Page -- Copies a linear buffer into a view port region.       *
 *=========================================================================*/
int __cdecl Buffer_To_Page(int x_pixel, int y_pixel, int pixel_width, int pixel_height, void * src, void * dest)
{
	if (src == NULL) return(0);

	int vpw = VP_Width(dest);
	int vph = VP_Height(dest);
	int scr_x = 0;
	int scr_y = 0;
	int x1_pixel = x_pixel + pixel_width;
	int y1_pixel = y_pixel + pixel_height;

	int c0 = Out_Code(x_pixel, y_pixel, vpw, vph);
	int c1 = Out_Code(x1_pixel, y1_pixel, vpw, vph);
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

	int h = y1_pixel - y_pixel;
	if (h <= 0) return(0);
	int w = x1_pixel - x_pixel;
	if (w <= 0) return(0);

	int stride = VP_Stride(dest);
	unsigned char * dst = VP_Offset(dest) + y_pixel * stride + x_pixel;
	unsigned char const * source = (unsigned char const *)src + scr_y * pixel_width + scr_x;
	while (h--) {
		memmove(dst, source, w);
		source += pixel_width;
		dst += stride;
	}
	return(0);
}


/***************************************************************************
 * Linear_Blit_To_Linear -- Copies a region between two view ports.        *
 *                                                                         *
 *    "trans" skips colour 0. Overlapping copies within one buffer work   *
 *    in either direction.                                                 *
 *=========================================================================*/
BOOL __cdecl Linear_Blit_To_Linear(void * thisptr, void * dest, int x_pixel, int y_pixel, int dest_x0, int dest_y0, int pixel_width, int pixel_height, BOOL trans)
{
	int x1_pixel = x_pixel + pixel_width;
	int y1_pixel = y_pixel + pixel_height;

	/*
	**	Clip against the source. (As in the original, clipping the source's top/left edge
	**	does not shift the destination.)
	*/
	int sw = VP_Width(thisptr);
	int sh = VP_Height(thisptr);
	int c0 = Out_Code(x_pixel, y_pixel, sw, sh);
	int c1 = Out_Code(x1_pixel, y1_pixel, sw, sh);
	if (c0 & c1) return(FALSE);
	if (c0 & 8) x_pixel = 0;
	if (c0 & 2) y_pixel = 0;
	if (c1 & 4) x1_pixel = sw;
	if (c1 & 1) y1_pixel = sh;

	/*
	**	Clip against the destination, adjusting the source to match.
	*/
	int dest_x1 = dest_x0 - x_pixel + x1_pixel;
	int dest_y1 = dest_y0 - y_pixel + y1_pixel;
	int dw = VP_Width(dest);
	int dh = VP_Height(dest);
	c0 = Out_Code(dest_x0, dest_y0, dw, dh);
	c1 = Out_Code(dest_x1, dest_y1, dw, dh);
	if (c0 & c1) return(FALSE);
	if (c0 & 8) {
		x_pixel -= dest_x0;
		dest_x0 = 0;
	}
	if (c0 & 2) {
		y_pixel -= dest_y0;
		dest_y0 = 0;
	}
	if (c1 & 4) {
		x1_pixel -= dest_x1 - dw;
		dest_x1 = dw;
	}
	if (c1 & 1) {
		y1_pixel -= dest_y1 - dh;
		dest_y1 = dh;
	}

	int sstride = VP_Stride(thisptr);
	int dstride = VP_Stride(dest);
	unsigned char * src = VP_Offset(thisptr) + y_pixel * sstride + x_pixel;
	unsigned char * dst = VP_Offset(dest) + dest_y0 * dstride + dest_x0;
	int w = dest_x1 - dest_x0;
	if (w <= 0) return(FALSE);
	int h = dest_y1 - dest_y0;
	if (h <= 0) return(FALSE);
	if (src == dst) return(FALSE);

	if (src > dst) {
		/*
		**	Forward copy.
		*/
		for (int y = 0; y < h; y++) {
			if (trans) {
				for (int x = 0; x < w; x++) {
					if (src[x]) dst[x] = src[x];
				}
			} else {
				memmove(dst, src, w);
			}
			src += sstride;
			dst += dstride;
		}
	} else {
		/*
		**	Backward copy (bottom line first, right to left) so that overlapping regions
		**	within the same buffer are handled correctly.
		*/
		src += (h - 1) * sstride;
		dst += (h - 1) * dstride;
		for (int y = 0; y < h; y++) {
			if (trans) {
				for (int x = w - 1; x >= 0; x--) {
					if (src[x]) dst[x] = src[x];
				}
			} else {
				memmove(dst, src, w);
			}
			src -= sstride;
			dst -= dstride;
		}
	}
	return(TRUE);
}


/***************************************************************************
 * Linear_Scale_To_Linear -- Scales a region from one view port to another *
 *                                                                         *
 *    Uses the same 16.16 fixed point stepping as the original, so the     *
 *    selection of source pixels is identical.                             *
 *=========================================================================*/
BOOL __cdecl Linear_Scale_To_Linear(void * thisptr, void * dest, int src_x, int src_y, int dst_x, int dst_y, int src_width, int src_height, int dst_width, int dst_height, BOOL trans, char * remap)
{
	if (dst_width == 0 || dst_height == 0 || src_width == 0 || src_height == 0) return(FALSE);

	int src_x0 = src_x;
	int src_y0 = src_y;
	int src_x1 = src_x + src_width;
	int src_y1 = src_y + src_height;
	int dst_x0 = dst_x;
	int dst_y0 = dst_y;
	int dst_x1 = dst_x + dst_width;
	int dst_y1 = dst_y + dst_height;

	/*
	**	Clip against the source view port, scaling the destination to match.
	*/
	int sw = VP_Width(thisptr);
	int sh = VP_Height(thisptr);
	int c0 = Out_Code(src_x0, src_y0, sw, sh);
	int c1 = Out_Code(src_x1, src_y1, sw, sh);
	if (c0 & c1) return(FALSE);
	if (c0 & 8) {
		src_x0 = 0;
		dst_x0 = (int)(((long long)(0 - src_x) * dst_width) / src_width) + dst_x;
	}
	if (c0 & 2) {
		src_y0 = 0;
		dst_y0 = (int)(((long long)(0 - src_y) * dst_height) / src_height) + dst_y;
	}
	if (c1 & 4) {
		src_x1 = sw;
		dst_x1 = (int)(((long long)(sw - src_x) * dst_width) / src_width) + dst_x;
	}
	if (c1 & 1) {
		src_y1 = sh;
		dst_y1 = (int)(((long long)(sh - src_y) * dst_height) / src_height) + dst_y;
	}

	/*
	**	Clip against the destination view port, scaling the source to match.
	*/
	int dw = VP_Width(dest);
	int dh = VP_Height(dest);
	c0 = Out_Code(dst_x0, dst_y0, dw, dh);
	c1 = Out_Code(dst_x1, dst_y1, dw, dh);
	if (c0 & c1) return(FALSE);
	if (c0 & 8) {
		dst_x0 = 0;
		src_x0 = (int)(((long long)(0 - dst_x) * src_width) / dst_width) + src_x;
	}
	if (c0 & 2) {
		dst_y0 = 0;
		src_y0 = (int)(((long long)(0 - dst_y) * src_height) / dst_height) + src_y;
	}
	if (c1 & 4) {
		dst_x1 = dw;
		src_x1 = (int)(((long long)(dw - dst_x) * src_width) / dst_width) + src_x;
	}
	if (c1 & 1) {
		dst_y1 = dh;
		src_y1 = (int)(((long long)(dh - dst_y) * src_height) / dst_height) + src_y;
	}

	int src_win_width = VP_Stride(thisptr);
	int dst_win_width = VP_Stride(dest);
	unsigned char const * src = VP_Offset(thisptr) + src_y0 * src_win_width + src_x0;
	unsigned char * dst = VP_Offset(dest) + dst_y0 * dst_win_width + dst_x0;

	/*
	**	Vertical stepping: whole source lines per destination line plus a remainder.
	*/
	int dy_intr = (src_height / dst_height) * src_win_width;
	int dy_frac = src_height % dst_height;
	int dy_acc = -dst_height;

	/*
	**	Horizontal stepping: 16.16 fixed point.
	*/
	unsigned int step = (unsigned int)(((long long)src_width << 16) / dst_width);
	unsigned int dx_int = step >> 16;
	unsigned int dx_frac = step << 16;

	int rows = dst_y1 - dst_y0;
	if (rows <= 0) return(FALSE);
	int cols = dst_x1 - dst_x0;
	if (cols <= 0) return(FALSE);

	unsigned char const * table = (unsigned char const *)remap;
	while (rows--) {
		unsigned char const * s = src;
		unsigned char * d = dst;
		unsigned int acc = 0;
		for (int i = 0; i < cols; i++) {
			unsigned char pixel = *s;
			if (table) pixel = table[pixel];
			if (!trans || *s) *d = pixel;
			unsigned int old = acc;
			acc += dx_frac;
			s += dx_int + (acc < old ? 1 : 0);
			d++;
		}
		dst += dst_win_width;
		src += dy_intr;
		dy_acc += dy_frac;
		if (dy_acc > 0) {
			src += src_win_width;
			dy_acc -= dst_height;
		}
	}
	return(TRUE);
}


/***************************************************************************
 * Icon (terrain tile) drawing. "icondata" is an IControl_Type icon set.    *
 *=========================================================================*/
static unsigned char const *	StampPtr = NULL;
static unsigned char const *	IsTrans = NULL;
static unsigned char const *	MapPtr = NULL;
static int					IconWidth = 0;
static int					IconHeight = 0;
static int					IconSize = 0;
static int					IconCount = 0;

/*
**	The icon set's layout, read for each icon drawn. (It isn't remembered by the set's address:
**	a set can be freed and another loaded at the same address, which then drew with the first
**	one's layout. Seen in the browser build, whose memory allocator reuses addresses sooner.)
*/
static void Init_Stamps(void const * icondata)
{
	if (icondata == NULL) return;
	IControl_Type const * control = (IControl_Type const *)icondata;
	IconCount = (unsigned short)control->Count;
	IconWidth = (unsigned short)control->Width;
	IconHeight = (unsigned short)control->Height;
	IconSize = IconWidth * IconHeight;
	MapPtr = (unsigned char const *)icondata + control->Map;
	StampPtr = (unsigned char const *)icondata + control->Icons;
	IsTrans = (unsigned char const *)icondata + control->TransFlag;
}

static int Map_Icon(int icon)
{
	/*
	**	Only the low byte of the icon number was replaced by the map lookup.
	*/
	if (MapPtr) icon = (icon & ~0xFF) | MapPtr[icon];
	return(icon);
}

void __cdecl Buffer_Draw_Stamp(void const * thisptr, void const * icondata, int icon, int x_pixel, int y_pixel, void const * remap)
{
	if (icondata == NULL) return;
	Init_Stamps(icondata);
	icon = Map_Icon(icon);
	if ((unsigned)icon >= (unsigned)IconCount) return;

	int stride = VP_Stride(thisptr);
	unsigned char * dst = VP_Offset(thisptr) + y_pixel * stride + x_pixel;
	unsigned char const * src = StampPtr + icon * IconSize;
	unsigned char const * table = (unsigned char const *)remap;

	for (int y = 0; y < IconHeight; y++) {
		if (table) {
			for (int x = 0; x < IconWidth; x++) {
				unsigned char c = table[src[x]];
				if (c) dst[x] = c;
			}
		} else if (IsTrans[icon]) {
			for (int x = 0; x < IconWidth; x++) {
				if (src[x]) dst[x] = src[x];
			}
		} else {
			memcpy(dst, src, IconWidth);
		}
		src += IconWidth;
		dst += stride;
	}
}

void __cdecl Buffer_Draw_Stamp_Clip(void const * thisptr, void const * icondata, int icon, int x_pixel, int y_pixel, void const * remap, int min_x, int min_y, int max_x, int max_y)
{
	if (icondata == NULL) return;
	Init_Stamps(icondata);
	icon = Map_Icon(icon);
	if ((unsigned)icon >= (unsigned)IconCount) return;

	int rows = IconHeight;
	int iwidth = IconWidth;
	unsigned char const * src = StampPtr + icon * IconSize;

	/*
	**	The clip window is given as origin + size; the icon position is window relative.
	*/
	max_x += min_x;
	x_pixel += min_x;
	max_y += min_y;
	y_pixel += min_y;

	if (x_pixel >= max_x) return;
	if (y_pixel >= max_y) return;
	if (y_pixel + IconHeight <= min_y) return;
	if (x_pixel + IconWidth <= min_x) return;

	if (x_pixel < min_x) {
		int clip = min_x - x_pixel;
		src += clip;
		iwidth -= clip;
		x_pixel = min_x;
	}
	int skip = IconWidth - iwidth;
	if (x_pixel + iwidth > max_x) {
		int old = iwidth;
		iwidth = max_x - x_pixel;
		skip += old - iwidth;
	}
	if (min_y > y_pixel) {
		int clip = min_y - y_pixel;
		rows -= clip;
		src += clip * IconWidth;
		y_pixel = min_y;
	}
	if (y_pixel + rows > max_y) {
		rows = max_y - y_pixel;
	}

	int stride = VP_Stride(thisptr);
	unsigned char * dst = VP_Offset(thisptr) + y_pixel * stride + x_pixel;
	unsigned char const * table = (unsigned char const *)remap;

	for (int y = 0; y < rows; y++) {
		if (table) {
			for (int x = 0; x < iwidth; x++) {
				unsigned char c = table[src[x]];
				if (c) dst[x] = c;
			}
		} else if (IsTrans[icon]) {
			for (int x = 0; x < iwidth; x++) {
				if (src[x]) dst[x] = src[x];
			}
		} else {
			memcpy(dst, src, iwidth);
		}
		src += iwidth + skip;
		dst += stride;
	}
}

}	// extern "C"
