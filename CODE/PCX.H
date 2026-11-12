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
 *                    File Name : PCX.H                                                        *
 *                                                                                             *
 *   This file was not part of the source release. It has been reconstructed for the modern  *
 *   port: PCX file structures (identical to FILEPCX.H) and the screen shot writer used by    *
 *   WRITEPCX.CPP.                                                                             *
 *---------------------------------------------------------------------------------------------*/

#ifndef PCX_H
#define PCX_H

#pragma pack(push, 1)

typedef	struct {
	char	red;
	char	green;
	char	blue;
} RGB;

typedef	struct	{
	char	id;
	char	version;
	char	encoding;
	char	pixelsize;
	short	x;
	short	y;
	short	width;
	short	height;
	short	xres;
	short	yres;
	RGB	ega_palette[16];
	char	nothing;
	char	color_planes;
	short	byte_per_line;
	short	palette_type;
	char	filler[58];
} PCX_HEADER;

#pragma pack(pop)

#endif

#ifndef CC_PCX_WRITE_H
#define CC_PCX_WRITE_H
class FileClass;
class GraphicBufferClass;
class PaletteClass;
int Write_PCX_File(FileClass & file, GraphicBufferClass & pic, PaletteClass * palette);
#endif
