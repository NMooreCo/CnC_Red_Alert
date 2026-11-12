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
 *                    File Name : PALETTE.CPP                                                  *
 *                                                                                             *
 *   This file was not part of the source release. It has been reconstructed for the modern  *
 *   port from the way PaletteClass is used throughout the code base.                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   PaletteClass::Adjust -- Adjusts the palette toward another palette.                       *
 *   PaletteClass::Adjust -- Adjusts this palette toward black.                                *
 *   PaletteClass::Closest_Color -- Finds closest match to color specified.                    *
 *   PaletteClass::Partial_Adjust -- Adjusts the specified parts of this palette toward black. *
 *   PaletteClass::Partial_Adjust -- Adjusts the palette toward another palette.               *
 *   PaletteClass::PaletteClass -- Constructor that fills palette with color specified.        *
 *   PaletteClass::Set -- Fade the display palette to this palette.                            *
 *   PaletteClass::operator = -- Assignment operator for palette objects.                      *
 *   PaletteClass::operator == -- Equality operator for palette objects.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"FUNCTION.H"
#include	"PALETTE.H"


PaletteClass const PaletteClass::CurrentPalette;


/***********************************************************************************************
 * PaletteClass::PaletteClass -- Constructor that fills palette with color specified.         *
 *=============================================================================================*/
PaletteClass::PaletteClass(RGBClass const & rgb)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		Palette[index] = rgb;
	}
}


/***********************************************************************************************
 * PaletteClass::operator == -- Equality operator for palette objects.                        *
 *=============================================================================================*/
int PaletteClass::operator == (PaletteClass const & palette) const
{
	if (this == &palette) return(true);
	return(memcmp(&Palette[0], &palette.Palette[0], sizeof(Palette)) == 0);
}


/***********************************************************************************************
 * PaletteClass::operator = -- Assignment operator for palette objects.                       *
 *=============================================================================================*/
PaletteClass & PaletteClass::operator = (PaletteClass const & palette)
{
	if (this == &palette) return(*this);

	memcpy(&Palette[0], &palette.Palette[0], sizeof(Palette));
	return(*this);
}


/***********************************************************************************************
 * PaletteClass::Adjust -- Adjusts this palette toward black.                                  *
 *                                                                                             *
 * INPUT:   ratio -- The ratio to adjust this palette toward black (0..255 means 0..100%).     *
 *=============================================================================================*/
void PaletteClass::Adjust(int ratio)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		Palette[index].Adjust(ratio, BlackColor);
	}
}


/***********************************************************************************************
 * PaletteClass::Adjust -- Adjusts the palette toward another palette.                         *
 *                                                                                             *
 * INPUT:   ratio    -- The ratio to adjust toward the target palette (0..255 means 0..100%).  *
 *          palette  -- The palette to adjust toward.                                          *
 *=============================================================================================*/
void PaletteClass::Adjust(int ratio, PaletteClass const & palette)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		Palette[index].Adjust(ratio, palette[index]);
	}
}


/***********************************************************************************************
 * PaletteClass::Partial_Adjust -- Adjusts the specified parts of this palette toward black.   *
 *                                                                                             *
 * INPUT:   ratio          -- The ratio to adjust toward black.                                *
 *          palette_lookup -- Table of 256 flags; only colors with a non zero flag change.     *
 *=============================================================================================*/
void PaletteClass::Partial_Adjust(int ratio, char * palette_lookup)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		if (palette_lookup[index]) {
			Palette[index].Adjust(ratio, BlackColor);
		}
	}
}


/***********************************************************************************************
 * PaletteClass::Partial_Adjust -- Adjusts the palette toward another palette.                 *
 *                                                                                             *
 * INPUT:   ratio          -- The ratio to adjust toward the target palette.                   *
 *          palette        -- The palette to adjust toward.                                    *
 *          palette_lookup -- Table of 256 flags; only colors with a non zero flag change.     *
 *=============================================================================================*/
void PaletteClass::Partial_Adjust(int ratio, PaletteClass const & palette, char * palette_lookup)
{
	for (int index = 0; index < COLOR_COUNT; index++) {
		if (palette_lookup[index]) {
			Palette[index].Adjust(ratio, palette[index]);
		}
	}
}


/***********************************************************************************************
 * PaletteClass::Closest_Color -- Finds closest match to color specified.                      *
 *                                                                                             *
 * INPUT:   rgb   -- The color to find the closest match for.                                  *
 *                                                                                             *
 * OUTPUT:  Returns with the color index of the closest matching color in this palette.        *
 *=============================================================================================*/
int PaletteClass::Closest_Color(RGBClass const & rgb) const
{
	int closest = 0;
	int value = -1;

	RGBClass const * ptr = &Palette[0];
	for (int index = 0; index < COLOR_COUNT; index++) {
		int difference = rgb.Difference(*ptr++);
		if (value == -1 || difference < value) {
			value = difference;
			closest = index;
		}
	}
	return(closest);
}


/***********************************************************************************************
 * PaletteClass::Set -- Fade the display palette to this palette.                              *
 *                                                                                             *
 *    This routine will fade the current display palette to match this palette over the time  *
 *    specified (in 1/60 second ticks). The callback (if any) is called repeatedly during the  *
 *    fade so that the game can continue to process background events.                        *
 *                                                                                             *
 * INPUT:   time     -- The time to take to perform the fade (0 means set it instantly).       *
 *          callback -- Optional routine to call during the fade.                              *
 *=============================================================================================*/
void PaletteClass::Set(int time, void (* callback)(void)) const
{
	CDTimerClass<SystemTimerClass> timer = time;
	PaletteClass original = CurrentPalette;

	while (timer) {

		/*
		**	Build an intermediate palette that is as close to the destination palette
		**	as the current time is proportional to the ending time.
		*/
		PaletteClass palette = original;
		int adjust = ((time - timer) * 256) / time;
		palette.Adjust(adjust, *this);

		/*
		**	Remember the current time so that multiple palette sets within the same game
		**	time tick won't occur.
		*/
		int holdtime = timer;

		/*
		**	Set the palette to this intermediate palette and then loop back
		**	to calculate and set a new intermediate palette.
		*/
		Set_Palette((void *)&palette[0]);
		((PaletteClass &)CurrentPalette) = palette;

		/*
		**	If the callback routine was specified, then call it once per palette
		**	setting loop.
		*/
		if (callback) {
			callback();
		}

		/*
		**	This loop ensures that the palette won't be set more than once per game tick.
		*/
		while (timer == holdtime && holdtime != 0) {
			if (callback) callback();
		}
	}

	/*
	**	Ensure that the final palette exactly matches the requested
	**	palette before exiting the fading routine.
	*/
	Set_Palette((void *)&Palette[0]);
	((PaletteClass &)CurrentPalette) = *this;
}
