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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood Win32 Library                   *
 *                                                                         *
 *                    File Name : DDRAW.CPP                                *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : October 10, 1995                         *
 *                                                                         *
 *                  Last Update : October 10, 1995   []                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/*
**	Modern port: DirectDraw has been replaced by the SDL2 platform layer (WIN32LIB/SDL).
**	The entry points the game uses are kept here and forward to it.
*/

#include "MISC.H"
#include "GBUFFER.H"
#include "PALETTE.H"
#include "PLATFORM.H"

HWND						MainWindow = NULL;			// Handle to programs main window (unused by the port)
SurfaceMonitorClass	AllSurfaces;					// Tracks surface "restores" (focus changes)
BOOL						CanVblankSync = TRUE;

BOOL						SystemToVideoBlits =FALSE;	// Does hardware support system mem to video mem blits?
BOOL						VideoToSystemBlits =FALSE;	// Does hardware support video mem to system mem blits?
BOOL						SystemToSystemBlits = FALSE; 	// Does hardware support system mem to system mem blits?
BOOL						OverlappedVideoBlits = TRUE;	// Can video driver blit overlapped regions?

/*
** Function to call if we detect focus loss
*/
void (*Misc_Focus_Loss_Function)(void) = NULL;
void (*Misc_Focus_Restore_Function)(void) = NULL;


/***********************************************************************************************
 * Process_DD_Result -- Display a message describing a DirectDraw error (no longer needed).   *
 *=============================================================================================*/
void Process_DD_Result(HRESULT , int )
{
}


/***********************************************************************************************
 * Check_Overlapped_Blit_Capability -- Software blits always handle overlapping regions.      *
 *=============================================================================================*/
void Check_Overlapped_Blit_Capability(void)
{
	OverlappedVideoBlits = TRUE;
}


/***********************************************************************************************
 * Set_Video_Mode -- Creates the game window with the requested logical resolution.           *
 *                                                                                             *
 * INPUT:   hwnd           -- (unused)                                                         *
 *          w, h           -- The logical screen size the game will draw at.                  *
 *          bits_per_pixel -- Must be 8; the game is palette based.                            *
 *                                                                                             *
 * OUTPUT:  TRUE if the window could be created.                                               *
 *=============================================================================================*/
BOOL Set_Video_Mode(HWND , int w, int h, int bits_per_pixel)
{
	if (bits_per_pixel != 8) return(FALSE);
	return(Video_Init(w, h) ? TRUE : FALSE);
}


/***********************************************************************************************
 * Reset_Video_Mode -- Shuts down the game window.                                             *
 *=============================================================================================*/
void Reset_Video_Mode(void)
{
	Video_Shutdown();
}


/***********************************************************************************************
 * Get_Free_Video_Memory -- Reports the amount of "video memory" available.                   *
 *=============================================================================================*/
unsigned int Get_Free_Video_Memory(void)
{
	return(64 * 1024 * 1024);
}


/***********************************************************************************************
 * Get_Video_Hardware_Capabilities -- Reports the capabilities of the video "hardware".        *
 *=============================================================================================*/
unsigned Get_Video_Hardware_Capabilities(void)
{
	return(VIDEO_BLITTER | VIDEO_COLOR_FILL | VIDEO_SYNC_PALETTE);
}


/***********************************************************************************************
 * Wait_Vert_Blank -- Waits for the next display refresh.                                      *
 *                                                                                             *
 *    The original waited for the vertical blank before changing the palette. The port simply *
 *    makes sure anything drawn so far reaches the screen.                                    *
 *=============================================================================================*/
void Wait_Vert_Blank(void)
{
	Video_Present();
}


/***********************************************************************************************
 * Set_DD_Palette -- Sets the display palette.                                                 *
 *                                                                                             *
 * INPUT:   palette -- 768 bytes of 6 bit RGB triplets.                                        *
 *=============================================================================================*/
void Set_DD_Palette ( void *palette )
{
	Video_Set_Palette(palette);
}


/***********************************************************************************************
 * Wait_Blit -- Waits for the blitter to finish (all blits are synchronous now).              *
 *=============================================================================================*/
void Wait_Blit (void)
{
}
