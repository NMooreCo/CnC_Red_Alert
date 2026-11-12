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
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : PALETTE.H                                                    *
 *                                                                                             *
 *   This file was not part of the source release. It has been reconstructed for the modern  *
 *   port from the way PaletteClass is used throughout the code base.                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#ifndef CCPALETTE_H
#define CCPALETTE_H

#include	"RGB.H"

/*
**	The palette class is used to manipulate a palette as a whole. All 256 colors are
**	represented by the palette class object.
*/
class PaletteClass
{
	public:
		enum {
			COLOR_COUNT=256		// Number of color indices on the palette.
		};

		PaletteClass(void) {};
		PaletteClass(RGBClass const & rgb);

		RGBClass & operator[] (int index) {return(Palette[index % COLOR_COUNT]);};
		RGBClass const & operator[] (int index) const {return(Palette[index % COLOR_COUNT]);};
		int operator == (PaletteClass const & palette) const;
		int operator != (PaletteClass const & palette) const {return(!(operator ==(palette)));};
		PaletteClass & operator = (PaletteClass const & palette);
		operator const unsigned char * (void) const {return((const unsigned char *)&Palette[0]);};
		operator unsigned char * (void) {return((unsigned char *)&Palette[0]);};

		void Adjust(int ratio);
		void Adjust(int ratio, PaletteClass const & palette);
		void Partial_Adjust(int ratio, char *palette_lookup);
		void Partial_Adjust(int ratio, PaletteClass const & palette, char *palette_lookup);
		int Closest_Color(RGBClass const & rgb) const;

		void Set(int time = 0, void (* callback)(void) = NULL) const;

		/*
		**	This is the most recently set palette (the one that is currently visible).
		*/
		static PaletteClass const CurrentPalette;

	protected:
		RGBClass Palette[COLOR_COUNT];
};

#endif
